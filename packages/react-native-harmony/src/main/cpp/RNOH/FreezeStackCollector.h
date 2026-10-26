/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#if USE_HERMES
#include <hermes/hermes.h>
#endif

namespace rnoh {

class FreezeStackCollector {
 public:
  static FreezeStackCollector& getInstance();

  struct RuntimeInfo {
    size_t rnInstanceId{0};
    void* hermesRuntime{nullptr};
  };

  struct CacheResult {
    std::string stack;
    int64_t ageMs{-1};
  };

  void registerRuntime(RuntimeInfo info);
  void unregisterRuntime(size_t rnInstanceId);

  void cacheStackResult(size_t rnInstanceId, const std::string& stack);
  CacheResult getCachedStackWithAge(size_t rnInstanceId) const;
  std::string getCachedStack(size_t rnInstanceId) const;
  std::string captureStackFromProfiler(size_t rnInstanceId) const;

 private:
  FreezeStackCollector() = default;
  ~FreezeStackCollector() = default;
  FreezeStackCollector(const FreezeStackCollector&) = delete;
  FreezeStackCollector& operator=(const FreezeStackCollector&) = delete;

  struct CacheEntry {
    std::string stack;
    std::chrono::steady_clock::time_point capturedAt{};
    bool hasValue{false};
  };

  void clearCache(size_t rnInstanceId);

  mutable std::mutex m_runtimeMutex;
  std::unordered_map<size_t, RuntimeInfo> m_runtimeInfos;

  mutable std::mutex m_cacheMutex;
  std::unordered_map<size_t, CacheEntry> m_cacheByInstance;

#if USE_HERMES
  static std::string formatProfilerSample(
      const facebook::hermes::sampling_profiler::ProfileSample& sample);
#endif
};

} // namespace rnoh
