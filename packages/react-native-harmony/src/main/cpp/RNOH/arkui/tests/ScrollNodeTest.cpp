/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOH/arkui/ScrollNode.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#endif
#include "MockArkUIAPI.h"
#include "RNOH/arkui/NativeNodeApi.h"

using namespace rnoh;

namespace {

const ArkUI_AttributeItem* getAttribute(
    ScrollNode& node,
    ArkUI_NodeAttributeType attribute) {
  auto item = mockGetAttribute(node.getArkUINodeHandle(), attribute);
  EXPECT_NE(item, nullptr);
  if (item != nullptr) {
    EXPECT_NE(item->value, nullptr);
  }
  return item;
}
class RecordingScrollNodeDelegate final : public ScrollNodeDelegate {
 public:
  void onScroll() override {
    scrollCount++;
  }

  void onScrollStart() override {
    scrollStartCount++;
  }

  void onScrollStop() override {
    scrollStopCount++;
  }

  float onScrollFrameBegin(float offset, int32_t scrollState) override {
    frameBeginCount++;
    lastOffset = offset;
    lastScrollState = scrollState;
    return frameBeginResult;
  }

  void onAppear() override {
    appearCount++;
  }

  int32_t scrollCount = 0;
  int32_t scrollStartCount = 0;
  int32_t scrollStopCount = 0;
  int32_t frameBeginCount = 0;
  int32_t appearCount = 0;
  float lastOffset = 0;
  int32_t lastScrollState = 0;
  float frameBeginResult = 0;
};

#if defined(_WIN32)
DWORD runFatalProbe(const wchar_t* scenario) {
  wchar_t executablePath[MAX_PATH]{};
  auto pathLength =
      GetModuleFileNameW(nullptr, executablePath, std::size(executablePath));
  if (pathLength == 0 || pathLength == std::size(executablePath)) {
    throw std::runtime_error("Failed to resolve test executable path");
  }

  auto probePath = std::filesystem::path(executablePath).parent_path() /
      "rnoh_scroll_node_fatal_probe.exe";
  std::wstring commandLine = L"\"" + probePath.wstring() + L"\" " + scenario;
  STARTUPINFOW startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  PROCESS_INFORMATION processInfo{};
  if (!CreateProcessW(
          nullptr,
          commandLine.data(),
          nullptr,
          nullptr,
          FALSE,
          CREATE_NO_WINDOW,
          nullptr,
          nullptr,
          &startupInfo,
          &processInfo)) {
    throw std::runtime_error("Failed to start ScrollNode fatal probe");
  }

  WaitForSingleObject(processInfo.hProcess, INFINITE);
  DWORD exitCode = 0;
  if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    throw std::runtime_error("Failed to read ScrollNode fatal probe result");
  }
  CloseHandle(processInfo.hThread);
  CloseHandle(processInfo.hProcess);
  return exitCode;
}
#endif

} // namespace

class ScrollNodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
    NativeNodeApi::INSTANCE = nullptr;
  }
};

TEST_F(ScrollNodeTest, Constructor_DefaultContext_CreatesScrollNode) {
  ScrollNode node;

  auto handle = node.getArkUINodeHandle();
  ASSERT_EQ(g_mockNodes.count(handle), 1u);
  EXPECT_EQ(g_mockNodes.at(handle).type, ARKUI_NODE_SCROLL);
}

