/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: ArkUI Node Infrastructure — vtable Mock Tests
// Tier: 4a (vtable Mock via MockArkUIAPI)
// Scope: Tests the shared vtable mock infrastructure at the API level,
//        exercising the same ArkUI_NativeNodeAPI_1 patterns that ArkUINode,
//        StackNode, ColumnNode, etc. rely on.
//
// Scope note:
//   These tests exercise the vtable mock API directly (not the ArkUINode
//   C++ class) because ArkUINode.h has a heavy include chain
//   (folly, react/renderer/*, glog, NodeApi.h → DynamicArkUILoader.h).
//   To test ArkUINode class directly, MockArkUIAPI needs to be extended
//   with: addNodeEventReceiver, removeNodeEventReceiver, registerNodeEvent,
//   unregisterNodeEvent, markDirty, insertChildAt, removeAllChildren,
//   getLayoutPosition, getMeasuredSize, setMeasuredSize, getChildAt.
//   Additionally, stubs are needed for OH_ArkUI_NodeEvent_GetEventType,
//   OH_ArkUI_NodeEvent_GetNodeHandle, OH_ArkUI_AccessibilityState_Create, etc.
// =============================================================================

#include <gtest/gtest.h>
#include "MockArkUIAPI.h"
#include "native_interface.h"
#include "native_node.h"

class ArkUINodeVtableTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
  }

  ArkUI_NativeNodeAPI_1* getApi() {
    ArkUI_NativeNodeAPI_1* api = nullptr;
    OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_NODE, ArkUI_NativeNodeAPI_1, api);
    return api;
  }
};

// ---------------------------------------------------------------------------
// Vtable retrieval
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, GetModuleInterface_ReturnsValidVtable) {
  ArkUI_NativeNodeAPI_1* api = getApi();
  ASSERT_NE(api, nullptr);
  EXPECT_EQ(api->version, 1);
}

TEST_F(ArkUINodeVtableTest, QueryModuleInterface_WrongKind_ReturnsNull) {
  void* result = OH_ArkUI_QueryModuleInterfaceByName(
      ARKUI_NATIVE_DIALOG, "ArkUI_NativeNodeAPI_1");
  EXPECT_EQ(result, nullptr);
}

TEST_F(ArkUINodeVtableTest, QueryModuleInterface_WrongStructName_ReturnsNull) {
  void* result = OH_ArkUI_QueryModuleInterfaceByName(
      ARKUI_NATIVE_NODE, "WrongStruct");
  EXPECT_EQ(result, nullptr);
}

// ---------------------------------------------------------------------------
// Node creation and disposal (ArkUINode constructor/destructor pattern)
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, CreateNode_StackType_ReturnsValidHandle) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);
  ASSERT_NE(handle, nullptr);
  EXPECT_TRUE(g_mockNodes.count(handle) > 0);
  EXPECT_EQ(g_mockCallLog.createNodeCount, 1);
}

TEST_F(ArkUINodeVtableTest, CreateNode_TextType_ReturnsValidHandle) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);
  ASSERT_NE(handle, nullptr);
  EXPECT_TRUE(g_mockNodes.count(handle) > 0);
}

TEST_F(ArkUINodeVtableTest, CreateNode_MultipleCalls_ReturnDistinctHandles) {
  auto* api = getApi();
  ArkUI_NodeHandle h1 = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle h2 = api->createNode(ARKUI_NODE_TEXT);
  ArkUI_NodeHandle h3 = api->createNode(ARKUI_NODE_IMAGE);
  EXPECT_NE(h1, h2);
  EXPECT_NE(h2, h3);
  EXPECT_NE(h1, h3);
  EXPECT_EQ(g_mockCallLog.createNodeCount, 3);
}

TEST_F(ArkUINodeVtableTest, DisposeNode_RemovesFromRegistry) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);
  ASSERT_TRUE(g_mockNodes.count(handle) > 0);

  api->disposeNode(handle);
  EXPECT_EQ(g_mockNodes.count(handle), 0u);
  EXPECT_EQ(g_mockCallLog.disposeNodeCount, 1);
}

// ---------------------------------------------------------------------------
// Attribute set/get/reset (ArkUINode setter pattern)
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, SetAttribute_Width_CapturesInMockState) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 100.0f}};
  ArkUI_AttributeItem item = {val, 1};
  int32_t ret = api->setAttribute(handle, NODE_WIDTH, &item);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 1);
  EXPECT_FLOAT_EQ(
      g_mockNodes[handle].attributes[NODE_WIDTH].item.value[0].f32, 100.0f);
}

