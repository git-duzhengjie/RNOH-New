/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */
#pragma once
#include <glog/logging.h>
#include <atomic>
#include <mutex>
#include <string>
#include "ApiVersionCheck.h"

namespace rnoh {

class RNInstanceCAPI;

// Atomic flag so that FFRT worker threads (readers) and the JS thread (writer)
// never race on the switch — a plain bool would be a data race (UB).
extern std::atomic<bool> PARALLEL_RUNTIME_SWITCH;

inline bool GetParallelizationEnabled() {
  return PARALLEL_RUNTIME_SWITCH.load(std::memory_order_acquire);
}

// 'phone' | 'tablet' | 'pc' | "isFoldableDevice"
// Returns by value under a lock; std::string cannot be read/written atomically
// and concurrent access would corrupt the heap-allocated buffer.
std::string deviceType();

void SetParallelizationEnabled(bool enabled);

void setDeviceInfo(std::string deviceType);

inline bool IsParallelizationEnabled() {
#ifdef PARALLELIZATION_ON
  return IsAtLeastApi22() && GetParallelizationEnabled();
#else
  return false;
#endif
}

} // namespace rnoh