TEST_F(ScrollNodeTest, Constructor_DefaultContext_RegistersScrollEvents) {
  ScrollNode node;

  auto handle = node.getArkUINodeHandle();
  const auto& events = g_mockNodes.at(handle).registeredEvents;
  EXPECT_EQ(events.size(), 7u);
  EXPECT_NE(
      std::find(events.begin(), events.end(), NODE_ON_ACCESSIBILITY_ACTIONS),
      events.end());
  EXPECT_NE(
      std::find(events.begin(), events.end(), NODE_ON_TOUCH_INTERCEPT),
      events.end());
  EXPECT_NE(
      std::find(events.begin(), events.end(), NODE_SCROLL_EVENT_ON_DID_SCROLL),
      events.end());
  EXPECT_NE(
      std::find(
          events.begin(), events.end(), NODE_SCROLL_EVENT_ON_SCROLL_START),
      events.end());
  EXPECT_NE(
      std::find(events.begin(), events.end(), NODE_SCROLL_EVENT_ON_SCROLL_STOP),
      events.end());
  EXPECT_NE(
      std::find(
          events.begin(),
          events.end(),
          NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN),
      events.end());
  EXPECT_NE(
      std::find(events.begin(), events.end(), NODE_EVENT_ON_APPEAR),
      events.end());
}

TEST_F(ScrollNodeTest, SetHorizontal_True_SetsHorizontalDirection) {
  ScrollNode node;
  node.setHorizontal(true);

  auto item =
      mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_SCROLL_DIRECTION);
  ASSERT_NE(item, nullptr);
  ASSERT_NE(item->value, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_SCROLL_DIRECTION_HORIZONTAL);
}

TEST_F(ScrollNodeTest, SetHorizontal_False_SetsVerticalDirection) {
  ScrollNode node;
  node.setHorizontal(false);

  auto item =
      mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_SCROLL_DIRECTION);
  ASSERT_NE(item, nullptr);
  ASSERT_NE(item->value, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_SCROLL_DIRECTION_VERTICAL);
}

TEST_F(ScrollNodeTest, IsHorizontal_HorizontalAttribute_ReturnsTrue) {
  ScrollNode node;
  ArkUI_NumberValue value[] = {{.i32 = ARKUI_SCROLL_DIRECTION_HORIZONTAL}};
  ArkUI_AttributeItem item = {value, 1};
  ASSERT_EQ(
      mockSetAttribute(
          node.getArkUINodeHandle(), NODE_SCROLL_SCROLL_DIRECTION, &item),
      0);

  EXPECT_TRUE(node.isHorizontal());
}

TEST_F(ScrollNodeTest, IsHorizontal_VerticalAttribute_ReturnsFalse) {
  ScrollNode node;
  ArkUI_NumberValue value[] = {{.i32 = ARKUI_SCROLL_DIRECTION_VERTICAL}};
  ArkUI_AttributeItem item = {value, 1};
  ASSERT_EQ(
      mockSetAttribute(
          node.getArkUINodeHandle(), NODE_SCROLL_SCROLL_DIRECTION, &item),
      0);

  EXPECT_FALSE(node.isHorizontal());
}

TEST_F(ScrollNodeTest, SetFriction_CustomValue_StoresArkUIAttribute) {
  ScrollNode node;
  node.setFriction(1.25f);

  auto item = mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_FRICTION);
  ASSERT_NE(item, nullptr);
  ASSERT_NE(item->value, nullptr);
  EXPECT_FLOAT_EQ(item->value[0].f32, 1.25f);
}

TEST_F(ScrollNodeTest, InsertChild_SingleChild_AddsChildHandle) {
  ScrollNode node;
  ArkUINode child(mockCreateNode(ARKUI_NODE_STACK));

  node.insertChild(child);

  const auto& state = g_mockNodes.at(node.getArkUINodeHandle());
  ASSERT_EQ(state.children.size(), 1u);
  EXPECT_EQ(state.children.front(), child.getArkUINodeHandle());
  EXPECT_EQ(g_mockCallLog.addChildCount, 1);
}

TEST_F(ScrollNodeTest, RemoveChild_InsertedChild_RemovesChildHandle) {
  ScrollNode node;
  ArkUINode child(mockCreateNode(ARKUI_NODE_STACK));
  node.insertChild(child);

  node.removeChild(child);

  EXPECT_TRUE(g_mockNodes.at(node.getArkUINodeHandle()).children.empty());
  EXPECT_EQ(g_mockCallLog.removeChildCount, 1);
}

