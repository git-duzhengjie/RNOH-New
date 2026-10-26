/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: ArkUI Node Subclasses — vtable Mock Tests
// Tier: 4a (vtable Mock via MockArkUIAPI)
// Scope: Tests vtable-level patterns used by StackNode, TextNode,
//        ColumnNode, and RefreshNode.
//
// Scope note:
//   Tests exercise the same ArkUI_NativeNodeAPI_1 patterns that these
//   node subclasses use (createNode with specific type, setAttribute with
//   type-specific attributes, addChild/insertChildAt, removeChild).
//   Direct class-level tests require extending MockArkUIAPI vtable with
//   insertChildAt, removeAllChildren, registerNodeEvent, etc. + stubs for
//   OH_ArkUI_NodeEvent_* and OH_ArkUI_AccessibilityState_* functions.
// =============================================================================

#include <gtest/gtest.h>
#include "MockArkUIAPI.h"
#include "native_interface.h"
#include "native_node.h"

class StackNodeVtableTest : public ::testing::Test {
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
// StackNode patterns: ARKUI_NODE_STACK
// ---------------------------------------------------------------------------

TEST_F(StackNodeVtableTest, CreateStackNode_ReturnsValidHandle) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);
  ASSERT_NE(handle, nullptr);
  EXPECT_TRUE(g_mockNodes.count(handle) > 0);
}

TEST_F(StackNodeVtableTest, StackNode_SetAlignContent_CapturesI32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_STACK);

  ArkUI_NumberValue val[] = {{.i32 = (int32_t)ARKUI_ALIGNMENT_CENTER}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_STACK_ALIGN_CONTENT, &item);

  EXPECT_EQ(
      g_mockNodes[handle]
          .attributes[NODE_STACK_ALIGN_CONTENT]
          .item.value[0]
          .i32,
      (int32_t)ARKUI_ALIGNMENT_CENTER);
}

TEST_F(StackNodeVtableTest, StackNode_AddChild_TracksChildren) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle child = api->createNode(ARKUI_NODE_TEXT);
  api->addChild(parent, child);

  EXPECT_EQ(g_mockNodes[parent].children.size(), 1u);
  EXPECT_EQ(g_mockNodes[parent].children[0], child);
}

TEST_F(StackNodeVtableTest, StackNode_RemoveChild_DecrementsCount) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle child = api->createNode(ARKUI_NODE_TEXT);
  api->addChild(parent, child);
  ASSERT_EQ(g_mockNodes[parent].children.size(), 1u);

  api->removeChild(parent, child);
  EXPECT_EQ(g_mockNodes[parent].children.size(), 0u);
}

// ---------------------------------------------------------------------------
// TextNode patterns: ARKUI_NODE_TEXT
// ---------------------------------------------------------------------------

TEST_F(StackNodeVtableTest, CreateTextNode_ReturnsValidHandle) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);
  ASSERT_NE(handle, nullptr);
  EXPECT_TRUE(g_mockNodes.count(handle) > 0);
}

TEST_F(StackNodeVtableTest, TextNode_SetTextContent_CapturesString) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_AttributeItem item = {.string = "Hello World"};
  api->setAttribute(handle, NODE_TEXT_CONTENT, &item);

  EXPECT_STREQ(
      g_mockNodes[handle].attributes[NODE_TEXT_CONTENT].item.string,
      "Hello World");
}

TEST_F(StackNodeVtableTest, TextNode_SetFontSize_CapturesFloat) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {{.f32 = 16.0f}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_FONT_SIZE, &item);

  EXPECT_FLOAT_EQ(
      g_mockNodes[handle].attributes[NODE_FONT_SIZE].item.value[0].f32, 16.0f);
}

TEST_F(StackNodeVtableTest, TextNode_SetFontWeight_CapturesI32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {{.i32 = (int32_t)ARKUI_FONT_WEIGHT_BOLD}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_FONT_WEIGHT, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_FONT_WEIGHT].item.value[0].i32,
      (int32_t)ARKUI_FONT_WEIGHT_BOLD);
}

TEST_F(StackNodeVtableTest, TextNode_SetTextAlign_CapturesI32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {
      {.i32 = (int32_t)ARKUI_TEXT_ALIGNMENT_CENTER}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_TEXT_ALIGN, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_TEXT_ALIGN].item.value[0].i32,
      (int32_t)ARKUI_TEXT_ALIGNMENT_CENTER);
}

TEST_F(StackNodeVtableTest, TextNode_SetTextMaxLines_CapturesI32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {{.i32 = 3}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_TEXT_MAX_LINES, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_TEXT_MAX_LINES].item.value[0].i32, 3);
}

