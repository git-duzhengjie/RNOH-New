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

#include "ImageCacheMemoryCache.h"
#include <algorithm>
#include <mutex>
#include "ImageCacheLog.h"
#include "MemCacheCallback.h"
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
#include "transformation/gputransform/ImageCacheEGL.h"
#endif

namespace ImageCachePro {

static constexpr size_t DEFAULT_SIZE = 256;
static constexpr size_t DEFAULT_MEMORY = 128 * 1024 * 1024;

#define MEMORY_2GB 2
#define MEMORY_4GB 4
#define MEMORY_6GB 6

#define CACHE_LIMIT_16MB (16 * 1024 * 1024)
#define CACHE_LIMIT_32MB (32 * 1024 * 1024)
#define CACHE_LIMIT_64MB (64 * 1024 * 1024)
#define CACHE_LIMIT_128MB (128 * 1024 * 1024)

static size_t CalculateMaxCacheSize() {
  long physPages = sysconf(_SC_PHYS_PAGES);
  long pageSize = sysconf(_SC_PAGESIZE);

  if (physPages <= 0 || pageSize <= 0) {
    return CACHE_LIMIT_128MB;
  }

  size_t totalMemory =
      static_cast<size_t>(physPages) * static_cast<size_t>(pageSize);
  size_t totalMemoryGB = totalMemory / (1024 * 1024 * 1024);

  size_t maxCacheSize = 0;
  if (totalMemoryGB <= MEMORY_2GB) {
    maxCacheSize = CACHE_LIMIT_16MB;
  } else if (totalMemoryGB <= MEMORY_4GB) {
    maxCacheSize = CACHE_LIMIT_32MB;
  } else if (totalMemoryGB <= MEMORY_6GB) {
    maxCacheSize = CACHE_LIMIT_64MB;
  } else {
    maxCacheSize = CACHE_LIMIT_128MB;
  }

  DLOG(INFO) << "[IMAGE] Total memory: " << totalMemoryGB
             << "GB, max cache size: " << (maxCacheSize / (1024 * 1024))
             << "MB";
  return maxCacheSize;
}
ImageCacheMemoryCache* ImageCacheMemoryCache::GetInstance() {
  static ImageCacheMemoryCache cache;
  return &cache;
}

ImageCacheMemoryCache::ImageCacheMemoryCache()
    : allowMaxSize_(DEFAULT_SIZE),
      allowMaxMemory_(CalculateMaxCacheSize()),
      cache_(allowMaxSize_, 0) {
  #ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  ImageCacheEGL::InitShared();
#endif
  static std::once_flag registerOnce;
  std::call_once(registerOnce, [this]() {
    MemCacheCallback::registerCallback([this](int memoryLevel) {
      DLOG(INFO)
          << "[IMAGE] ImageCacheMemoryCache received memory callback, level="
          << memoryLevel;
      if (memoryLevel == MEMORY_LEVEL_CRITICAL) {
        this->Clear();
      } else if (
          memoryLevel == MEMORY_LEVEL_MODERATE ||
          memoryLevel == MEMORY_LEVEL_LOW) {
        this->ClearHalf();
      }
    });
  });
}

ImageCacheMemoryCache::~ImageCacheMemoryCache() {
  #ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  ImageCacheEGL::DestroyShared();
#endif
}

bool ImageCacheMemoryCache::RemoveTailEntry() {
  std::string keyOut;
  std::shared_ptr<ImageCacheDataCacheInternal> valueOut;
  if (!cache_.getTail(keyOut, valueOut)) {
    return false;
  }
  cache_.remove(keyOut);
  if (valueOut != nullptr) {
    size_t entrySize = valueOut->GetImageCacheSize();
    if (entrySize <= currentMemory_) {
      currentMemory_ -= entrySize;
    } else {
      currentMemory_ = 0;
    }
  }
  return true;
}

void ImageCacheMemoryCache::Put(
    const std::string& key,
    std::shared_ptr<ImageCacheDataCacheInternal> value) {
  IMAGE_CACHE_CHECK(key.empty());
  IMAGE_CACHE_CHECK(value == nullptr);

  std::lock_guard<std::mutex> lock(memoryMutex_);
  // Use insert's return value to handle existing entry
  // insert returns true if key was newly inserted, false if updated existing
  std::shared_ptr<ImageCacheDataCacheInternal> existing;
  bool hadExisting = cache_.tryGet(key, existing);
  if (hadExisting && existing != nullptr) {
    size_t existingSize = existing->GetImageCacheSize();
    if (existingSize <= currentMemory_) {
      currentMemory_ -= existingSize;
    } else {
      currentMemory_ = 0;
    }
  }
  cache_.insert(key, value, true);
  currentMemory_ += value->GetImageCacheSize();

  while (currentMemory_ >= allowMaxMemory_ || cache_.size() >= allowMaxSize_) {
    if (!RemoveTailEntry()) {
      break;
    }
  }
}

size_t ImageCacheMemoryCache::Size() {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  return cache_.size();
}

bool ImageCacheMemoryCache::Remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  std::shared_ptr<ImageCacheDataCacheInternal> value;
  if (!cache_.tryGet(key, value)) {
    return true;
  }
  if (cache_.remove(key)) {
    if (value != nullptr) {
      size_t entrySize = value->GetImageCacheSize();
      if (entrySize <= currentMemory_) {
        currentMemory_ -= entrySize;
      } else {
        currentMemory_ = 0;
      }
    }
    return true;
  }
  return false;
}

