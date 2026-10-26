/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <arkui/drawable_descriptor.h>
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <atomic>
#include <dlfcn.h>
#include <glog/logging.h>

namespace rnoh {

class DynamicArkUILoader {
 public:
  static bool init();
  static bool initAPI15();
  static bool initAPI22();
  static void cleanup();
  static bool isAvailable();
  static int32_t (
      *getRunTaskFunction())(ArkUI_ContextHandle, void*, void (*)(void*));
  static ArkUI_TextChangeEvent* (*getTextChangeEventFun())(
      ArkUI_NodeEvent* event);
  static void (*getSetAnimationFrameDurationsFun())(
      ArkUI_DrawableDescriptor*,
      const int32_t*,
      int32_t);

 private:
  static bool ensureLoaded();
  static void* loaderHandle;
  static int32_t (
      *OH_ArkUI_RunTaskInScopeFun)(ArkUI_ContextHandle, void*, void (*)(void*));
  static ArkUI_TextChangeEvent* (*OH_ArkUI_NodeEvent_GetTextChangeEventFun)(
      ArkUI_NodeEvent* event);
  static void (*OH_ArkUI_DrawableDescriptor_SetAnimationFrameDurationsFun)(
      ArkUI_DrawableDescriptor*,
      const int32_t*,
      int32_t);
  static bool initialized;
  static bool initializedAPI15;
  static bool initializedAPI22;
  static std::atomic<int32_t> refCount;
  static std::once_flag loadFlag;
  static std::once_flag initFlag;
  static std::once_flag initFlagAPI15;
  static std::once_flag initFlagAPI22;
};

} // namespace rnoh