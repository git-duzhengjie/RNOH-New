/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IMAGE_CACHE_PREFETCH_BUFFER_H
#define IMAGE_CACHE_PREFETCH_BUFFER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "../../include/ImageCacheData.h"

namespace ImageCachePro {

class FramePrefetchBuffer {
 public:
  explicit FramePrefetchBuffer(uint32_t prefetchAhead) : prefetchAhead_(prefetchAhead) {}

  void Put(uint32_t frameIndex, uint32_t frameCount, const std::vector<std::shared_ptr<ImageCacheData>>& frameList) {
    if (frameCount == 0) {
      return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t nextIdx = frameIndex;
    for (auto& frame : frameList) {
      if (frame) {
        uint32_t idx = nextIdx % frameCount;
        frames_[idx] = frame;
        nextIdx++;
      }
    }
  }

  std::shared_ptr<ImageCacheData> TryGet(uint32_t frameIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = frames_.find(frameIndex);
    if (it == frames_.end()) {
      return nullptr;
    }
    return it->second;
  }

  bool HasFrame(uint32_t frameIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.count(frameIndex) > 0;
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.clear();
  }

  size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.size();
  }

  uint32_t GetPrefetchAhead() const { return prefetchAhead_; }

 private:
  const uint32_t prefetchAhead_;
  mutable std::mutex mutex_;
  std::unordered_map<uint32_t, std::shared_ptr<ImageCacheData>> frames_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_PREFETCH_BUFFER_H
