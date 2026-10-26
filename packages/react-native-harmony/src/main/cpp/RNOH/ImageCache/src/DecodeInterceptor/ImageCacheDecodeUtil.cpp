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

#include "ImageCacheDecodeUtil.h"
#include <cstring>
#include "ImageCacheConfig.h"
#include "ImageCacheData.h"
#include "ImageCacheDataCacheInternal.h"
#include "ImageCacheLog.h"

namespace ImageCachePro {

namespace {
constexpr int kDefaultFrameDelayMs = 100;
}

bool ImageCacheDecodeUtil::IsAnimatedFormat(ImageCacheFormat format) {
  return format == ImageCacheFormat::GIF || format == ImageCacheFormat::WEBP;
}

bool ImageCacheDecodeUtil::ParseLazyHeader(
    const std::shared_ptr<ImageCacheTaskInternal>& taskInternal) {
  auto frameDecodeContext = taskInternal->product.frameDecodeContext;
  if (frameDecodeContext == nullptr) {
    return true;
  }
  frameDecodeContext->imageSourceNative_ =
      taskInternal->GetImageSourceNativeWrapper();

  uint32_t frameCount = 0;
  auto delayTimeList = GetDelayTimeList(taskInternal, frameCount);
  if (delayTimeList == nullptr || frameCount <= 1) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
    return false;
  }
  frameDecodeContext->frameCount_ = frameCount;
  frameDecodeContext->animationDelayTimeList_ = std::move(delayTimeList);
  return true;
}

OH_DecodingOptions* ImageCacheDecodeUtil::CreateDecodeOptions(
    const Image_Size& desiredSize) {
  OH_DecodingOptions* decodeOption = nullptr;
  if (OH_DecodingOptions_Create(&decodeOption) != IMAGE_SUCCESS ||
      decodeOption == nullptr) {
    return nullptr;
  }

  if (desiredSize.width != 0 && desiredSize.height != 0) {
    Image_Size size = desiredSize;
    OH_DecodingOptions_SetDesiredSize(decodeOption, &size);
  }
  return decodeOption;
}

void ImageCacheDecodeUtil::ReleasePixelmapList(
    OH_PixelmapNative** pixelmapList,
    size_t frameCount) {
  if (pixelmapList == nullptr) {
    return;
  }
  for (size_t i = 0; i < frameCount; i++) {
    if (pixelmapList[i] != nullptr) {
      OH_PixelmapNative_Release(pixelmapList[i]);
      pixelmapList[i] = nullptr;
    }
  }
}

bool ImageCacheDecodeUtil::DecodeStatic(
    const std::shared_ptr<ImageCacheTaskInternal>& taskInternal) {
  OH_DecodingOptions* decodeOption =
      CreateDecodeOptions(taskInternal->GetDesiredImageSize());
  if (decodeOption == nullptr) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::CREATE_DECODE_OPTION_FAILED);
    return false;
  }

  OH_PixelmapNative* pixelmap = nullptr;
  Image_ErrorCode errCode = OH_ImageSourceNative_CreatePixelmap(
      taskInternal->GetImageSourceNative(), decodeOption, &pixelmap);
  OH_DecodingOptions_Release(decodeOption);

  if (errCode != IMAGE_SUCCESS || pixelmap == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
    return false;
  }

  taskInternal->product.imageDataList.push_back(
      std::make_shared<ImageCacheData>(pixelmap));
  return true;
}

