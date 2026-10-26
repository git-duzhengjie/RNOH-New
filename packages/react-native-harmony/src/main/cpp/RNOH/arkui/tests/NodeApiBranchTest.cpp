/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for the API-level branching in `NodeApi` — the
 * wrapper class that ArkUINode uses for all `createNode` /
 * `setAttribute` / `getAttribute` calls.
 *
 * Per `gtest-module-catalog.md` the component tier requires "vtable
 * Mock (ArkUI ToggleNode / LoadingProgressNode)". The production
 * `NodeApi::createNode` etc. branch on `IsNewAPIVersion()` (API >= 20):
 *
 *   - Old API path (API < 20): delegates to
 *     `NativeNodeApi::getInstance()->createNode()` → mock vtable.
 *   - New API path (API >= 20): calls `DynamicArkUILoader::
 *     getRunTaskFunction()` which runs a callback in a UIContext
 *     scope, and the callback delegates to `NativeNodeApi::getInstance()`.
 *
 * This test links `NodeApiTestImpl.cpp` instead of the production
 * `NodeApi.cpp`. `NodeApiTestImpl.cpp` is copied verbatim from
 * `NodeApi.cpp`'s method bodies, with the only difference being that
 * `DynamicArkUILoader::getRunTaskFunction()` is replaced with an
 * injectable hook — see that file's header for the verbatim-copy
 * relationship and re-sync requirement. The hook makes
 * `IsNewAPIVersion()` injectable via `setNewApiVersionForTesting()` /
 * `setRunTaskFunctionForTesting()`, so BOTH paths can be exercised
 * on the v12 SDK host build.
 *
 * Tests exercise:
 *   - Old API path: `createNode` / `setAttribute` / `resetAttribute` /
 *     `getAttribute` delegate to `NativeNodeApi::getInstance()` (mock
 *     vtable), no run-task callback involved.
 *   - New API path: `createNode` / `setAttribute` / `resetAttribute`
 *     call the injectable run-task function, which invokes a callback
 *     that delegates to `NativeNodeApi::getInstance()` (mock vtable).
 *   - New API path with null UIContext: falls back to old API path.
 *   - New API path with run-task returning error: `setAttribute`
 *     throws `std::runtime_error`.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <stdexcept>

#include "MockArkUIAPI.h"
#include "native_interface.h"
#include "native_node.h"

#include "NativeNodeApi.h"
#include "NodeApi.h"

// Forward declarations of test-only hooks defined in NodeApiTestImpl.cpp.
namespace rnoh {
void setNewApiVersionForTesting(bool enabled);
void setRunTaskFunctionForTesting(
    std::function<int32_t(ArkUI_ContextHandle, void*, void (*)(void*))> fn);
void clearRunTaskFunctionForTesting();
} // namespace rnoh

using rnoh::clearRunTaskFunctionForTesting;
using rnoh::NodeApi;
using rnoh::setNewApiVersionForTesting;
using rnoh::setRunTaskFunctionForTesting;

namespace {

// A fake UIContext handle — the production code only passes it through,
// never dereferences it.
ArkUI_ContextHandle kFakeUiContext =
    reinterpret_cast<ArkUI_ContextHandle>(0xDEAD);

// Tracks whether the run-task callback was invoked.
int g_callbackInvokeCount = 0;

// A run-task function that immediately invokes the callback (synchronous,
// like the real OH_ArkUI_RunTaskInScope on device).
int32_t synchronousRunTask(
    ArkUI_ContextHandle ctx,
    void* userData,
    void (*callback)(void*)) {
  g_callbackInvokeCount++;
  callback(userData);
  return 0; // ARKUI_ERROR_CODE_NO_ERROR
}

} // namespace

class NodeApiBranchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
    g_callbackInvokeCount = 0;
    // Default: old API path.
    clearRunTaskFunctionForTesting();
  }

  void TearDown() override {
    clearRunTaskFunctionForTesting();
  }
};

