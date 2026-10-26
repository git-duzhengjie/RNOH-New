/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef IMAGE_CACHE_PIXELMAP_UTILS_H
#define IMAGE_CACHE_PIXELMAP_UTILS_H

#include <multimedia/image_framework/image/pixelmap_native.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "ImageCacheData.h"
#include "ImageCacheLog.h"
#include "ImageCacheType.h"

namespace ImageCachePro {

class ImageCachePixelmapUtils {
 public:
  static void SetMemoryName(
      std::shared_ptr<ImageCacheData> data,
      const std::string& name) {
    (void)data;
    (void)name;
  }

  static Image_Size GetImageSize(
      OH_PixelmapNative* data,
      int32_t* pixelFormat = nullptr) {
    Image_Size imageSize = {0, 0};
    IMAGE_CACHE_CHECK_RETURN(data == nullptr, imageSize);
    OH_Pixelmap_ImageInfo* imageInfo = nullptr;
    Image_ErrorCode errorCode = OH_PixelmapImageInfo_Create(&imageInfo);
    if (errorCode != IMAGE_SUCCESS) {
      LOG(ERROR) << "GetImageSize Create ImageInfo Failed: " << errorCode;
      return imageSize;
    }
    errorCode = OH_PixelmapNative_GetImageInfo(data, imageInfo);
    if (errorCode != IMAGE_SUCCESS) {
      OH_PixelmapImageInfo_Release(imageInfo);
      return imageSize;
    }

    OH_PixelmapImageInfo_GetWidth(imageInfo, &imageSize.width);
    OH_PixelmapImageInfo_GetHeight(imageInfo, &imageSize.height);
    if (pixelFormat != nullptr) {
      int32_t format = 0;
      OH_PixelmapImageInfo_GetPixelFormat(imageInfo, &format);
      *pixelFormat = format;
    }
    OH_PixelmapImageInfo_Release(imageInfo);
    return imageSize;
  }

  static size_t GetFrameSize(
      uint32_t width,
      uint32_t height,
      int32_t pixelFormat) {
    const uint64_t pixels = static_cast<uint64_t>(width) * height;
    const double frameSizeDouble =
        static_cast<double>(pixels) * FormatToBytesPerPixel(pixelFormat);
    const uint64_t frameSize =
        static_cast<uint64_t>(std::ceil(frameSizeDouble));

    return frameSize > static_cast<uint64_t>(SIZE_MAX)
        ? SIZE_MAX
        : static_cast<size_t>(frameSize);
  }

  static size_t GetFrameSize(const ImageCacheFileTypeInfo& fileTypeInfo) {
    return GetFrameSize(
        Image_Size{fileTypeInfo.width, fileTypeInfo.height},
        fileTypeInfo.pixelFormat);
  }

  static size_t GetFrameSize(
      const Image_Size& imageSize,
      int32_t pixelFormat) {
    return GetFrameSize(imageSize.width, imageSize.height, pixelFormat);
  }

  static void Orientate(
      std::shared_ptr<ImageCacheData> data,
      const std::string& orientation) {
    IMAGE_CACHE_CHECK(data == nullptr);
    IMAGE_CACHE_CHECK(data->GetFrameCount() < 1);
    auto pixelmapList = data->GetPixelmapList();
    IMAGE_CACHE_CHECK(pixelmapList == nullptr);

    auto func = GetOrientateFunc(orientation);
    if (func == nullptr) {
      return;
    }

    uint32_t frameCount = data->GetFrameCount();
    for (uint32_t i = 0; i < frameCount; i++) {
      auto* pixelmap = pixelmapList[i];
      if (pixelmap != nullptr) {
        func(pixelmap);
      }
    }
  }

  static void Orientate(
      std::shared_ptr<ImageCacheData> data,
      ImageCacheOrientation orientation) {
    if (orientation == ImageCacheOrientation::AUTO ||
        orientation == ImageCacheOrientation::UP) {
      return;
    }

    static std::unordered_map<ImageCacheOrientation, std::string> enumMap = {
        {ImageCacheOrientation::DOWN, "Bottom-right"},
        {ImageCacheOrientation::LEFT, "Left-bottom"},
        {ImageCacheOrientation::RIGHT, "Right-top"},
    };

    if (enumMap.find(orientation) != enumMap.end()) {
      Orientate(data, enumMap[orientation]);
    }
  }

 private:
  static std::function<void(OH_PixelmapNative*)> GetOrientateFunc(
      const std::string& orientation) {
    const float rotate90 = 90;
    const float rotate180 = 180;
    const float rotate270 = 270;
    static std::
        unordered_map<std::string, std::function<void(OH_PixelmapNative*)>>
            funcMap = {
                {"Top-left", nullptr},
                {"Top-right",
                 [](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Flip(pixelmap, true, false);
                 }},
                {"Bottom-right",
                 [rotate180](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Rotate(pixelmap, rotate180);
                 }},
                {"Bottom-left",
                 [](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Flip(pixelmap, false, true);
                 }},
                {"Left-top",
                 [rotate270](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Flip(pixelmap, true, false);
                   OH_PixelmapNative_Rotate(pixelmap, rotate270);
                 }},
                {"Right-top",
                 [rotate90](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Rotate(pixelmap, rotate90);
                 }},
                {"Right-bottom",
                 [rotate90](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Flip(pixelmap, true, false);
                   OH_PixelmapNative_Rotate(pixelmap, rotate90);
                 }},
                {"Left-bottom",
                 [rotate270](OH_PixelmapNative* pixelmap) {
                   OH_PixelmapNative_Rotate(pixelmap, rotate270);
                 }},
            };

    if (funcMap.find(orientation) != funcMap.end()) {
      return funcMap[orientation];
    }
    return nullptr;
  }

  static float FormatToBytesPerPixel(int32_t format) {
    switch (format) {
      case PIXEL_FORMAT_RGBA_8888:
      case PIXEL_FORMAT_BGRA_8888:
      case PIXEL_FORMAT_RGBA_1010102:
        return 4.0f;
      case PIXEL_FORMAT_RGB_888:
        return 3.0f;
      case PIXEL_FORMAT_RGB_565:
        return 2.0f;
      case PIXEL_FORMAT_ALPHA_8:
        return 1.0f;
      case PIXEL_FORMAT_RGBA_F16:
        return 8.0f;
      case PIXEL_FORMAT_NV21:
      case PIXEL_FORMAT_NV12:
        return 1.5f; // YUV 4:2:0
      default:
        return 4.0f;
    }
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_PIXELMAP_UTILS_H