TEST_F(ArkUINodeVtableTest, SetAttribute_Height_CapturesInMockState) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 200.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_HEIGHT, &item);

  EXPECT_FLOAT_EQ(
      g_mockNodes[handle].attributes[NODE_HEIGHT].item.value[0].f32, 200.0f);
}

TEST_F(ArkUINodeVtableTest, SetAttribute_Opacity_CapturesFloatValue) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 0.5f}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_OPACITY, &item);

  EXPECT_FLOAT_EQ(
      g_mockNodes[handle].attributes[NODE_OPACITY].item.value[0].f32, 0.5f);
}

TEST_F(ArkUINodeVtableTest, SetAttribute_BackgroundColor_CapturesU32Value) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.u32 = 0xFF0000FF}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_BACKGROUND_COLOR, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_BACKGROUND_COLOR].item.value[0].u32,
      0xFF0000FFu);
}

TEST_F(ArkUINodeVtableTest, SetAttribute_Enabled_CapturesI32Value) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.i32 = 1}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_ENABLED, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_ENABLED].item.value[0].i32, 1);
}

TEST_F(ArkUINodeVtableTest, SetAttribute_Alignment_CapturesI32Value) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.i32 = (int32_t)ARKUI_ALIGNMENT_CENTER}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_ALIGNMENT, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_ALIGNMENT].item.value[0].i32,
      (int32_t)ARKUI_ALIGNMENT_CENTER);
}

TEST_F(ArkUINodeVtableTest, GetAttribute_ReturnsPreviouslySetValue) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 250.5f}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_WIDTH, &item);

  const ArkUI_AttributeItem* retrieved = api->getAttribute(handle, NODE_WIDTH);
  ASSERT_NE(retrieved, nullptr);
  EXPECT_FLOAT_EQ(retrieved->value[0].f32, 250.5f);
}

TEST_F(ArkUINodeVtableTest, GetAttribute_NotSet_ReturnsNull) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  const ArkUI_AttributeItem* retrieved = api->getAttribute(handle, NODE_WIDTH);
  EXPECT_EQ(retrieved, nullptr);
}

TEST_F(ArkUINodeVtableTest, ResetAttribute_ClearsStoredValue) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.f32 = 42.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_WIDTH, &item);
  ASSERT_NE(api->getAttribute(handle, NODE_WIDTH), nullptr);

  api->resetAttribute(handle, NODE_WIDTH);
  EXPECT_EQ(api->getAttribute(handle, NODE_WIDTH), nullptr);
  EXPECT_EQ(g_mockCallLog.resetAttributeCount, 1);
}

// ---------------------------------------------------------------------------
// Multi-attribute independence (ArkUINode fluent setter pattern)
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, MultipleAttributes_OnSameNode_AreIndependent) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue w[] = {{.f32 = 100.0f}};
  ArkUI_AttributeItem _item_1 = {w, 1};
  api->setAttribute(handle, NODE_WIDTH, &_item_1);

  ArkUI_NumberValue h[] = {{.f32 = 200.0f}};
  ArkUI_AttributeItem _item_2 = {h, 1};
  api->setAttribute(handle, NODE_HEIGHT, &_item_2);

  ArkUI_NumberValue op[] = {{.f32 = 0.8f}};
  ArkUI_AttributeItem _item_3 = {op, 1};
  api->setAttribute(handle, NODE_OPACITY, &_item_3);

  EXPECT_FLOAT_EQ(
      api->getAttribute(handle, NODE_WIDTH)->value[0].f32, 100.0f);
  EXPECT_FLOAT_EQ(
      api->getAttribute(handle, NODE_HEIGHT)->value[0].f32, 200.0f);
  EXPECT_FLOAT_EQ(
      api->getAttribute(handle, NODE_OPACITY)->value[0].f32, 0.8f);
}

TEST_F(ArkUINodeVtableTest, SameAttribute_OnDifferentNodes_AreIndependent) {
  auto* api = getApi();
  ArkUI_NodeHandle h1 = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle h2 = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue v1[] = {{.f32 = 100.0f}};
  ArkUI_AttributeItem _item_1 = {v1, 1};
  api->setAttribute(h1, NODE_WIDTH, &_item_1);

  ArkUI_NumberValue v2[] = {{.f32 = 200.0f}};
  ArkUI_AttributeItem _item_2 = {v2, 1};
  api->setAttribute(h2, NODE_WIDTH, &_item_2);

  EXPECT_FLOAT_EQ(api->getAttribute(h1, NODE_WIDTH)->value[0].f32, 100.0f);
  EXPECT_FLOAT_EQ(api->getAttribute(h2, NODE_WIDTH)->value[0].f32, 200.0f);
}