// --- Old API path (API < 20) ---

TEST_F(NodeApiBranchTest, OldApi_CreateNode_DelegatesToNativeNodeApi) {
  NodeApi api(nullptr); // null uiContext → old API path
  ArkUI_NodeHandle handle = api.createNode(ARKUI_NODE_STACK);
  EXPECT_NE(handle, nullptr);
  EXPECT_EQ(g_mockCallLog.createNodeCount, 1);
  EXPECT_EQ(g_callbackInvokeCount, 0); // no run-task callback
}

TEST_F(NodeApiBranchTest, OldApi_SetAttribute_DelegatesToNativeNodeApi) {
  NodeApi api(nullptr);
  auto handle = api.createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 100.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api.setAttribute(handle, NODE_WIDTH, &item);

  EXPECT_EQ(g_mockCallLog.setAttributeCount, 1);
  EXPECT_EQ(g_callbackInvokeCount, 0);
}

TEST_F(NodeApiBranchTest, OldApi_ResetAttribute_DelegatesToNativeNodeApi) {
  NodeApi api(nullptr);
  auto handle = api.createNode(ARKUI_NODE_STACK);
  api.resetAttribute(handle, NODE_WIDTH);
  EXPECT_EQ(g_mockCallLog.resetAttributeCount, 1);
  EXPECT_EQ(g_callbackInvokeCount, 0);
}

TEST_F(NodeApiBranchTest, OldApi_GetAttribute_DelegatesToNativeNodeApi) {
  NodeApi api(nullptr);
  auto handle = api.createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 42.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api.setAttribute(handle, NODE_WIDTH, &item);

  auto retrieved = api.getAttribute(handle, NODE_WIDTH);
  ASSERT_NE(retrieved, nullptr);
  EXPECT_FLOAT_EQ(retrieved->value[0].f32, 42.0f);
  EXPECT_EQ(g_callbackInvokeCount, 0);
}

// --- New API path (API >= 20) ---

TEST_F(NodeApiBranchTest, NewApi_CreateNode_InvokesRunTaskCallback) {
  setRunTaskFunctionForTesting(synchronousRunTask);
  NodeApi api(kFakeUiContext);

  ArkUI_NodeHandle handle = api.createNode(ARKUI_NODE_STACK);
  EXPECT_NE(handle, nullptr);
  // createNodeWithUIContext calls runTask once, and the callback calls
  // NativeNodeApi::getInstance()->createNode — so createNodeCount
  // increments by 1 (inside the callback).
  EXPECT_EQ(g_mockCallLog.createNodeCount, 1);
  EXPECT_EQ(g_callbackInvokeCount, 1);
}

TEST_F(NodeApiBranchTest, NewApi_SetAttribute_InvokesRunTaskCallback) {
  setRunTaskFunctionForTesting(synchronousRunTask);
  NodeApi api(kFakeUiContext);
  auto handle = api.createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 200.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api.setAttribute(handle, NODE_WIDTH, &item);

  EXPECT_EQ(g_callbackInvokeCount, 2); // 1 for createNode + 1 for setAttribute
  // The callback called NativeNodeApi::getInstance()->setAttribute.
  ASSERT_TRUE(g_mockNodes[handle].attributes.count(NODE_WIDTH));
  EXPECT_FLOAT_EQ(
      g_mockNodes[handle].attributes[NODE_WIDTH].item.value[0].f32, 200.0f);
}