bool ImageCacheDecodeUtil::DecodeAnimated(
    const std::shared_ptr<ImageCacheTaskInternal>& taskInternal) {
  // Always query the actual frame count from the image source to ensure our
  // buffer is large enough. The header-reported count (fileTypeInfo) may
  // differ from the actual count after full parsing.
  uint32_t frameCount = 0;
  if (OH_ImageSourceNative_GetFrameCount(
          taskInternal->GetImageSourceNative(), &frameCount) != IMAGE_SUCCESS ||
      frameCount <= 1) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
    return false;
  }
  if (frameCount > MAX_BATCH_FRAME_COUNT) {
    DLOG(ERROR) << "DecodeAnimated: frameCount=" << frameCount
                << " exceeds limit " << MAX_BATCH_FRAME_COUNT
                << " task=" << taskInternal->GetTaskInfo();
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
    return false;
  }

  OH_DecodingOptions* decodeOption =
      CreateDecodeOptions(taskInternal->GetDesiredImageSize());
  if (decodeOption == nullptr) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::CREATE_DECODE_OPTION_FAILED);
    return false;
  }

  auto pixelmapList = std::make_unique<OH_PixelmapNative*[]>(frameCount);
  (void)memset(pixelmapList.get(), 0, sizeof(OH_PixelmapNative*) * frameCount);
  size_t outSize = frameCount;
  Image_ErrorCode errCode = OH_ImageSourceNative_CreatePixelmapList(
      taskInternal->GetImageSourceNative(),
      decodeOption,
      pixelmapList.get(),
      outSize);
  OH_DecodingOptions_Release(decodeOption);

  if (errCode != IMAGE_SUCCESS || outSize == 0 || outSize > frameCount) {
    ReleasePixelmapList(pixelmapList.get(), frameCount);
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
    return false;
  }

  frameCount = static_cast<uint32_t>(outSize);
  auto delayTimeList = std::make_unique<int32_t[]>(frameCount);
  for (uint32_t i = 0; i < frameCount; i++) {
    delayTimeList[i] = kDefaultFrameDelayMs;
  }

  errCode = OH_ImageSourceNative_GetDelayTimeList(
      taskInternal->GetImageSourceNative(), delayTimeList.get(), frameCount);
  if (errCode != IMAGE_SUCCESS) {
    LOG(WARNING) << taskInternal->GetTaskInfo()
                 << " GetDelayTimeList failed: " << errCode
                 << ", use default delay";
  }

  for (uint32_t i = 0; i < frameCount; i++) {
    if (pixelmapList[i] == nullptr) {
      ReleasePixelmapList(pixelmapList.get(), frameCount);
      TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
      return false;
    }
  }

  taskInternal->product.imageDataList.push_back(
      std::make_shared<ImageCacheData>(
          std::move(pixelmapList), std::move(delayTimeList), frameCount));
  return true;
}

bool ImageCacheDecodeUtil::Decode(
    const std::shared_ptr<ImageCacheTaskInternal>& taskInternal) {
  IMAGE_CACHE_CHECK_RETURN(taskInternal == nullptr, false);
  if (taskInternal->GetImageSourceNative() == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::IMAGE_SOURCE_NATIVE_NULL);
    return false;
  }

  if (taskInternal->IsAnimation()) {
    return DecodeAnimated(taskInternal);
  }
  return DecodeStatic(taskInternal);
}

std::shared_ptr<ImageCacheData> ImageCacheDecodeUtil::DecodeFrame(
    const std::shared_ptr<ImageCacheTaskInternal>& taskInternal,
    int frameIndex) {
  IMAGE_CACHE_CHECK_RETURN(taskInternal == nullptr, nullptr);

  OH_ImageSourceNative* source = taskInternal->GetImageSourceNative();
  if (source == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::IMAGE_SOURCE_NATIVE_NULL);
    return nullptr;
  }

  uint32_t frameCount = 0;
  if (OH_ImageSourceNative_GetFrameCount(source, &frameCount) !=
          IMAGE_SUCCESS ||
      frameCount == 0) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
    return nullptr;
  }
  if (frameIndex < 0 || static_cast<uint32_t>(frameIndex) >= frameCount) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
    return nullptr;
  }

  OH_DecodingOptions* decodeOption =
      CreateDecodeOptions(taskInternal->GetDesiredImageSize());
  if (decodeOption == nullptr) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::CREATE_DECODE_OPTION_FAILED);
    return nullptr;
  }

  OH_DecodingOptions_SetIndex(decodeOption, frameIndex);

  OH_PixelmapNative* pixelmap = nullptr;
  Image_ErrorCode errCode =
      OH_ImageSourceNative_CreatePixelmap(source, decodeOption, &pixelmap);
  OH_DecodingOptions_Release(decodeOption);
  if (errCode != IMAGE_SUCCESS || pixelmap == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
    return nullptr;
  }

  return std::make_shared<ImageCacheData>(pixelmap);
}

std::unique_ptr<int32_t[]> ImageCacheDecodeUtil::GetDelayTimeList(
    const std::shared_ptr<ImageCacheTaskInternal>& taskInternal,
    uint32_t& frameCount) {
  if (OH_ImageSourceNative_GetFrameCount(
          taskInternal->GetImageSourceNative(), &frameCount) != IMAGE_SUCCESS ||
      frameCount <= 1) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
    return nullptr;
  }
  if (frameCount > MAX_BATCH_FRAME_COUNT) {
    TASK_SET_ERROR(
        taskInternal, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
    return nullptr;
  }

  auto delayTimeList = std::make_unique<int32_t[]>(frameCount);
  for (uint32_t i = 0; i < frameCount; i++) {
    delayTimeList[i] = kDefaultFrameDelayMs;
  }

  auto errCode = OH_ImageSourceNative_GetDelayTimeList(
      taskInternal->GetImageSourceNative(), delayTimeList.get(), frameCount);
  if (errCode != IMAGE_SUCCESS) {
    LOG(WARNING) << taskInternal->GetTaskInfo()
                 << " GetDelayTimeList failed: " << errCode
                 << ", use default delay";
  }
  return delayTimeList;
}
} // namespace ImageCachePro