TEST_F(StackNodeVtableTest, TextNode_ResetTextMaxLines_ClearsAttribute) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {{.i32 = 5}};
  ArkUI_AttributeItem _item_1 = {val, 1};
  api->setAttribute(handle, NODE_TEXT_MAX_LINES, &_item_1);
  ASSERT_NE(api->getAttribute(handle, NODE_TEXT_MAX_LINES), nullptr);

  api->resetAttribute(handle, NODE_TEXT_MAX_LINES);
  EXPECT_EQ(api->getAttribute(handle, NODE_TEXT_MAX_LINES), nullptr);
}

TEST_F(StackNodeVtableTest, TextNode_SetTextColor_CapturesU32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {{.u32 = 0xFF0000FFu}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_FONT_COLOR, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_FONT_COLOR].item.value[0].u32,
      0xFF0000FFu);
}

TEST_F(StackNodeVtableTest, TextNode_SetCopyOption_CapturesI32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue val[] = {
      {.i32 = (int32_t)ARKUI_COPY_OPTIONS_LOCAL_DEVICE}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_TEXT_COPY_OPTION, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_TEXT_COPY_OPTION].item.value[0].i32,
      (int32_t)ARKUI_COPY_OPTIONS_LOCAL_DEVICE);
}

// ---------------------------------------------------------------------------
// ColumnNode patterns: ARKUI_NODE_COLUMN
// ---------------------------------------------------------------------------

TEST_F(StackNodeVtableTest, CreateColumnNode_ReturnsValidHandle) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_COLUMN);
  ASSERT_NE(handle, nullptr);
  EXPECT_TRUE(g_mockNodes.count(handle) > 0);
}

TEST_F(StackNodeVtableTest, ColumnNode_AddMultipleChildren_PreservesOrder) {
  auto* api = getApi();
  ArkUI_NodeHandle parent = api->createNode(ARKUI_NODE_COLUMN);
  ArkUI_NodeHandle c1 = api->createNode(ARKUI_NODE_TEXT);
  ArkUI_NodeHandle c2 = api->createNode(ARKUI_NODE_TEXT);
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

// ---------------------------------------------------------------------------
// RefreshNode patterns: ARKUI_NODE_REFRESH
// ---------------------------------------------------------------------------

TEST_F(StackNodeVtableTest, CreateRefreshNode_ReturnsValidHandle) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_REFRESH);
  ASSERT_NE(handle, nullptr);
  EXPECT_TRUE(g_mockNodes.count(handle) > 0);
}

TEST_F(StackNodeVtableTest, RefreshNode_SetRefreshing_CapturesI32) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_REFRESH);

  ArkUI_NumberValue val[] = {{.i32 = 1}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_REFRESH_REFRESHING, &item);

  EXPECT_EQ(
      g_mockNodes[handle].attributes[NODE_REFRESH_REFRESHING].item.value[0].i32, 1);
}

TEST_F(StackNodeVtableTest, RefreshNode_SetPullDownRatio_CapturesFloat) {
  auto* api = getApi();
  ArkUI_NodeHandle handle = api->createNode(ARKUI_NODE_REFRESH);

  ArkUI_NumberValue val[] = {{.f32 = 0.65f}};
  ArkUI_AttributeItem item = {val, 1};
  api->setAttribute(handle, NODE_REFRESH_PULL_DOWN_RATIO, &item);

  EXPECT_FLOAT_EQ(
      g_mockNodes[handle].attributes[NODE_REFRESH_PULL_DOWN_RATIO].item.value[0].f32,
      0.65f);
}

// ---------------------------------------------------------------------------
// Cross-node-type independence
// ---------------------------------------------------------------------------

TEST_F(StackNodeVtableTest, DifferentNodeTypes_ShareMockRegistryButKeepAttributes) {
  auto* api = getApi();
  ArkUI_NodeHandle stack = api->createNode(ARKUI_NODE_STACK);
  ArkUI_NodeHandle text = api->createNode(ARKUI_NODE_TEXT);

  ArkUI_NumberValue sv[] = {{.f32 = 100.0f}};
  ArkUI_AttributeItem _item_1 = {sv, 1};
  api->setAttribute(stack, NODE_WIDTH, &_item_1);

  ArkUI_NumberValue tv[] = {{.f32 = 200.0f}};
  ArkUI_AttributeItem _item_2 = {tv, 1};
  api->setAttribute(text, NODE_WIDTH, &_item_2);

  EXPECT_FLOAT_EQ(
      api->getAttribute(stack, NODE_WIDTH)->value[0].f32, 100.0f);
  EXPECT_FLOAT_EQ(
      api->getAttribute(text, NODE_WIDTH)->value[0].f32, 200.0f);
}
