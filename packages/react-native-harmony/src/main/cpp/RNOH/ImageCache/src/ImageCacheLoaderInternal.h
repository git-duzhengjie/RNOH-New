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

#ifndef IMAGE_CACHE_LOADER_INTERNAL_H
#define IMAGE_CACHE_LOADER_INTERNAL_H

#include "ImageCacheLoader.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

class ImageCacheLoaderInternal : public ImageCacheLoader {
 public:
  ImageCacheLoaderInternal() {
    ImageCacheMetrics::Instance().Objects().Increment(
        "ImageCacheLoaderInternal", this);
  }
  ~ImageCacheLoaderInternal() override {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "ImageCacheLoaderInternal", this);
  }

  bool LoadFromMemory(const std::shared_ptr<ImageCacheTaskInternal>& task);
  bool WriteCacheToMemory(const std::shared_ptr<ImageCacheTaskInternal>& task);
  bool DownloadImage(const std::shared_ptr<ImageCacheTaskInternal>& task);
  bool DecodeImage(const std::shared_ptr<ImageCacheTaskInternal>& task);
  void CancelLoad(const std::shared_ptr<ImageCacheTaskInternal>& task);

  void AddMemoryCacheInterceptor(
      std::shared_ptr<ImageCacheMemoryInterceptor> interceptor,
      Position position = Position::START) override;
  void AddLoadInterceptor(
      std::shared_ptr<ImageCacheLoadInterceptor> interceptor,
      Position position = Position::START) override;
  void AddDecodeInterceptor(
      std::shared_ptr<ImageCacheDecodeInterceptor> interceptor,
      Position position = Position::START) override;

 private:
  std::shared_ptr<ImageCacheMemoryInterceptor> memoryHead_ = nullptr;
  std::shared_ptr<ImageCacheMemoryInterceptor> memoryTail_ = nullptr;
  std::shared_ptr<ImageCacheLoadInterceptor> loadHead_ = nullptr;
  std::shared_ptr<ImageCacheLoadInterceptor> loadTail_ = nullptr;
  std::shared_ptr<ImageCacheDecodeInterceptor> decodeHead_ = nullptr;
  std::shared_ptr<ImageCacheDecodeInterceptor> decodeTail_ = nullptr;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_LOADER_INTERNAL_H