// ---------------------------------------------------------------------------
// Parent-child relationships (StackNode/ColumnNode insertChild pattern)
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, AddChild_RecordsParentChildRelationship) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle child = api->createNode(ARKUI_NODE_TEXT);

  api->addChild(parent, child);

  auto& state = g_mockNodes[parent];
  EXPECT_EQ(state.children.size(), 1u);
  EXPECT_EQ(state.children[0], child);
  EXPECT_EQ(state.totalChildCount, 1);
  EXPECT_EQ(g_mockCallLog.addChildCount, 1);
}

TEST_F(ArkUINodeVtableTest, AddChild_MultipleChildren_PreservesOrder) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_COLUMN);
  ArkUI_NodeHandle c1 = api->createNode(ARKUI_NODE_TEXT);
  ArkUI_NodeHandle c2 = api->createNode(ARKUI_NODE_IMAGE);
  ArkUI_NodeHandle c3 = api->createNode(ARKUI_NODE_TEXT);

  api->addChild(parent, c1);
  api->addChild(parent, c2);
  api->addChild(parent, c3);

  auto& state = g_mockNodes[parent];
  EXPECT_EQ(state.children.size(), 3u);
  EXPECT_EQ(state.children[0], c1);
  EXPECT_EQ(state.children[1], c2);
  EXPECT_EQ(state.children[2], c3);
}

TEST_F(ArkUINodeVtableTest, RemoveChild_RemovesFromParentChildren) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle child = api->createNode(ARKUI_NODE_TEXT);
  api->addChild(parent, child);
  ASSERT_EQ(g_mockNodes[parent].children.size(), 1u);

  api->removeChild(parent, child);

  EXPECT_EQ(g_mockNodes[parent].children.size(), 0u);
  EXPECT_EQ(g_mockCallLog.removeChildCount, 1);
}

TEST_F(ArkUINodeVtableTest, RemoveChild_NotInParent_ChildCountUnchanged) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle child = api->createNode(ARKUI_NODE_TEXT);

  api->removeChild(parent, child);

  EXPECT_EQ(g_mockNodes[parent].children.size(), 0u);
  EXPECT_EQ(g_mockCallLog.removeChildCount, 1);
}

// ---------------------------------------------------------------------------
// End-to-end pattern: create → set attributes → get → reset → dispose
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, FullLifecycle_CreateSetGetResetDispose_Succeeds) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);
  ASSERT_NE(handle, nullptr);

  ArkUI_NumberValue w[] = {{.f32 = 390.0f}};
  ArkUI_AttributeItem _item_1 = {w, 1};
  api->setAttribute(handle, NODE_WIDTH, &_item_1);

  ArkUI_NumberValue h[] = {{.f32 = 844.0f}};
  ArkUI_AttributeItem _item_2 = {h, 1};
  api->setAttribute(handle, NODE_HEIGHT, &_item_2);

  ArkUI_NumberValue bg[] = {{.u32 = 0xFFFFFFFFu}};
  ArkUI_AttributeItem _item_3 = {bg, 1};
  api->setAttribute(handle, NODE_BACKGROUND_COLOR, &_item_3);

  EXPECT_FLOAT_EQ(api->getAttribute(handle, NODE_WIDTH)->value[0].f32, 390.0f);
  EXPECT_FLOAT_EQ(api->getAttribute(handle, NODE_HEIGHT)->value[0].f32, 844.0f);
  EXPECT_EQ(
      api->getAttribute(handle, NODE_BACKGROUND_COLOR)->value[0].u32,
      0xFFFFFFFFu);

  api->resetAttribute(handle, NODE_WIDTH);
  EXPECT_EQ(api->getAttribute(handle, NODE_WIDTH), nullptr);

  api->disposeNode(handle);
  EXPECT_EQ(g_mockNodes.count(handle), 0u);
}

// ---------------------------------------------------------------------------
// Isolation: resetMockState ensures no cross-test contamination
// ---------------------------------------------------------------------------

TEST_F(ArkUINodeVtableTest, SetUp_ResetsCallLogToZero) {
  EXPECT_EQ(g_mockCallLog.createNodeCount, 0);
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 0);
  EXPECT_EQ(g_mockCallLog.getAttributeCount, 0);
  EXPECT_EQ(g_mockCallLog.addChildCount, 0);
  EXPECT_EQ(g_mockCallLog.removeChildCount, 0);
  EXPECT_EQ(g_mockCallLog.disposeNodeCount, 0);
  EXPECT_EQ(g_mockCallLog.resetAttributeCount, 0);
}

TEST_F(ArkUINodeVtableTest, SetUp_ClearsAllMockNodes) {
  EXPECT_TRUE(g_mockNodes.empty());
}
