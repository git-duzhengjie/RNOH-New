/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "HiCollieFreezeHandler.h"

#include <dlfcn.h>
#include <glog/logging.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ApiVersionCheck.h"
#include "RNInstanceInternal.h"

namespace rnoh {

namespace {

constexpr size_t MAX_FREEZE_STACK_BYTES = 64 * 1024;
constexpr std::string_view FREEZE_STACK_TRUNCATED_SUFFIX =
    "\n...[truncated by RNOH freeze handler]\n";
constexpr int OH_THREAD_BLOCK_3S = 0;
constexpr int OH_THREAD_BLOCK_6S = 1;

using FreezeLogCallback = size_t (*)(int type, void* buffer, size_t size);
using OH_HiCollie_SetFreezeCallbackFunc = void* (*)(FreezeLogCallback callback);

std::mutex g_instancesMtx;
std::unordered_map<size_t, std::weak_ptr<RNInstanceInternal>> g_instancesById;
std::optional<size_t> g_latestRegisteredInstanceId;
std::once_flag g_registerFreezeCallbackOnce;
std::mutex g_lastFreezeStackMtx;
std::string g_lastFreezeStack;
std::chrono::steady_clock::time_point g_lastFreezeStackCapturedAt{};

constexpr auto LAST_FREEZE_STACK_REUSE_WINDOW = std::chrono::seconds(10);

std::vector<std::shared_ptr<RNInstanceInternal>> getActiveRNInstances() {
  std::vector<std::shared_ptr<RNInstanceInternal>> instances;
  std::optional<size_t> latestRegisteredInstanceId;
  auto lock = std::lock_guard(g_instancesMtx);
  latestRegisteredInstanceId = g_latestRegisteredInstanceId;
  if (g_latestRegisteredInstanceId.has_value()) {
    auto it = g_instancesById.find(*g_latestRegisteredInstanceId);
    if (it != g_instancesById.end()) {
      auto instance = it->second.lock();
      if (instance) {
        instances.push_back(instance);
      } else {
        g_instancesById.erase(it);
        g_latestRegisteredInstanceId.reset();
      }
    }
  }

  for (auto it = g_instancesById.begin(); it != g_instancesById.end();) {
    if (g_latestRegisteredInstanceId.has_value() &&
        it->first == *g_latestRegisteredInstanceId) {
      ++it;
      continue;
    }

    auto instance = it->second.lock();
    if (!instance) {
      it = g_instancesById.erase(it);
      continue;
    }

    instances.push_back(instance);
    ++it;
  }

  std::sort(
      instances.begin(), instances.end(), [](const auto& lhs, const auto& rhs) {
        return lhs->getId() < rhs->getId();
      });

  if (latestRegisteredInstanceId.has_value()) {
    auto latestIt = std::find_if(
        instances.begin(), instances.end(), [&](const auto& instance) {
          return instance != nullptr &&
              instance->getId() == *latestRegisteredInstanceId;
        });
    if (latestIt != instances.end()) {
      auto latestInstance = std::move(*latestIt);
      instances.erase(latestIt);
      instances.insert(instances.begin(), std::move(latestInstance));
    }
  }

  return instances;
}

OH_HiCollie_SetFreezeCallbackFunc resolveFreezeCallbackRegistration() {
  auto* handle = dlopen("libohhicollie.so", RTLD_NOW | RTLD_GLOBAL);
  if (handle == nullptr) {
    return nullptr;
  }

  auto* symbol = dlsym(handle, "OH_HiCollie_SetFreezeCallback");
  if (symbol == nullptr) {
    return nullptr;
  }

  return reinterpret_cast<OH_HiCollie_SetFreezeCallbackFunc>(symbol);
}

bool isUiThreadFreezeType(int type) {
  return type == OH_THREAD_BLOCK_3S || type == OH_THREAD_BLOCK_6S;
}

std::string fitStackToLimit(
    std::string_view stack,
    size_t maxBytes,
    std::string_view truncatedSuffix) {
  if (maxBytes == 0) {
    return "";
  }

  if (stack.size() <= maxBytes) {
    return std::string(stack);
  }

  const auto suffixSize = truncatedSuffix.size();
  const auto prefixSize =
      maxBytes > suffixSize ? maxBytes - suffixSize : static_cast<size_t>(0);

  std::string fitted(stack.substr(0, prefixSize));
  if (suffixSize > 0) {
    fitted.append(truncatedSuffix.data(), suffixSize);
  }
  return fitted;
}

void clearLastFreezeStack() {
  auto lock = std::lock_guard(g_lastFreezeStackMtx);
  g_lastFreezeStack.clear();
  g_lastFreezeStackCapturedAt = {};
}

void updateLastFreezeStack(const std::string& stack) {
  auto lock = std::lock_guard(g_lastFreezeStackMtx);
  g_lastFreezeStack = stack;
  g_lastFreezeStackCapturedAt = std::chrono::steady_clock::now();
}

std::string getReusableFreezeStack() {
  auto lock = std::lock_guard(g_lastFreezeStackMtx);
  if (g_lastFreezeStack.empty()) {
    return "";
  }

  if (std::chrono::steady_clock::now() - g_lastFreezeStackCapturedAt >
      LAST_FREEZE_STACK_REUSE_WINDOW) {
    return "";
  }

  return g_lastFreezeStack;
}

std::string collectStacksFromInstances(
    const std::vector<std::shared_ptr<RNInstanceInternal>>& instances) {
  std::vector<std::pair<size_t, std::string>> capturedStacks;
  capturedStacks.reserve(instances.size());

  for (const auto& instance : instances) {
    if (!instance) {
      continue;
    }

    auto stack = instance->captureCurrentJSStackSync();
    if (stack.empty()) {
      continue;
    }

    capturedStacks.emplace_back(instance->getId(), std::move(stack));
  }

  if (capturedStacks.empty()) {
    return "";
  }

  if (capturedStacks.size() == 1) {
    return std::move(capturedStacks.front().second);
  }

  std::string combinedStack;
  for (const auto& [rnInstanceId, stack] : capturedStacks) {
    if (!combinedStack.empty() && combinedStack.back() != '\n') {
      combinedStack.push_back('\n');
    }
    combinedStack.append("[RNInstance ");
    combinedStack.append(std::to_string(rnInstanceId));
    combinedStack.append("]\n");
    combinedStack.append(stack);
  }

  return combinedStack;
}

size_t freezeLogCallback(int type, void* buffer, size_t size) {
  if (buffer == nullptr || size == 0 || !isUiThreadFreezeType(type)) {
    return 0;
  }

  std::string stackRecord;
  if (type == OH_THREAD_BLOCK_3S) {
    clearLastFreezeStack();
  } else if (type == OH_THREAD_BLOCK_6S) {
    stackRecord = getReusableFreezeStack();
  }

  if (stackRecord.empty()) {
    stackRecord = collectStacksFromInstances(getActiveRNInstances());
  }

  if (type == OH_THREAD_BLOCK_3S && !stackRecord.empty()) {
    updateLastFreezeStack(stackRecord);
  }

  const auto capturedBytes = stackRecord.size();
  auto fittedStackRecord = fitStackToLimit(
      stackRecord, MAX_FREEZE_STACK_BYTES, FREEZE_STACK_TRUNCATED_SUFFIX);
  const auto bytesToWrite = std::min(fittedStackRecord.size(), size);

  if (bytesToWrite > 0) {
    std::memcpy(buffer, fittedStackRecord.data(), bytesToWrite);
  }
  if (bytesToWrite < size) {
    static_cast<char*>(buffer)[bytesToWrite] = '\0';
  }

  LOG(INFO) << "[FreezeStack] JS stack captured for freeze callback: bytes="
            << capturedBytes << ", writtenBytes=" << bytesToWrite;
  return bytesToWrite;
}

} // namespace

void HiCollieFreezeHandler::registerInstance(
    const std::shared_ptr<RNInstanceInternal>& instance) {
  if (!instance) {
    return;
  }

  {
    auto lock = std::lock_guard(g_instancesMtx);
    g_instancesById[instance->getId()] = instance;
    g_latestRegisteredInstanceId = instance->getId();
  }

  clearLastFreezeStack();
  ensureFreezeCallbackRegisteredOnce();
}

void HiCollieFreezeHandler::unregisterInstance(size_t rnInstanceId) {
  {
    auto lock = std::lock_guard(g_instancesMtx);
    g_instancesById.erase(rnInstanceId);
    if (g_latestRegisteredInstanceId == rnInstanceId) {
      g_latestRegisteredInstanceId.reset();
    }
  }
  clearLastFreezeStack();
}

void HiCollieFreezeHandler::markInstanceDestroyed(size_t rnInstanceId) {
  unregisterInstance(rnInstanceId);
}

void HiCollieFreezeHandler::ensureFreezeCallbackRegisteredOnce() {
  std::call_once(g_registerFreezeCallbackOnce, []() {
    if (!IsAtLeastApi24()) {
      return;
    }

    auto setFreezeCallback = resolveFreezeCallbackRegistration();
    if (setFreezeCallback == nullptr) {
      return;
    }

    setFreezeCallback(&freezeLogCallback);
  });
}

} // namespace rnoh
