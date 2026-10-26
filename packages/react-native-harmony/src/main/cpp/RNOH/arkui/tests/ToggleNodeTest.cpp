/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::ToggleNode` — the ArkUI node wrapper
 * used by `SwitchComponentInstance`. Per `gtest-module-catalog.md` the
 * module's Mock requirement is "vtable Mock (ArkUI ToggleNode)".
 *
 * Per guide §5.4, the ArkUI C API is intercepted via the shared
 * `MockArkUIAPI` (in `tests/mock/`). `OH_ArkUI_QueryModuleInterfaceByName`
 * returns a mock `ArkUI_NativeNodeAPI_1` vtable whose
 * `createNode`/`setAttribute`/`getAttribute`/`resetAttribute` store
 * every attribute in a per-handle map (`g_mockNodes`). The no-op
 * `addNodeEventReceiver` / `registerNodeEvent` stubs let real
 * `ToggleNode` instances be constructed and destroyed against the mock
 * vtable without crashing on a null function-pointer dereference.
 *
 * Tests exercise:
 *   - Constructor creates an `ARKUI_NODE_TOGGLE` node and registers the
 *     `NODE_TOGGLE_ON_CHANGE` event (+ the 2 base `NODE_EVENT_TYPES`).
 *   - `setSelectedColor` writes `NODE_TOGGLE_SELECTED_COLOR` with the
 *     colour's uint32 value; a null colour resets the attribute.
 *   - `setUnselectedColor` writes `NODE_TOGGLE_UNSELECTED_COLOR`.
 *   - `setThumbColor` writes `NODE_TOGGLE_SWITCH_POINT_COLOR`.
 *   - `setFocusable` writes `NODE_FOCUSABLE` with 0/1.
 *   - `setValue` writes `NODE_TOGGLE_VALUE` with 0/1; `getValue` reads
 *     it back and returns a bool.
 *   - `setToggleNodeDelegate` stores the delegate pointer, and
 *     `onNodeEvent(NODE_TOGGLE_ON_CHANGE, args)` forwards the new value
 *     to the delegate's `onValueChange(int32_t&)`.
 *   - Non-`NODE_TOGGLE_ON_CHANGE` events are NOT forwarded to the
 *     delegate.
 *   - Destructor unregisters `NODE_TOGGLE_ON_CHANGE` and disposes the
 *     node.
 */

#include <gtest/gtest.h>

#include "MockArkUIAPI.h"
#include "native_interface.h"
#include "native_node.h"

#include "ToggleNode.h"

#include <react/renderer/graphics/Color.h>

using rnoh::ToggleNode;
using rnoh::ToggleNodeDelegate;

namespace {

// A simple delegate that records the last value received.
class CapturingDelegate : public ToggleNodeDelegate {
 public:
  int32_t lastValue = -1;
  int callCount = 0;
  void onValueChange(int32_t& value) override {
    lastValue = value;
    callCount++;
  }
};

// Builds a SharedColour from a packed uint32_t (ARGB). The
// facebook::react::Color ctor used here matches the one exercised by
// the Animated tests' transform_stub.
facebook::react::SharedColor makeColor(uint32_t argb) {
  return facebook::react::Color(argb);
}

} // namespace

class ToggleNodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
  }
};

// --- Constructor / destructor lifecycle ---

TEST_F(ToggleNodeTest, Constructor_CreatesToggleNodeAndRegistersEvents) {
  resetMockState(); // Verify reset clears prior state (called in SetUp too).
  {
    ToggleNode node;
    auto handle = node.getArkUINodeHandle();
    EXPECT_NE(handle, nullptr);
    ASSERT_TRUE(g_mockNodes.count(handle));
  }
  // After destruction the mock registry should have disposed the node.
  EXPECT_EQ(g_mockCallLog.disposeNodeCount, 1);
  // Constructor registers: 2 base events (NODE_ON_ACCESSIBILITY_ACTIONS,
  // NODE_ON_TOUCH_INTERCEPT) + 1 toggle event (NODE_TOGGLE_ON_CHANGE).
  EXPECT_EQ(g_mockCallLog.registerNodeEventCount, 3);
  // Destructor unregisters the same 3 events.
  EXPECT_EQ(g_mockCallLog.unregisterNodeEventCount, 3);
}

TEST_F(ToggleNodeTest, Constructor_RegistersAddNodeEventReceiver) {
  {
    ToggleNode node;
    EXPECT_EQ(g_mockCallLog.addNodeEventReceiverCount, 1);
  }
  EXPECT_EQ(g_mockCallLog.removeNodeEventReceiverCount, 1);
}

// --- setSelectedColor ---

TEST_F(ToggleNodeTest, SetSelectedColor_StoresColorValueInAttribute) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  auto color = makeColor(0xFFFF0000); // opaque red
  node.setSelectedColor(color);

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_TOGGLE_SELECTED_COLOR));
  EXPECT_EQ(attrs[NODE_TOGGLE_SELECTED_COLOR].item.size, 1);
  EXPECT_EQ(attrs[NODE_TOGGLE_SELECTED_COLOR].item.value[0].u32, 0xFFFF0000u);
}

TEST_F(ToggleNodeTest, SetSelectedColor_NullColor_ResetsAttribute) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  // First set a colour, then pass null → resetAttribute should fire.
  node.setSelectedColor(makeColor(0xFF00FF00));
  ASSERT_TRUE(g_mockNodes[handle].attributes.count(NODE_TOGGLE_SELECTED_COLOR));

  facebook::react::SharedColor nullColor;
  node.setSelectedColor(nullColor);

  EXPECT_FALSE(
      g_mockNodes[handle].attributes.count(NODE_TOGGLE_SELECTED_COLOR));
  EXPECT_GT(g_mockCallLog.resetAttributeCount, 0);
}

// --- setUnselectedColor ---

TEST_F(ToggleNodeTest, SetUnselectedColor_StoresColorValueInAttribute) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setUnselectedColor(makeColor(0xFF0000FF)); // opaque blue

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_TOGGLE_UNSELECTED_COLOR));
  EXPECT_EQ(attrs[NODE_TOGGLE_UNSELECTED_COLOR].item.value[0].u32, 0xFF0000FFu);
}

TEST_F(ToggleNodeTest, SetUnselectedColor_NullColor_ResetsAttribute) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setUnselectedColor(makeColor(0xFF0000FF));
  ASSERT_TRUE(
      g_mockNodes[handle].attributes.count(NODE_TOGGLE_UNSELECTED_COLOR));

  facebook::react::SharedColor nullColor;
  node.setUnselectedColor(nullColor);

  EXPECT_FALSE(
      g_mockNodes[handle].attributes.count(NODE_TOGGLE_UNSELECTED_COLOR));
}

// --- setThumbColor ---

TEST_F(ToggleNodeTest, SetThumbColor_StoresColorInSwitchPointColorAttr) {
  // NOTE: the production code maps `thumbTintColor` to
  // `NODE_TOGGLE_SWITCH_POINT_COLOR`, not a "thumb color" attribute.
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setThumbColor(makeColor(0xFFFFFFFF)); // opaque white

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_TOGGLE_SWITCH_POINT_COLOR));
  EXPECT_EQ(
      attrs[NODE_TOGGLE_SWITCH_POINT_COLOR].item.value[0].u32, 0xFFFFFFFFu);
}

// --- setFocusable ---

TEST_F(ToggleNodeTest, SetFocusable_True_WritesOne) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setFocusable(true);

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_FOCUSABLE));
  EXPECT_EQ(attrs[NODE_FOCUSABLE].item.value[0].i32, 1);
}

TEST_F(ToggleNodeTest, SetFocusable_False_WritesZero) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setFocusable(false);

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_FOCUSABLE));
  EXPECT_EQ(attrs[NODE_FOCUSABLE].item.value[0].i32, 0);
}

// --- setValue / getValue ---

TEST_F(ToggleNodeTest, SetValue_True_StoresOneAndGetValueReturnsTrue) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setValue(true);

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_TOGGLE_VALUE));
  EXPECT_EQ(attrs[NODE_TOGGLE_VALUE].item.value[0].i32, 1);

  EXPECT_TRUE(node.getValue());
}

TEST_F(ToggleNodeTest, SetValue_False_StoresZeroAndGetValueReturnsFalse) {
  ToggleNode node;
  auto handle = node.getArkUINodeHandle();

  node.setValue(false);

  EXPECT_FALSE(node.getValue());
  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_TOGGLE_VALUE));
  EXPECT_EQ(attrs[NODE_TOGGLE_VALUE].item.value[0].i32, 0);
}

// --- setToggleNodeDelegate / onNodeEvent ---

TEST_F(ToggleNodeTest, OnNodeEvent_ToggleOnChange_ForwardsValueToDelegate) {
  ToggleNode node;
  CapturingDelegate delegate;
  node.setToggleNodeDelegate(&delegate);

  // ArkUINode::EventArgs (ArkUI_NumberValue[12]) is a protected
  // typedef; use the raw array type directly.
  ArkUI_NumberValue args[MAX_COMPONENT_EVENT_ARG_NUM]{};
  args[0].i32 = 1;
  node.onNodeEvent(NODE_TOGGLE_ON_CHANGE, args);

  EXPECT_EQ(delegate.callCount, 1);
  EXPECT_EQ(delegate.lastValue, 1);

  // Directly exercise the ToggleNodeDelegate::onValueChange virtual
  // (the same path ToggleNode::onNodeEvent dispatches through) with a
  // fresh value to confirm the delegate records the call.
  int32_t v = 7;
  delegate.onValueChange(v);
  EXPECT_EQ(delegate.callCount, 2);
  EXPECT_EQ(delegate.lastValue, 7);
}

TEST_F(ToggleNodeTest, OnNodeEvent_NonToggleEvent_DoesNotCallDelegate) {
  ToggleNode node;
  CapturingDelegate delegate;
  node.setToggleNodeDelegate(&delegate);

  ArkUI_NumberValue args[MAX_COMPONENT_EVENT_ARG_NUM]{};
  args[0].i32 = 42;
  // A base-class event type — should NOT trigger the toggle delegate.
  node.onNodeEvent(NODE_ON_TOUCH_INTERCEPT, args);

  EXPECT_EQ(delegate.callCount, 0);
}

TEST_F(ToggleNodeTest, OnNodeEvent_ToggleOnChange_NoDelegate_DoesNotCrash) {
  // No delegate set — onNodeEvent must not dereference a null delegate.
  ToggleNode node;
  ArkUI_NumberValue args[MAX_COMPONENT_EVENT_ARG_NUM]{};
  args[0].i32 = 1;
  EXPECT_NO_FATAL_FAILURE(node.onNodeEvent(NODE_TOGGLE_ON_CHANGE, args));
}