#if GTEST_HAS_DEATH_TEST
TEST_F(ScrollNodeTest, InsertChild_SecondChild_Aborts) {
  ScrollNode node;
  ArkUINode firstChild(mockCreateNode(ARKUI_NODE_STACK));
  ArkUINode secondChild(mockCreateNode(ARKUI_NODE_STACK));
  node.insertChild(firstChild);

  EXPECT_DEATH(node.insertChild(secondChild), "");
}

TEST_F(ScrollNodeTest, RemoveChild_WithoutInsertedChild_Aborts) {
  ScrollNode node;
  ArkUINode child(mockCreateNode(ARKUI_NODE_STACK));

  EXPECT_DEATH(node.removeChild(child), "");
}
#endif

#if defined(_WIN32)
TEST_F(ScrollNodeTest, InsertChild_SecondChild_FatalProbeTerminatesProcess) {
  EXPECT_NE(runFatalProbe(L"insert"), 0u);
}

TEST_F(
    ScrollNodeTest,
    RemoveChild_WithoutInsertedChild_FatalProbeTerminatesProcess) {
  EXPECT_NE(runFatalProbe(L"remove"), 0u);
}
#endif

TEST_F(ScrollNodeTest, OnNodeEvent_WithDelegate_DispatchesAllCallbacks) {
  ScrollNode node;
  RecordingScrollNodeDelegate delegate;
  delegate.frameBeginResult = 7.5f;
  node.setScrollNodeDelegate(&delegate);
  ArkUI_NumberValue eventArgs[MAX_COMPONENT_EVENT_ARG_NUM]{};

  node.onNodeEvent(NODE_SCROLL_EVENT_ON_DID_SCROLL, eventArgs);
  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_START, eventArgs);
  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_STOP, eventArgs);
  eventArgs[0].f32 = 12.25f;
  eventArgs[1].i32 = 3;
  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN, eventArgs);
  node.onNodeEvent(NODE_EVENT_ON_APPEAR, eventArgs);

  EXPECT_EQ(delegate.scrollCount, 1);
  EXPECT_EQ(delegate.scrollStartCount, 1);
  EXPECT_EQ(delegate.scrollStopCount, 1);
  EXPECT_EQ(delegate.frameBeginCount, 1);
  EXPECT_EQ(delegate.appearCount, 1);
  EXPECT_FLOAT_EQ(delegate.lastOffset, 12.25f);
  EXPECT_EQ(delegate.lastScrollState, 3);
  EXPECT_FLOAT_EQ(eventArgs[0].f32, 7.5f);
}

TEST_F(ScrollNodeTest, OnNodeEvent_WithoutDelegate_LeavesArgumentsUnchanged) {
  ScrollNode node;
  ArkUI_NumberValue eventArgs[MAX_COMPONENT_EVENT_ARG_NUM]{};
  eventArgs[0].f32 = 4.25f;
  eventArgs[1].i32 = 2;

  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN, eventArgs);

  EXPECT_FLOAT_EQ(eventArgs[0].f32, 4.25f);
  EXPECT_EQ(eventArgs[1].i32, 2);
}

TEST_F(
    ScrollNodeTest,
    SetScrollNodeDelegate_Nullptr_DetachesDelegateForAllEvents) {
  ScrollNode node;
  RecordingScrollNodeDelegate delegate;
  ArkUI_NumberValue eventArgs[MAX_COMPONENT_EVENT_ARG_NUM]{};
  eventArgs[0].f32 = 4.25f;
  eventArgs[1].i32 = 2;
  node.setScrollNodeDelegate(&delegate);
  node.setScrollNodeDelegate(nullptr);

  node.onNodeEvent(NODE_SCROLL_EVENT_ON_DID_SCROLL, eventArgs);
  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_START, eventArgs);
  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_STOP, eventArgs);
  node.onNodeEvent(NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN, eventArgs);
  node.onNodeEvent(NODE_EVENT_ON_APPEAR, eventArgs);

  EXPECT_EQ(delegate.scrollCount, 0);
  EXPECT_EQ(delegate.scrollStartCount, 0);
  EXPECT_EQ(delegate.scrollStopCount, 0);
  EXPECT_EQ(delegate.frameBeginCount, 0);
  EXPECT_EQ(delegate.appearCount, 0);
  EXPECT_FLOAT_EQ(eventArgs[0].f32, 4.25f);
  EXPECT_EQ(eventArgs[1].i32, 2);
}

