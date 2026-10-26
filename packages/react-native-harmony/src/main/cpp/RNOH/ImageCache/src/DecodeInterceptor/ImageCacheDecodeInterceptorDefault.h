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

#ifndef IMAGE_CACHE_DECODE_INTERCEPTOR_DEFAULT_H
#define IMAGE_CACHE_DECODE_INTERCEPTOR_DEFAULT_H

#include <multimedia/image_framework/image/image_source_native.h>
#include "ImageCacheDecodeUtil.h"
#include "ImageCacheInterceptor.h"
#include "ImageCacheLog.h"
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

class ImageCacheDecodeInterceptorDefault : public ImageCacheDecodeInterceptor {
 public:
  explicit ImageCacheDecodeInterceptorDefault(
      ImageCacheManagerInternal& manager)
      : ImageCacheDecodeInterceptor(manager) {
    name = "Default DecodeInterceptor";
  }

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override {
    const auto& fileTypeInfo = task->GetFileTypeInfo();
    if (fileTypeInfo.format == ImageCacheFormat::UNKNOWN) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::UNKNOWN_FILE_TYPE);
      return false;
    }

    auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
    IMAGE_CACHE_CHECK_RETURN(taskInternal == nullptr, false);

    if (taskInternal->GetImageSourceNative() == nullptr) {
      if (task->product.imageBuffer == nullptr ||
          task->product.imageLength == 0) {
        TASK_SET_ERROR(task, ImageCacheErrorCode::EMPTY_IMAGE_BUFFER);
        return false;
      }
      OH_ImageSourceNative* rawSource = nullptr;
      Image_ErrorCode errCode = OH_ImageSourceNative_CreateFromData(
          task->product.imageBuffer.get(),
          task->product.imageLength,
          &rawSource);
      if (errCode != IMAGE_SUCCESS || rawSource == nullptr) {
        TASK_SET_ERROR(task, ImageCacheErrorCode::DECODE_FAILED);
        return false;
      }
      taskInternal->SetImageSourceNative(rawSource);
    }

    // LAZY_DECODE: parse header only, skip pixel decode
    if (taskInternal->GetDecodeMode() ==
        ImageCacheTaskInternal::DecodeMode::LAZY_DECODE) {
      return ImageCacheDecodeUtil::ParseLazyHeader(taskInternal);
    }

    // frame decode
    if (taskInternal->IsFrameDecodeMode()) {
      auto job = taskInternal->GetJob();
      int startFrame = 0;
      int fetchCount = 1;
      int frameCount = 1;
      if (job) {
        auto& req = job->GetDecodeRequest();
        startFrame = req.startFrame;
        fetchCount = req.fetchCount;
        if (req.frameContext) {
          frameCount = req.frameContext->frameCount_;
        }
      }
      if (frameCount == 0) {
        TASK_SET_ERROR(
            task, ImageCacheErrorCode::ANIMATED_DECODE_INVALID_FRAME_COUNT);
        return false;
      }
      for (auto index = 0; index < fetchCount; index++) {
        auto data = ImageCacheDecodeUtil::DecodeFrame(
            taskInternal, startFrame % frameCount);
        if (data) {
          taskInternal->product.imageDataList.push_back(data);
        } else {
          taskInternal->product.imageDataList.clear();
          return false;
        }
        startFrame++;
      }
      return ImageCacheDecodeUtil::ParseLazyHeader(taskInternal);
    }
    // normal decode
    return ImageCacheDecodeUtil::Decode(taskInternal);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DECODE_INTERCEPTOR_DEFAULT_H