TEST_F(NodeApiBranchTest, NewApi_ResetAttribute_InvokesRunTaskCallback) {
  setRunTaskFunctionForTesting(synchronousRunTask);
  NodeApi api(kFakeUiContext);
  auto handle = api.createNode(ARKUI_NODE_STACK);

  // Set first via old path so the attribute exists.
  ArkUI_NumberValue val[] = {{.f32 = 1.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api.setAttribute(handle, NODE_WIDTH, &item);
  ASSERT_TRUE(g_mockNodes[handle].attributes.count(NODE_WIDTH));

  // Reset via new path.
  int beforeCallback = g_callbackInvokeCount;
  api.resetAttribute(handle, NODE_WIDTH);

  EXPECT_EQ(g_callbackInvokeCount, beforeCallback + 1);
  EXPECT_FALSE(g_mockNodes[handle].attributes.count(NODE_WIDTH));
}

TEST_F(NodeApiBranchTest, NewApi_GetAttribute_DelegatesDirectly_NoRunTask) {
  // getAttribute always delegates directly to NativeNodeApi (no
  // UIContext-scoped path) — verify no run-task callback.
  setRunTaskFunctionForTesting(synchronousRunTask);
  NodeApi api(kFakeUiContext);
  auto handle = api.createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 99.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api.setAttribute(handle, NODE_WIDTH, &item);

  int beforeGet = g_callbackInvokeCount;
  auto retrieved = api.getAttribute(handle, NODE_WIDTH);
  EXPECT_EQ(g_callbackInvokeCount, beforeGet); // no new callback
  ASSERT_NE(retrieved, nullptr);
  EXPECT_FLOAT_EQ(retrieved->value[0].f32, 99.0f);
}

// --- New API path with null UIContext → falls back to old path ---

TEST_F(NodeApiBranchTest, NewApi_NullUiContext_FallsBackToOldApiPath) {
  setRunTaskFunctionForTesting(synchronousRunTask);
  NodeApi api(nullptr); // null uiContext → old path even if new API enabled

  auto handle = api.createNode(ARKUI_NODE_STACK);
  EXPECT_NE(handle, nullptr);
  EXPECT_EQ(g_mockCallLog.createNodeCount, 1);
  EXPECT_EQ(g_callbackInvokeCount, 0); // no run-task callback
}

// --- New API path with run-task returning error → throws ---

TEST_F(NodeApiBranchTest, NewApi_RunTaskError_SetAttribute_ThrowsRuntimeError) {
  auto failingRunTask =
      [](ArkUI_ContextHandle, void* userData, void (*callback)(void*)) {
        g_callbackInvokeCount++;
        return -1; // non-zero error code
      };
  setRunTaskFunctionForTesting(failingRunTask);
  NodeApi api(kFakeUiContext);
  auto handle = api.createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 1.0f}};
  ArkUI_AttributeItem item = {val, 1};
  EXPECT_THROW(api.setAttribute(handle, NODE_WIDTH, &item), std::runtime_error);
  // The callback was NOT invoked because runTask returned error.
  // setAttributeWithUIContext calls runTask once.
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 0);
}

// --- addChild / insertChildAt branching ---

TEST_F(NodeApiBranchTest, OldApi_AddChild_DelegatesToNativeNodeApi) {
  NodeApi api(nullptr);
  auto parent = api.createNode(ARKUI_NODE_STACK);
  auto child = api.createNode(ARKUI_NODE_TEXT);
  api.addChild(parent, child);
  EXPECT_EQ(g_mockCallLog.addChildCount, 1);
  EXPECT_EQ(g_callbackInvokeCount, 0);
}

TEST_F(NodeApiBranchTest, NewApi_AddChild_InvokesRunTaskCallback) {
  setRunTaskFunctionForTesting(synchronousRunTask);
  NodeApi api(kFakeUiContext);
  auto parent = api.createNode(ARKUI_NODE_STACK);
  auto child = api.createNode(ARKUI_NODE_TEXT);
  int beforeAdd = g_callbackInvokeCount;
  api.addChild(parent, child);
  EXPECT_EQ(g_callbackInvokeCount, beforeAdd + 1);
  EXPECT_EQ(g_mockCallLog.addChildCount, 1);
  ASSERT_EQ(g_mockNodes[parent].children.size(), 1u);
  EXPECT_EQ(g_mockNodes[parent].children[0], child);
}