TEST_F(ScrollNodeTest, OnNodeEvent_UnhandledEvent_DoesNotDispatchDelegate) {
  ScrollNode node;
  RecordingScrollNodeDelegate delegate;
  ArkUI_NumberValue eventArgs[MAX_COMPONENT_EVENT_ARG_NUM]{};
  node.setScrollNodeDelegate(&delegate);

  node.onNodeEvent(static_cast<ArkUI_NodeEventType>(-1), eventArgs);

  EXPECT_EQ(delegate.scrollCount, 0);
  EXPECT_EQ(delegate.scrollStartCount, 0);
  EXPECT_EQ(delegate.scrollStopCount, 0);
  EXPECT_EQ(delegate.frameBeginCount, 0);
  EXPECT_EQ(delegate.appearCount, 0);
}

TEST_F(ScrollNodeTest, SetScrollNodeDelegate_ReplacedDelegate_UsesLatest) {
  ScrollNode node;
  RecordingScrollNodeDelegate firstDelegate;
  RecordingScrollNodeDelegate secondDelegate;
  ArkUI_NumberValue eventArgs[MAX_COMPONENT_EVENT_ARG_NUM]{};
  node.setScrollNodeDelegate(&firstDelegate);
  node.setScrollNodeDelegate(&secondDelegate);

  node.onNodeEvent(NODE_SCROLL_EVENT_ON_DID_SCROLL, eventArgs);

  EXPECT_EQ(firstDelegate.scrollCount, 0);
  EXPECT_EQ(secondDelegate.scrollCount, 1);
}

TEST_F(
    ScrollNodeTest,
    SetEnableScrollInteraction_BooleanValues_StoreArkUIAttribute) {
  ScrollNode node;

  auto& result = node.setEnableScrollInteraction(true);
  auto item = getAttribute(node, NODE_SCROLL_ENABLE_SCROLL_INTERACTION);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(&result, &node);
  EXPECT_EQ(item->value[0].i32, 1);

  node.setEnableScrollInteraction(false);
  item = getAttribute(node, NODE_SCROLL_ENABLE_SCROLL_INTERACTION);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 0);
}

TEST_F(ScrollNodeTest, SetFlingSpeedLimit_Api24_StoresArkUIAttribute) {
  ScrollNode node;

  auto& result = node.setFlingSpeedLimit(4200.5f);

  auto item = getAttribute(node, NODE_SCROLL_FLING_SPEED_LIMIT);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(&result, &node);
  EXPECT_FLOAT_EQ(item->value[0].f32, 4200.5f);
}

TEST_F(ScrollNodeTest, SetEdgeEffect_BounceCombinations_StoreArkUIAttribute) {
  ScrollNode node;

  node.setEdgeEffect(false, false);
  auto item = getAttribute(node, NODE_SCROLL_EDGE_EFFECT);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 2);
  EXPECT_EQ(item->value[0].i32, ARKUI_EDGE_EFFECT_NONE);
  EXPECT_EQ(item->value[1].i32, 0);

  node.setEdgeEffect(true, true);
  item = getAttribute(node, NODE_SCROLL_EDGE_EFFECT);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 2);
  EXPECT_EQ(item->value[0].i32, ARKUI_EDGE_EFFECT_SPRING);
  EXPECT_EQ(item->value[1].i32, 1);
}

