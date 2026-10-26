/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "DynamicArkUILoader.h"

namespace rnoh {

void* DynamicArkUILoader::loaderHandle = nullptr;
int32_t (*DynamicArkUILoader::OH_ArkUI_RunTaskInScopeFun)(
    ArkUI_ContextHandle,
    void*,
    void (*)(void*)) = nullptr;
ArkUI_TextChangeEvent* (
    *DynamicArkUILoader::OH_ArkUI_NodeEvent_GetTextChangeEventFun)(
    ArkUI_NodeEvent* event) = nullptr;
void (*DynamicArkUILoader::
          OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurationsFun)(
    ArkUI_DrawableDescriptor*,
    const int32_t*,
    int32_t) = nullptr;
bool DynamicArkUILoader::initialized = false;
bool DynamicArkUILoader::initializedAPI15 = false;
bool DynamicArkUILoader::initializedAPI22 = false;
std::atomic<int32_t> DynamicArkUILoader::refCount{0};
std::once_flag DynamicArkUILoader::loadFlag;
std::once_flag DynamicArkUILoader::initFlag;
std::once_flag DynamicArkUILoader::initFlagAPI15;
std::once_flag DynamicArkUILoader::initFlagAPI22;

bool DynamicArkUILoader::ensureLoaded() {
  std::call_once(loadFlag, []() {
    loaderHandle = dlopen("libace_ndk.z.so", RTLD_LAZY);
    if (!loaderHandle) {
      LOG(ERROR) << "Failed to load so: " << dlerror();
    }
  });
  if (loaderHandle != nullptr) {
    refCount.fetch_add(1, std::memory_order_relaxed);
  }
  return loaderHandle != nullptr;
}

bool DynamicArkUILoader::init() {
  std::call_once(initFlag, []() {
    if (!ensureLoaded()) {
      return;
    }
    auto func = reinterpret_cast<decltype(OH_ArkUI_RunTaskInScopeFun)>(
        dlsym(loaderHandle, "OH_ArkUI_RunTaskInScope"));
    if (!func) {
      LOG(ERROR) << "Failed to get OH_ArkUI_RunTaskInScope: " << dlerror();
      return;
    }
    OH_ArkUI_RunTaskInScopeFun = func;
    initialized = true;
  });
  return initialized;
}

bool DynamicArkUILoader::initAPI15() {
  std::call_once(initFlagAPI15, []() {
    if (!ensureLoaded()) {
      return;
    }
    auto func =
        reinterpret_cast<decltype(OH_ArkUI_NodeEvent_GetTextChangeEventFun)>(
            dlsym(loaderHandle, "OH_ArkUI_NodeEvent_GetTextChangeEvent"));
    if (!func) {
      LOG(ERROR) << "Failed to get OH_ArkUI_NodeEvent_GetTextChangeEvent: "
                 << dlerror();
      return;
    }
    OH_ArkUI_NodeEvent_GetTextChangeEventFun = func;
    initializedAPI15 = true;
  });
  return initializedAPI15;
}

bool DynamicArkUILoader::initAPI22() {
  std::call_once(initFlagAPI22, []() {
    if (!ensureLoaded()) {
      return;
    }
    auto func = reinterpret_cast<
        decltype(OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurationsFun)>(
        dlsym(
            loaderHandle,
            "OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurations"));
    if (!func) {
      LOG(WARNING)
          << "Failed to get OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurations: "
          << dlerror() << " (API < 22, using fallback)";
      return;
    }
    OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurationsFun = func;
    initializedAPI22 = true;
  });
  return initializedAPI22;
}

// cleanup is only called when the application exits
void DynamicArkUILoader::cleanup() {
  int32_t current = refCount.fetch_sub(1, std::memory_order_relaxed);
  if (current == 1 && loaderHandle) {
    dlclose(loaderHandle);
    loaderHandle = nullptr;
    OH_ArkUI_RunTaskInScopeFun = nullptr;
    OH_ArkUI_NodeEvent_GetTextChangeEventFun = nullptr;
    OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurationsFun = nullptr;
    initialized = false;
    initializedAPI15 = false;
    initializedAPI22 = false;
  }
}

bool DynamicArkUILoader::isAvailable() {
  return initialized && OH_ArkUI_RunTaskInScopeFun != nullptr;
}

int32_t (*DynamicArkUILoader::getRunTaskFunction())(
    ArkUI_ContextHandle,
    void*,
    void (*)(void*)) {
  return OH_ArkUI_RunTaskInScopeFun;
}

ArkUI_TextChangeEvent* (*DynamicArkUILoader::getTextChangeEventFun())(
    ArkUI_NodeEvent* event) {
  return OH_ArkUI_NodeEvent_GetTextChangeEventFun;
}

void (*DynamicArkUILoader::getSetAnimationFrameDurationsFun())(
    ArkUI_DrawableDescriptor*,
    const int32_t*,
    int32_t) {
  return OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurationsFun;
}

} // namespace rnoh