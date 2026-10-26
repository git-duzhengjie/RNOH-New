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

#ifndef IMAGE_CACHE_MEMORY_INTERCEPTOR_DEFAULT_H
#define IMAGE_CACHE_MEMORY_INTERCEPTOR_DEFAULT_H

#include "ImageCacheData.h"
#include "ImageCacheDataCacheInternal.h"
#include "ImageCacheInterceptor.h"
#include "ImageCacheLog.h"
#include "ImageCacheMemoryCache.h"
#include "ImageCachePixelmapUtils.h"

namespace ImageCachePro {

class ImageCacheMemoryInterceptorDefault : public ImageCacheMemoryInterceptor {
 public:
  explicit ImageCacheMemoryInterceptorDefault(
      ImageCacheManagerInternal& manager)
      : ImageCacheMemoryInterceptor(manager) {
    name = "Default MemoryCacheInterceptor";
  }

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override {
    IMAGE_CACHE_CHECK_RETURN(task == nullptr, false);
    if (task->cacheType == ImageCacheTaskType::WRITE) {
      return Write(task);
    }
    if (task->cacheType == ImageCacheTaskType::READ) {
      return Read(task);
    }
    return false;
  }

 private:
  bool Read(const std::shared_ptr<ImageCacheTask>& task) {
    if (task->cacheKey.empty()) {
      return false;
    }
    auto cached = ImageCacheMemoryCache::GetInstance()->Get(task->cacheKey);
    if (cached == nullptr) {
      return false;
    }
    auto internal =
        std::dynamic_pointer_cast<ImageCacheDataCacheInternal>(cached);
    if (internal == nullptr) {
      return false;
    }
    auto imageData = internal->GetImageData();
    if (imageData) {
      task->product.imageDataList.push_back(imageData);
    }
    task->product.frameDecodeContext = internal->GetFrameDecodeContext();
    return true;
  }

  bool Write(const std::shared_ptr<ImageCacheTask>& task) {
    if (task->cacheKey.empty()) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::CACHE_EMPTY_MEMORY_KEY);
      return false;
    }
    auto imageData = task->product.imageDataList.empty()
        ? nullptr
        : task->product.imageDataList.front();
    size_t cacheSize = 0;
    if (imageData) {
      const auto& fileTypeInfo = task->GetFileTypeInfo();
      cacheSize = ImageCachePixelmapUtils::GetFrameSize(fileTypeInfo);
      cacheSize *= imageData->GetFrameCount();
    }
    if (cacheSize > GetMemoryCacheMaxEntrySize()) {
      DLOG(ERROR) << "ImageCacheMemoryInterceptorDefault put cache to memory: "
                  << task->cacheKey << " data size: " << cacheSize
                  << " is too large";
      return true;
    }

    auto dataCache = std::make_shared<ImageCacheDataCacheInternal>(
        cacheSize, imageData, task->product.frameDecodeContext);
    if (dataCache == nullptr) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::CACHE_CREATE_FAILED);
      return false;
    }
    ImageCacheMemoryCache::GetInstance()->Put(task->cacheKey, dataCache);
    return true;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_MEMORY_INTERCEPTOR_DEFAULT_H