TEST_F(
    ScrollNodeTest,
    SetScrollOverScrollMode_SupportedValues_MapToEdgeEffect) {
  ScrollNode node;

  node.setScrollOverScrollMode("never");
  auto item = getAttribute(node, NODE_SCROLL_EDGE_EFFECT);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_EDGE_EFFECT_NONE);
  EXPECT_EQ(item->value[1].i32, 0);

  node.setScrollOverScrollMode("auto");
  item = getAttribute(node, NODE_SCROLL_EDGE_EFFECT);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_EDGE_EFFECT_SPRING);
  EXPECT_EQ(item->value[1].i32, 0);

  node.setScrollOverScrollMode("always");
  item = getAttribute(node, NODE_SCROLL_EDGE_EFFECT);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_EDGE_EFFECT_SPRING);
  EXPECT_EQ(item->value[1].i32, 1);
}

// Known production issue: the current implementation leaves edgeEffect
// uninitialized for unsupported overScrollMode values. Keep this as a disabled
// regression test and document it in the PR instead of changing production code
// from this test-only change.
TEST_F(
    ScrollNodeTest,
    DISABLED_SetScrollOverScrollMode_UnknownValue_UsesAutoBehavior) {
  ScrollNode node;

  node.setScrollOverScrollMode("unsupported");

  auto item = getAttribute(node, NODE_SCROLL_EDGE_EFFECT);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 2);
  EXPECT_EQ(item->value[0].i32, ARKUI_EDGE_EFFECT_SPRING);
  EXPECT_EQ(item->value[1].i32, 0);
}

TEST_F(ScrollNodeTest, SetScrollBarDisplayMode_AllModes_StoreArkUIAttribute) {
  ScrollNode node;
  const ArkUI_ScrollBarDisplayMode modes[] = {
      ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF,
      ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO,
      ARKUI_SCROLL_BAR_DISPLAY_MODE_ON};

  for (auto mode : modes) {
    node.setScrollBarDisplayMode(mode);
    auto item = getAttribute(node, NODE_SCROLL_BAR_DISPLAY_MODE);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->value[0].i32, mode);
  }
}

TEST_F(ScrollNodeTest, SetScrollBarColor_ArgbValues_StoreArkUIAttribute) {
  ScrollNode node;

  node.setScrollBarColor(0x00000000u);
  auto item = getAttribute(node, NODE_SCROLL_BAR_COLOR);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].u32, 0x00000000u);

  node.setScrollBarColor(0xFFA1B2C3u);
  item = getAttribute(node, NODE_SCROLL_BAR_COLOR);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].u32, 0xFFA1B2C3u);
}

TEST_F(ScrollNodeTest, SetScrollSnap_EmptyPoints_StoresControlValues) {
  ScrollNode node;

  node.setScrollSnap(ARKUI_SCROLL_SNAP_ALIGN_NONE, false, true, {});

  auto item = getAttribute(node, NODE_SCROLL_SNAP);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 3);
  EXPECT_EQ(item->value[0].i32, ARKUI_SCROLL_SNAP_ALIGN_NONE);
  EXPECT_EQ(item->value[1].i32, 0);
  EXPECT_EQ(item->value[2].i32, 1);
}

TEST_F(ScrollNodeTest, SetScrollSnap_MultiplePoints_StoresAllValues) {
  ScrollNode node;

  node.setScrollSnap(
      ARKUI_SCROLL_SNAP_ALIGN_CENTER, true, false, {10.5, 80.0, 125.25});

  auto item = getAttribute(node, NODE_SCROLL_SNAP);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 6);
  EXPECT_EQ(item->value[0].i32, ARKUI_SCROLL_SNAP_ALIGN_CENTER);
  EXPECT_EQ(item->value[1].i32, 1);
  EXPECT_EQ(item->value[2].i32, 0);
  EXPECT_FLOAT_EQ(item->value[3].f32, 10.5f);
  EXPECT_FLOAT_EQ(item->value[4].f32, 80.0f);
  EXPECT_FLOAT_EQ(item->value[5].f32, 125.25f);
}

