/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Host stubs for ArkUI direct C functions (not vtable entries) that
 * `ArkUINode.cpp` references. These are:
 *
 *   - `OH_ArkUI_AccessibilityState_*` — create/dispose/set fields on an
 *     opaque `ArkUI_AccessibilityState` handle. Used by
 *     `ArkUINode::setAccessibilityState`. In the host test build the
 *     setters are no-ops and `Create` returns nullptr; ToggleNode /
 *     LoadingProgressNode tests never exercise accessibility state.
 *
 *   - `OH_ArkUI_NodeEvent_*` — read fields from an `ArkUI_NodeEvent*`.
 *     Used by the static `receiveEvent` callback registered in the
 *     ArkUINode constructor. The callback is registered but never
 *     invoked in tests (no events are delivered), so these stubs just
 *     need to satisfy the linker.
 *
 * Together with the mock `ArkUI_NativeNodeAPI_1` vtable in
 * `MockArkUIAPI.cpp`, these stubs let real ArkUINode / ToggleNode /
 * LoadingProgressNode instances be constructed and destroyed against
 * the host test build without linking `libace_ndk.z.so`.
 */

#include "native_interface.h"
#include "native_node.h"

extern "C" {

// --- AccessibilityState ---

ArkUI_AccessibilityState* OH_ArkUI_AccessibilityState_Create(void) {
  return nullptr;
}

void OH_ArkUI_AccessibilityState_Dispose(ArkUI_AccessibilityState* state) {
  (void)state;
}

void OH_ArkUI_AccessibilityState_SetDisabled(
    ArkUI_AccessibilityState* state,
    int32_t isDisabled) {
  (void)state;
  (void)isDisabled;
}

void OH_ArkUI_AccessibilityState_SetSelected(
    ArkUI_AccessibilityState* state,
    int32_t isSelected) {
  (void)state;
  (void)isSelected;
}

void OH_ArkUI_AccessibilityState_SetCheckedState(
    ArkUI_AccessibilityState* state,
    int32_t checkedState) {
  (void)state;
  (void)checkedState;
}

// --- NodeEvent (registered callback never invoked in tests) ---

ArkUI_NodeEventType OH_ArkUI_NodeEvent_GetEventType(ArkUI_NodeEvent* event) {
  (void)event;
  return static_cast<ArkUI_NodeEventType>(0);
}

ArkUI_NodeHandle OH_ArkUI_NodeEvent_GetNodeHandle(ArkUI_NodeEvent* event) {
  (void)event;
  return nullptr;
}

ArkUI_UIInputEvent* OH_ArkUI_NodeEvent_GetInputEvent(ArkUI_NodeEvent* event) {
  (void)event;
  return nullptr;
}

ArkUI_NodeComponentEvent* OH_ArkUI_NodeEvent_GetNodeComponentEvent(
    ArkUI_NodeEvent* event) {
  (void)event;
  return nullptr;
}

ArkUI_StringAsyncEvent* OH_ArkUI_NodeEvent_GetStringAsyncEvent(
    ArkUI_NodeEvent* event) {
  (void)event;
  return nullptr;
}

} // extern "C"
