/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::LoadingProgressNode` — the ArkUI node
 * wrapper used by `ActivityIndicatorComponentInstance`. Per
 * `gtest-module-catalog.md` the module's Mock requirement is
 * "vtable Mock (ArkUI LoadingProgress)".
 *
 * Per guide §5.4, the ArkUI C API is intercepted via the shared
 * `MockArkUIAPI` (in `tests/mock/`). `OH_ArkUI_QueryModuleInterfaceByName`
 * returns a mock `ArkUI_NativeNodeAPI_1` vtable; `setAttribute` /
 * `resetAttribute` / `getAttribute` store / erase / read attributes in
 * the per-handle `g_mockNodes` map.
 *
 * Tests exercise:
 *   - Constructor creates an `ARKUI_NODE_LOADING_PROGRESS` node.
 *   - `setColor` writes `NODE_LOADING_PROGRESS_COLOR` with the colour's
 *     uint32 value.
 *   - `resetColor` erases the `NODE_LOADING_PROGRESS_COLOR` attribute.
 *   - `setAnimating(true)` writes `NODE_LOADING_PROGRESS_ENABLE_LOADING`
 *     with value 1; `setAnimating(false)` writes 0.
 */

#include <gtest/gtest.h>

#include "MockArkUIAPI.h"
#include "native_interface.h"
#include "native_node.h"

#include "LoadingProgressNode.h"

#include <react/renderer/graphics/Color.h>

using rnoh::LoadingProgressNode;

namespace {
facebook::react::SharedColor makeColor(uint32_t argb) {
  return facebook::react::Color(argb);
}
} // namespace

class LoadingProgressNodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
  }
};

// --- Constructor ---

TEST_F(LoadingProgressNodeTest, Constructor_CreatesLoadingProgressNode) {
  resetMockState(); // Verify reset clears prior state (called in SetUp too).
  {
    LoadingProgressNode node;
    auto handle = node.getArkUINodeHandle();
    EXPECT_NE(handle, nullptr);
    ASSERT_TRUE(g_mockNodes.count(handle));
  }
  EXPECT_EQ(g_mockCallLog.disposeNodeCount, 1);
  // LoadingProgressNode registers the 2 base events
  // (NODE_ON_ACCESSIBILITY_ACTIONS, NODE_ON_TOUCH_INTERCEPT) but no
  // component-specific event.
  EXPECT_EQ(g_mockCallLog.registerNodeEventCount, 2);
  EXPECT_EQ(g_mockCallLog.unregisterNodeEventCount, 2);
}

// --- setColor ---

TEST_F(
    LoadingProgressNodeTest,
    SetColor_StoresColorInLoadingProgressColorAttr) {
  LoadingProgressNode node;
  auto handle = node.getArkUINodeHandle();

  node.setColor(makeColor(0xFF112233));

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_LOADING_PROGRESS_COLOR));
  EXPECT_EQ(attrs[NODE_LOADING_PROGRESS_COLOR].item.size, 1);
  EXPECT_EQ(attrs[NODE_LOADING_PROGRESS_COLOR].item.value[0].u32, 0xFF112233u);
}

TEST_F(LoadingProgressNodeTest, SetColor_Overwrite_ReplacesPreviousValue) {
  LoadingProgressNode node;
  auto handle = node.getArkUINodeHandle();

  node.setColor(makeColor(0xFF000000));
  node.setColor(makeColor(0xFFFFFFFF));

  auto& attrs = g_mockNodes[handle].attributes;
  EXPECT_EQ(attrs[NODE_LOADING_PROGRESS_COLOR].item.value[0].u32, 0xFFFFFFFFu);
  // Only one entry should exist — setAttribute overwrites.
  EXPECT_EQ(attrs.count(NODE_LOADING_PROGRESS_COLOR), 1u);
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 2);
}

// --- resetColor ---

TEST_F(LoadingProgressNodeTest, ResetColor_ErasesLoadingProgressColorAttr) {
  LoadingProgressNode node;
  auto handle = node.getArkUINodeHandle();

  node.setColor(makeColor(0xFFAABBCC));
  ASSERT_TRUE(
      g_mockNodes[handle].attributes.count(NODE_LOADING_PROGRESS_COLOR));

  node.resetColor();

  EXPECT_FALSE(
      g_mockNodes[handle].attributes.count(NODE_LOADING_PROGRESS_COLOR));
  EXPECT_GT(g_mockCallLog.resetAttributeCount, 0);
}

// --- setAnimating ---

TEST_F(
    LoadingProgressNodeTest,
    SetAnimating_True_WritesOneToEnableLoadingAttr) {
  LoadingProgressNode node;
  auto handle = node.getArkUINodeHandle();

  node.setAnimating(true);

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_LOADING_PROGRESS_ENABLE_LOADING));
  EXPECT_EQ(attrs[NODE_LOADING_PROGRESS_ENABLE_LOADING].item.value[0].u32, 1u);
}

TEST_F(
    LoadingProgressNodeTest,
    SetAnimating_False_WritesZeroToEnableLoadingAttr) {
  LoadingProgressNode node;
  auto handle = node.getArkUINodeHandle();

  node.setAnimating(false);

  auto& attrs = g_mockNodes[handle].attributes;
  ASSERT_TRUE(attrs.count(NODE_LOADING_PROGRESS_ENABLE_LOADING));
  EXPECT_EQ(attrs[NODE_LOADING_PROGRESS_ENABLE_LOADING].item.value[0].u32, 0u);
}

TEST_F(LoadingProgressNodeTest, EnableThenDisable_BothCallsRecorded) {
  // Verify that toggling animating on then off produces two distinct
  // setAttribute calls with the expected values.
  LoadingProgressNode node;
  node.setAnimating(true);
  node.setAnimating(false);
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 2);
}