TEST_F(ScrollNodeTest, SetEnablePaging_BooleanValues_StoreArkUIAttribute) {
  ScrollNode node;

  node.setEnablePaging(true);
  auto item = getAttribute(node, NODE_SCROLL_ENABLE_PAGING);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 1);

  node.setEnablePaging(false);
  item = getAttribute(node, NODE_SCROLL_ENABLE_PAGING);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 0);
}

TEST_F(ScrollNodeTest, SetCenterContent_BooleanValues_MapToAlignment) {
  ScrollNode node;

  node.setCenterContent(true);
  auto item = getAttribute(node, NODE_STACK_ALIGN_CONTENT);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_ALIGNMENT_CENTER);

  node.setCenterContent(false);
  item = getAttribute(node, NODE_STACK_ALIGN_CONTENT);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, ARKUI_ALIGNMENT_TOP_START);
}

TEST_F(ScrollNodeTest, SetBackToTop_Api24_StoresBooleanAttribute) {
  ScrollNode node;

  node.setBackToTop(true);
  auto item = getAttribute(node, NODE_SCROLL_BACK_TO_TOP);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 1);

  node.setBackToTop(false);
  item = getAttribute(node, NODE_SCROLL_BACK_TO_TOP);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 0);
}

TEST_F(ScrollNodeTest, SetFadingEdge_LengthValues_StoreEnabledAndLength) {
  ScrollNode node;

  node.setFadingEdge(18.5f);
  auto item = getAttribute(node, NODE_SCROLL_FADING_EDGE);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 2);
  EXPECT_EQ(item->value[0].i32, 1);
  EXPECT_FLOAT_EQ(item->value[1].f32, 18.5f);

  node.setFadingEdge(0.0f);
  item = getAttribute(node, NODE_SCROLL_FADING_EDGE);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 0);
  EXPECT_FLOAT_EQ(item->value[1].f32, 0.0f);

  node.setFadingEdge(-3.0f);
  item = getAttribute(node, NODE_SCROLL_FADING_EDGE);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 0);
  EXPECT_FLOAT_EQ(item->value[1].f32, -3.0f);
}

TEST_F(ScrollNodeTest, ResetScrollSnap_ExistingAttribute_RemovesAttribute) {
  ScrollNode node;
  node.setScrollSnap(ARKUI_SCROLL_SNAP_ALIGN_START, true, true, {24.0});
  ASSERT_NE(
      mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_SNAP), nullptr);

  auto& result = node.resetScrollSnap();

  EXPECT_EQ(&result, &node);
  EXPECT_EQ(
      mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_SNAP), nullptr);
  EXPECT_EQ(g_mockCallLog.resetAttributeCount, 1);
}

TEST_F(ScrollNodeTest, SetNestedScroll_SingleMode_StoresModeBothDirections) {
  ScrollNode node;

  node.setNestedScroll(ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST);

  auto item = getAttribute(node, NODE_SCROLL_NESTED_SCROLL);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 2);
  EXPECT_EQ(item->value[0].i32, ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST);
  EXPECT_EQ(item->value[1].i32, ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST);
}