void ImageCacheMemoryCache::Clear() {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  size_t cacheSize = cache_.size();
  cache_.clear();
  currentMemory_ = 0;
}

void ImageCacheMemoryCache::SetCacheLimit(size_t maxSize, size_t maxUsage) {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  constexpr size_t defaultMaxSize = 4096;
  constexpr size_t defaultMaxMemory = 1024 * 1024 * 1024;
  // Cap maxSize at defaultMaxSize; 0 means unbounded (LRU cache treats 0 as
  // no limit), so only apply the cap when maxSize > 0.
  allowMaxSize_ = (maxSize > 0) ? std::min(maxSize, defaultMaxSize) : 0;
  cache_.resize(allowMaxSize_);
  allowMaxMemory_ =
      (maxUsage > 0) ? std::min(maxUsage, defaultMaxMemory) : defaultMaxMemory;

  while (currentMemory_ >= allowMaxMemory_ || cache_.size() >= allowMaxSize_) {
    if (!RemoveTailEntry()) {
      break;
    }
  }
}

size_t ImageCacheMemoryCache::GetAllowMaxSizeLimit() const {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  return allowMaxSize_;
}

std::shared_ptr<ImageCacheDataCacheInternal> ImageCacheMemoryCache::Get(
    const std::string& key) {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  std::shared_ptr<ImageCacheDataCacheInternal> value;
  if (cache_.tryGet(key, value)) {
    return value;
  }
  return nullptr;
}

bool ImageCacheMemoryCache::Contains(const std::string& key) const {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  return cache_.contains(key);
}

size_t ImageCacheMemoryCache::GetAllowMaxMemory() const {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  return allowMaxMemory_;
}

size_t ImageCacheMemoryCache::GetCurrentMemory() const {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  return currentMemory_;
}

void ImageCacheMemoryCache::ClearHalf() {
  std::lock_guard<std::mutex> lock(memoryMutex_);
  size_t currentSize = cache_.size();
  if (currentSize == 0) {
    return;
  }

  size_t removeCount = currentSize / 2;
  if (removeCount == 0) {
    removeCount = 1;
  }

  for (size_t i = 0; i < removeCount; i++) {
    if (!RemoveTailEntry()) {
      break;
    }
  }

  }

// Runtime threshold implementation (1/4 of memory cache capacity)
size_t GetMemoryCacheMaxEntrySize() {
  return ImageCacheMemoryCache::GetInstance()->GetAllowMaxMemory() / 4;
}

} // namespace ImageCachePro
