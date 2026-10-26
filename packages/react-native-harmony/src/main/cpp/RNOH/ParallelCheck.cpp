/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */
#include "RNOH/ParallelCheck.h"
#include <mutex>
#include "RNOH/RNInstance.h"
#include "RNOH/RNInstanceCAPI.h"
#include "RNOH/arkui/ArkUINodeRegistry.h"
#include "RNOH/arkui/NativeNodeApi.h"

namespace rnoh {

std::atomic<bool> PARALLEL_RUNTIME_SWITCH{false};

namespace {
// Guards DEVICE_TYPE — std::string cannot be made atomic, so all reads/writes
// go through this mutex to avoid torn reads and heap corruption.
std::mutex g_deviceTypeMutex;
std::string DEVICE_TYPE = "phone";
} // namespace

std::string deviceType() {
  std::lock_guard<std::mutex> lock(g_deviceTypeMutex);
  return DEVICE_TYPE;
}

void SetParallelizationEnabled(bool enabled) {
  bool oldValue = PARALLEL_RUNTIME_SWITCH.exchange(
      enabled, std::memory_order_acq_rel);

  // If the switch value changed and NativeNodeApi has been initialized,
  // reset the instance so it will be re-initialized with the new switch value
  if (oldValue != enabled) {
    NativeNodeApi::resetInstance();

    // Re-register the node event receiver if ArkUINodeRegistry has been
    // initialized This is necessary because the API instance has changed
    if (ArkUINodeRegistry::isInitialized()) {
      ArkUINodeRegistry::getInstance().reinitializeNodeEventReceiver();
    } else {
      LOG(INFO)
          << "SetParallelizationEnabled: ArkUINodeRegistry not initialized yet, skipping re-registration";
    }
  }
}

void setDeviceInfo(std::string newDeviceType) {
  std::lock_guard<std::mutex> lock(g_deviceTypeMutex);
  DEVICE_TYPE = std::move(newDeviceType);
}

} // namespace rnoh
