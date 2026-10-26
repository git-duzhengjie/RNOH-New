/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#include "HermesMemoryWaterMark.h"

#include <atomic>

namespace rnoh {

namespace {

std::atomic<int64_t> g_hermesMemoryWaterMarkMB{
    DEFAULT_HERMES_MEMORY_WATER_MARK_MB};
std::atomic<bool> g_hasLoggedHermesWaterMarkExceededForProcessLifetime{false};

} // namespace

void setHermesMemoryWaterMarkMB(int64_t hermesMemoryWaterMarkMB) {
  g_hermesMemoryWaterMarkMB.store(
      hermesMemoryWaterMarkMB, std::memory_order_release);
}

int64_t getHermesMemoryWaterMarkMB() {
  return g_hermesMemoryWaterMarkMB.load(std::memory_order_acquire);
}

int64_t getHermesMemoryWaterMarkBytes() {
  return getHermesMemoryWaterMarkMB() * BYTES_PER_MB;
}

bool tryMarkHermesWaterMarkExceededForProcessLifetime() {
  bool expected = false;
  return g_hasLoggedHermesWaterMarkExceededForProcessLifetime
      .compare_exchange_strong(
          expected, true, std::memory_order_acq_rel, std::memory_order_acquire);
}

const std::string& getHermesFrameworkVersion() {
  static const std::string frameworkVersion = RNOH_FRAMEWORK_VERSION;
  return frameworkVersion;
}

} // namespace rnoh
