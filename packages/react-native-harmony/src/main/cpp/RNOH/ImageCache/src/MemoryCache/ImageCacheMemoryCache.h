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

#ifndef IMAGE_CACHE_MEMORY_CACHE_H
#define IMAGE_CACHE_MEMORY_CACHE_H

#include <mutex>
#include <string>
#include "ImageCacheData.h"
#include "ImageCacheDataCacheInternal.h"
#include "LRUCache11.h"

namespace ImageCachePro {

class ImageCacheMemoryCache {
 public:
  static ImageCacheMemoryCache* GetInstance();

  ImageCacheMemoryCache(const ImageCacheMemoryCache&) = delete;
  ImageCacheMemoryCache& operator=(const ImageCacheMemoryCache&) = delete;

  size_t Size();
  void Put(const std::string& key, std::shared_ptr<ImageCacheDataCacheInternal> value);
  std::shared_ptr<ImageCacheDataCacheInternal> Get(const std::string& key);
  bool Contains(const std::string& key) const;
  bool Remove(const std::string& key);
  void Clear();
  void SetCacheLimit(size_t maxSize, size_t maxUsage);

  size_t GetAllowMaxMemory() const;
  size_t GetAllowMaxSizeLimit() const;
  size_t GetCurrentMemory() const;

  void ClearHalf();

 private:
  ImageCacheMemoryCache();
  ~ImageCacheMemoryCache();

  bool RemoveTailEntry();

  mutable std::mutex memoryMutex_;
  size_t allowMaxSize_ = 0;
  size_t allowMaxMemory_ = 0;
  size_t currentMemory_ = 0;
  lru11::
      Cache<std::string, std::shared_ptr<ImageCacheDataCacheInternal>, lru11::NullLock>
          cache_;
};

// Runtime threshold (1/4 of memory cache capacity)
size_t GetMemoryCacheMaxEntrySize();

} // namespace ImageCachePro

#endif // IMAGE_CACHE_MEMORY_CACHE_H
