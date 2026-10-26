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

#ifndef IMAGE_CACHE_METRICS_H
#define IMAGE_CACHE_METRICS_H

#include <glog/logging.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

namespace ImageCachePro {

/**
 * @brief Thread-safe object instance counter for ImageCache classes.
 *
 * Each dynamically-allocated class increments its counter in the constructor
 * and decrements it in the destructor. Logs include the keyword
 * "&&&&&&&&" for easy grep.
 *
 * Owned by ImageCacheMetrics; access via the process-wide singleton.
 *
 * Usage (in a class's constructor):
 *   ImageCacheMetrics::Instance().Objects().Increment("ImageCacheManagerInternal",
 * this);
 *
 * Usage (in a class's destructor):
 *   ImageCacheMetrics::Instance().Objects().Decrement("ImageCacheManagerInternal",
 * this);
 *
 * Query all counts:
 *   auto counts = ImageCacheMetrics::Instance().Objects().GetAllCounts();
 */
class ImageCacheObjectCounter {
 public:
  ImageCacheObjectCounter() = default;

  void Increment(const std::string& className, const void* ptr) {
    auto& counter = GetCounter(className);
    counter.fetch_add(1, std::memory_order_relaxed);
  }

  void Decrement(const std::string& className, const void* ptr) {
    auto& counter = GetCounter(className);
    counter.fetch_sub(1, std::memory_order_relaxed);
  }

  /**
   * @brief Return a snapshot of all class counts.
   * Key = class name, value = current live instance count.
   */
  std::map<std::string, int64_t> GetAllCounts() const {
    std::map<std::string, int64_t> result;
    std::lock_guard<std::mutex> lock(mapMutex_);
    for (const auto& kv : counters_) {
      result[kv.first] = kv.second.load(std::memory_order_relaxed);
    }
    return result;
  }

  /**
   * @brief Return the count for a specific class, or 0 if not tracked.
   */
  int64_t GetCount(const std::string& className) const {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = counters_.find(className);
    if (it != counters_.end()) {
      return it->second.load(std::memory_order_relaxed);
    }
    return 0;
  }

  // Non-copyable, non-movable
  ImageCacheObjectCounter(const ImageCacheObjectCounter&) = delete;
  ImageCacheObjectCounter& operator=(const ImageCacheObjectCounter&) = delete;

 private:
  std::atomic<int64_t>& GetCounter(const std::string& className) {
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = counters_.find(className);
    if (it == counters_.end()) {
      // operator[] value-initializes std::atomic<int64_t> to 0; avoids
      // copy-construction which is deleted for std::atomic.
      return counters_[className];
    }
    return it->second;
  }

  mutable std::mutex mapMutex_;
  std::map<std::string, std::atomic<int64_t>> counters_;
};

// Identifies a single monotonic counter tracked by the dispatcher.
enum class MetricsCounter : uint32_t {
  TotalRequest = 0,
  TotalSubmitted,
  TotalCompleted,
  TotalCancelled,
  TotalMerged,
  TotalFailed,
  CacheHits,
  CacheMisses,
  // -- LoadImageSync synchronous path (independent of async counters) --
  SyncLoadHits,
  SyncLoadMisses,
  // -- Download statistics --
  DownloadsInitiated,
  LocalFileHits,
  DownloadsSucceeded,
  DownloadsFailed,
  Count,
};

inline constexpr size_t kMetricsCounterCount =
    static_cast<size_t>(MetricsCounter::Count);

// Snapshot of all counters + live object counts at a point in time. Copyable.
struct ImageCacheMetricsSnapshot {
  uint64_t values[kMetricsCounterCount] = {};
  std::map<std::string, int64_t> objects;

  uint64_t Get(MetricsCounter kind) const {
    return values[static_cast<size_t>(kind)];
  }
};

// All dispatcher counters behind a single array + unified accessors.
// Process-wide singleton: aggregate across all Dispatcher instances.
class ImageCacheMetrics {
 public:
  static ImageCacheMetrics& Instance() {
    // NoDestroy (intentional leak): this is a pure in-memory statistics object
    // (atomics + map + mutex, no system resources). It must outlive every
    // static/singleton whose destructor calls Objects().Decrement(); otherwise
    // reverse-order static destruction (SIOF) accesses this singleton's
    // already-destroyed mutex -> system_error(EINVAL) thrown from a
    // destructor -> std::terminate -> SIGABRT. Do NOT revert to a destructed
    // static.
    static ImageCacheMetrics* instance = new ImageCacheMetrics();
    return *instance;
  }

  ImageCacheMetrics() {
    for (size_t i = 0; i < kMetricsCounterCount; ++i) {
      values_[i].store(0, std::memory_order_relaxed);
    }
  }

  // Counters: monotonic increment.
  void Increment(MetricsCounter kind, uint64_t n = 1) {
    values_[Index(kind)].fetch_add(n, std::memory_order_relaxed);
  }
  uint64_t Load(MetricsCounter kind) const {
    return values_[Index(kind)].load(std::memory_order_relaxed);
  }

  ImageCacheMetricsSnapshot Snapshot() const {
    ImageCacheMetricsSnapshot s;
    for (size_t i = 0; i < kMetricsCounterCount; ++i) {
      s.values[i] = values_[i].load(std::memory_order_relaxed);
    }
    s.objects = objects_.GetAllCounts();
    return s;
  }

  void Reset() {
    for (size_t i = 0; i < kMetricsCounterCount; ++i) {
      values_[i].store(0, std::memory_order_relaxed);
    }
  }

  // Live object instance counts, keyed by class name.
  ImageCacheObjectCounter& Objects() {
    return objects_;
  }

 private:
  static size_t Index(MetricsCounter kind) {
    return static_cast<size_t>(kind);
  }

  std::atomic<uint64_t> values_[kMetricsCounterCount];
  ImageCacheObjectCounter objects_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DISPATCHER_METRICS_H
