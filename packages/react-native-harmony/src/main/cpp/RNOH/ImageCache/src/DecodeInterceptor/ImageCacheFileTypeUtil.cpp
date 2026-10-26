/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ImageCacheFileTypeUtil.h"
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <map>
#include <vector>
#include "ImageCacheLog.h"
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

namespace {

static bool MatchBoxType(
    const uint8_t* buffer,
    uint32_t length,
    uint32_t offset,
    const char* type) {
  if (buffer == nullptr || type == nullptr || offset + 4 > length) {
    return false;
  }
  return buffer[offset] == static_cast<uint8_t>(type[0]) &&
      buffer[offset + 1] == static_cast<uint8_t>(type[1]) &&
      buffer[offset + 2] == static_cast<uint8_t>(type[2]) &&
      buffer[offset + 3] == static_cast<uint8_t>(type[3]);
}

static bool MatchBrand(
    const uint8_t* buffer,
    uint32_t length,
    uint32_t offset,
    const char* brand) {
  if (buffer == nullptr || brand == nullptr || offset + 4 > length) {
    return false;
  }
  return buffer[offset] == static_cast<uint8_t>(brand[0]) &&
      buffer[offset + 1] == static_cast<uint8_t>(brand[1]) &&
      buffer[offset + 2] == static_cast<uint8_t>(brand[2]) &&
      buffer[offset + 3] == static_cast<uint8_t>(brand[3]);
}

static bool
IsHeicBrand(const uint8_t* buffer, uint32_t length, uint32_t brandOffset) {
  static const char* kHeicBrands[] = {
      "heic",
      "heix",
      "hevc",
      "hevx",
      "heim",
      "heis",
      "hevm",
      "hevs",
      "mif1",
      "msf1",
      "heif",
      nullptr,
  };
  for (const char** brand = kHeicBrands; *brand != nullptr; ++brand) {
    if (MatchBrand(buffer, length, brandOffset, *brand)) {
      return true;
    }
  }
  return false;
}

/** HEIC/HEIF uses ISOBMFF container, detects ftyp box and HEIF brand
 * identifiers */
static bool DetectHeic(const uint8_t* buffer, uint32_t length) {
  if (buffer == nullptr || length < 12) {
    return false;
  }
  if (MatchBoxType(buffer, length, 4, "ftyp") &&
      IsHeicBrand(buffer, length, 8)) {
    return true;
  }
  constexpr uint32_t kScanLimit = 64;
  const uint32_t limit = length < kScanLimit ? length : kScanLimit;
  for (uint32_t i = 4; i + 8 <= limit; ++i) {
    if (MatchBoxType(buffer, length, i, "ftyp") &&
        IsHeicBrand(buffer, length, i + 4)) {
      return true;
    }
  }
  return false;
}

} // namespace

static bool MatchSignature(
    const uint8_t* buffer,
    uint32_t length,
    const std::vector<uint8_t>& signature) {
  if (buffer == nullptr || length < signature.size()) {
    return false;
  }
  for (size_t i = 0; i < signature.size(); i++) {
    if (signature[i] != 0 && buffer[i] != signature[i]) {
      return false;
    }
  }
  return true;
}

static ImageCacheFormat DetectFormat(const uint8_t* buffer, uint32_t length) {
  static std::map<ImageCacheFormat, std::vector<std::vector<uint8_t>>>
      signatures = {
          {ImageCacheFormat::JPG, {{0xFF, 0xD8}}},
          {ImageCacheFormat::BMP, {{0x42, 0x4D}}},
          {ImageCacheFormat::GIF, {{0x47, 0x49, 0x46}}},
          {ImageCacheFormat::WEBP,
           {{0x52, 0x49, 0x46, 0x46, 0, 0, 0, 0, 0x57, 0x45, 0x42, 0x50}}},
          {ImageCacheFormat::PNG,
           {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}}},
          {ImageCacheFormat::SVG,
           {{0x3C, 0x3F, 0x78, 0x6D, 0x6C}, {0x3C, 0x73, 0x76, 0x67, 0x20}}},
          {ImageCacheFormat::ICO, {{0x00, 0x00, 0x01, 0x00}}},
      };

  for (const auto& entry : signatures) {
    for (const auto& sig : entry.second) {
      if (MatchSignature(buffer, length, sig)) {
        return entry.first;
      }
    }
  }
  if (DetectHeic(buffer, length)) {
    return ImageCacheFormat::HEIC;
  }
  return ImageCacheFormat::UNKNOWN;
}

static void FillImageSourceInfo(
    OH_ImageSourceNative* source,
    ImageCacheFileTypeInfo& info) {
  if (source == nullptr) {
    return;
  }
  OH_ImageSource_Info* imageInfo = nullptr;
  if (OH_ImageSourceInfo_Create(&imageInfo) != IMAGE_SUCCESS) {
    return;
  }
  if (OH_ImageSourceNative_GetImageInfo(source, 0, imageInfo) ==
      IMAGE_SUCCESS) {
    OH_ImageSourceInfo_GetWidth(imageInfo, &info.width);
    OH_ImageSourceInfo_GetHeight(imageInfo, &info.height);
  }
  OH_ImageSourceInfo_Release(imageInfo);
  uint32_t frameCount = 1;
  if (OH_ImageSourceNative_GetFrameCount(source, &frameCount) ==
          IMAGE_SUCCESS &&
      frameCount > 0) {
    info.frameCount = frameCount;
  }
}

ImageCacheErrorCode ImageCacheFileTypeUtil::ParseFromBuffer(
    ImageCacheTaskInternal& task,
    ImageCacheFileTypeInfo& outInfo) {
  const auto& product = task.product;
  if (product.imageBuffer == nullptr || product.imageLength == 0) {
    return ImageCacheErrorCode::EMPTY_IMAGE_BUFFER;
  }
  outInfo.format = DetectFormat(product.imageBuffer.get(), product.imageLength);
  if (outInfo.format == ImageCacheFormat::UNKNOWN) {
    return ImageCacheErrorCode::UNKNOWN_FILE_TYPE;
  }

  OH_ImageSourceNative* source = task.GetImageSourceNative();
  if (source == nullptr) {
    Image_ErrorCode errCode = OH_ImageSourceNative_CreateFromData(
        const_cast<uint8_t*>(product.imageBuffer.get()),
        product.imageLength,
        &source);
    if (errCode != IMAGE_SUCCESS || source == nullptr) {
      return ImageCacheErrorCode::CREATE_SOURCE_FAILED;
    }
    task.SetImageSourceNative(source);
  }
  FillImageSourceInfo(source, outInfo);
  return ImageCacheErrorCode::OK;
}

} // namespace ImageCachePro