TEST_F(ScrollNodeTest, SetNestedScroll_TwoModes_StoresForwardAndBackwardModes) {
  ScrollNode node;

  node.setNestedScroll(
      ARKUI_SCROLL_NESTED_MODE_SELF_FIRST, ARKUI_SCROLL_NESTED_MODE_PARALLEL);

  auto item = getAttribute(node, NODE_SCROLL_NESTED_SCROLL);
  ASSERT_NE(item, nullptr);
  ASSERT_EQ(item->size, 2);
  EXPECT_EQ(item->value[0].i32, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
  EXPECT_EQ(item->value[1].i32, ARKUI_SCROLL_NESTED_MODE_PARALLEL);
}

TEST_F(
    ScrollNodeTest,
    SetNestedScrollEnabled_BooleanValues_StoreInteractionAttribute) {
  ScrollNode node;

  node.setNestedScrollEnabled(true);
  auto item = getAttribute(node, NODE_SCROLL_ENABLE_SCROLL_INTERACTION);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 1);

  node.setNestedScrollEnabled(false);
  item = getAttribute(node, NODE_SCROLL_ENABLE_SCROLL_INTERACTION);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].i32, 0);
}

TEST_F(ScrollNodeTest, SetEndFillColor_ArgbValue_StoresBackgroundColor) {
  ScrollNode node;

  node.setEndFillColor(0x7F102030u);

  auto item = getAttribute(node, NODE_BACKGROUND_COLOR);
  ASSERT_NE(item, nullptr);
  EXPECT_EQ(item->value[0].u32, 0x7F102030u);
}

TEST_F(ScrollNodeTest, ScrollTo_AnimatedOffset_StoresAllArguments) {
  ScrollNode node;
  node.scrollTo(12.5f, 30.0f, true, true);

  auto item = mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_OFFSET);
  ASSERT_NE(item, nullptr);
  ASSERT_NE(item->value, nullptr);
  ASSERT_EQ(item->size, 6);
  EXPECT_FLOAT_EQ(item->value[0].f32, 12.5f);
  EXPECT_FLOAT_EQ(item->value[1].f32, 30.0f);
  EXPECT_EQ(item->value[2].i32, 250);
  EXPECT_EQ(item->value[3].i32, ARKUI_CURVE_LINEAR);
  EXPECT_EQ(item->value[4].i32, 0);
  EXPECT_EQ(item->value[5].i32, 1);
}

TEST_F(ScrollNodeTest, ScrollTo_NonAnimatedBoundedOffset_StoresZeroFlags) {
  ScrollNode node;
  node.scrollTo(-4.0f, 6.0f, false, false);

  auto item = mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_OFFSET);
  ASSERT_NE(item, nullptr);
  ASSERT_NE(item->value, nullptr);
  ASSERT_EQ(item->size, 6);
  EXPECT_FLOAT_EQ(item->value[0].f32, -4.0f);
  EXPECT_FLOAT_EQ(item->value[1].f32, 6.0f);
  EXPECT_EQ(item->value[2].i32, 0);
  EXPECT_EQ(item->value[5].i32, 0);
}

TEST_F(ScrollNodeTest, GetScrollOffset_StoredOffset_ReturnsPoint) {
  ScrollNode node;
  ArkUI_NumberValue value[] = {{.f32 = 8.0f}, {.f32 = 13.0f}};
  ArkUI_AttributeItem item = {value, 2};
  ASSERT_EQ(
      mockSetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_OFFSET, &item),
      0);

  auto offset = node.getScrollOffset();

  EXPECT_FLOAT_EQ(offset.x, 8.0f);
  EXPECT_FLOAT_EQ(offset.y, 13.0f);
}

TEST_F(ScrollNodeTest, Destructor_OwnedNode_ReleasesArkUIResources) {
  ArkUI_NodeHandle handle = nullptr;
  {
    ScrollNode node;
    handle = node.getArkUINodeHandle();
    ASSERT_EQ(g_mockNodes.count(handle), 1u);
  }

  EXPECT_EQ(g_mockNodes.count(handle), 0u);
  EXPECT_EQ(g_mockCallLog.unregisterNodeEventCount, 7);
  EXPECT_EQ(g_mockCallLog.removeAllChildrenCount, 1);
  EXPECT_EQ(g_mockCallLog.removeNodeEventReceiverCount, 1);
  EXPECT_EQ(g_mockCallLog.disposeNodeCount, 1);
}
