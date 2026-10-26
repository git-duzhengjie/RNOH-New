/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests for rnoh::TouchEventDispatcher.
//
// Two groups:
//   1. Offset management (updateOffset/getOffsetForNode/removeOffset) — pure
//      map operations, no platform dependency.
//   2. dispatchTouchEvent end-to-end routing — constructs rnoh::TouchEvent
//      via the ArkUI_UIInputEvent ctor (stubbed) + field assignment (avoiding
//      folly), drives a MockTouchTarget tree, and asserts routing/cancel
//      paths don't crash. The emitter (MockTouchEventEmitter) is a no-op
//      stub (empty EventDispatcher), so onTouch* become harmless; these are
//      routing smoke tests, not emitter-call assertions.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "RNOH/arkui/TouchEventDispatcher.h"
#include "RNOH/tests/MockTouchEventEmitter.h"
#include "RNOH/tests/MockTouchTarget.h"

using namespace rnoh;
using facebook::react::LayoutMetrics;
using facebook::react::Point;
using facebook::react::Rect;
using facebook::react::Size;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

constexpr int32_t kNodeId = 1;

LayoutMetrics makeLayout(Point origin, Size size) {
  LayoutMetrics lm;
  lm.frame = Rect{origin, size};
  return lm;
}

// Builds a rnoh::TouchEvent by invoking the ArkUI_UIInputEvent ctor (which
// uses stubbed OH_ArkUI_* no-ops) and then overwriting the public fields.
// This avoids constructing a folly::dynamic while staying compatible with
// the struct's only non-folly entry point.
rnoh::TouchEvent makeTouchEvent(
    uint32_t action, int32_t id, int32_t x, int32_t y) {
  rnoh::TouchEvent te(nullptr);
  te.action = action;
  te.timestamp = 1000000000ull; // 1s in ns
  te.activeTouchPoints.clear();
  te.activeTouchPoints.push_back({id, 0.0f, x, y, x, y});
  return te;
}

// Builds a stub ArkUI_UIInputEvent (host-constructible) to drive the
// dispatchTouchEvent(ArkUI_UIInputEvent*) overload end-to-end through the
// real TouchEvent(event) ctor + findTargetAndSendTouchEvent path.
ArkUI_UIInputEvent makeArkUIEvent(
    uint32_t action, int32_t id, int32_t x, int32_t y) {
  ArkUI_UIInputEvent ev;
  ev.action = action;
  ev.timestamp = 1000000000ull;
  ev.pointers.push_back(
      {id, 0.0f, static_cast<float>(x), static_cast<float>(y),
       static_cast<float>(x), static_cast<float>(y)});
  return ev;
}

} // namespace

// ===========================================================================
// Group 1: offset management (pure map logic)
// ===========================================================================

class TouchEventDispatcherOffsetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // m_nodeOffsets is a file-level global (TouchEventDispatcher.cpp:16);
    // clearing this id in SetUp isolates tests from each other.
    dispatcher.removeOffset(kNodeId);
  }
};

TEST_F(TouchEventDispatcherOffsetTest, UpdateOffset_NewNode_StoresOffset) {
  dispatcher.updateOffset(kNodeId, 10.0f);
  EXPECT_FLOAT_EQ(dispatcher.getOffsetForNode(kNodeId), 10.0f);
}

TEST_F(TouchEventDispatcherOffsetTest,
       UpdateOffset_ExistingNode_AccumulatesOffset) {
  dispatcher.updateOffset(kNodeId, 10.0f);
  dispatcher.updateOffset(kNodeId, 5.0f);
  EXPECT_FLOAT_EQ(dispatcher.getOffsetForNode(kNodeId), 15.0f);
}

TEST_F(TouchEventDispatcherOffsetTest,
       GetOffsetForNode_UnknownNode_ReturnsZero) {
  EXPECT_FLOAT_EQ(dispatcher.getOffsetForNode(999999), 0.0f);
}

TEST_F(TouchEventDispatcherOffsetTest,
       GetOffsetForNode_KnownNode_ReturnsValue) {
  dispatcher.updateOffset(kNodeId, 42.5f);
  EXPECT_FLOAT_EQ(dispatcher.getOffsetForNode(kNodeId), 42.5f);
}

TEST_F(TouchEventDispatcherOffsetTest, RemoveOffset_KnownNode_ClearsEntry) {
  dispatcher.updateOffset(kNodeId, 7.0f);
  dispatcher.removeOffset(kNodeId);
  EXPECT_FLOAT_EQ(dispatcher.getOffsetForNode(kNodeId), 0.0f);
}

TEST_F(TouchEventDispatcherOffsetTest, RemoveOffset_UnknownNode_NoThrow) {
  EXPECT_NO_THROW(dispatcher.removeOffset(888888));
}

// ===========================================================================
// Group 2: dispatchTouchEvent routing (smoke / no-crash)
// ===========================================================================

class TouchEventDispatcherDispatchTest : public ::testing::Test {
 protected:
  void SetUp() override {
    emitter_ = std::make_shared<MockTouchEventEmitter>();
    root_ = std::make_shared<NiceMock<MockTouchTarget>>();
    child_ = std::make_shared<NiceMock<MockTouchTarget>>();
    // The dispatcher's m_touchTargetByTouchId and the root<->child parent/
    // children cycle retain the mocks; allow leaks to avoid spurious gmock
    // errors (the process is short-lived).
    ::testing::Mock::AllowLeak(root_.get());
    ::testing::Mock::AllowLeak(child_.get());

    // Root: allows traversal into children, contains the point in its
    // bounding box, exposes the child, and is marked as a root touch target
    // (so it is skipped when recursing into children).
    ON_CALL(*root_, canChildrenHandleTouch()).WillByDefault(Return(true));
    ON_CALL(*root_, containsPointInBoundingBox(_)).WillByDefault(Return(true));
    ON_CALL(*root_, getTouchTargetChildren())
        .WillByDefault(Return(std::vector<TouchTarget::Shared>{child_}));
    ON_CALL(*root_, isRootTouchTarget()).WillByDefault(Return(true));
    ON_CALL(*root_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));
    ON_CALL(*root_, getTouchTargetTag()).WillByDefault(Return(2));

    // Child: handles touches, contains the (computed) local point, owns the
    // emitter, and reports the root as its parent. Identity transform +
    // origin {0,0} means computeChildPoint returns the point unchanged.
    ON_CALL(*child_, canHandleTouch()).WillByDefault(Return(true));
    ON_CALL(*child_, containsPoint(_)).WillByDefault(Return(true));
    ON_CALL(*child_, getTouchEventEmitter()).WillByDefault(Return(emitter_));
    ON_CALL(*child_, isRootTouchTarget()).WillByDefault(Return(false));
    ON_CALL(*child_, getTouchTargetTag()).WillByDefault(Return(1));
    ON_CALL(*child_, getTouchTargetParent()).WillByDefault(Return(root_));
    ON_CALL(*child_, getLayoutMetrics())
        .WillByDefault(Return(makeLayout(Point{0, 0}, Size{100, 100})));
    ON_CALL(*child_, getTransform())
        .WillByDefault(Return(facebook::react::Transform{}));
  }

  void TearDown() override {
    // Break the root<->child shared_ptr cycle (root holds child via
    // getTouchTargetChildren, child holds root via getTouchTargetParent) and
    // clear the dispatcher's touch-id map so mock objects are destroyed.
    ON_CALL(*root_, getTouchTargetChildren())
        .WillByDefault(Return(std::vector<TouchTarget::Shared>{}));
    ON_CALL(*child_, getTouchTargetParent())
        .WillByDefault(Return(nullptr));
    root_.reset();
    child_.reset();
  }

  // Release a registered touch id from the dispatcher's internal map by
  // dispatching a CANCEL for it. Harmless if the id was never registered.
  void releaseTouch(int32_t id) {
    dispatcher.dispatchTouchEvent(
        makeTouchEvent(UI_TOUCH_EVENT_ACTION_CANCEL, id, 50, 50), root_);
  }

  std::shared_ptr<MockTouchEventEmitter> emitter_;
  std::shared_ptr<NiceMock<MockTouchTarget>> root_;
  std::shared_ptr<NiceMock<MockTouchTarget>> child_;
};

// cancelActiveTouches with no active touches (m_previousEvent.touches empty)
// takes the `if (size() > 0)` false branch — a no-op. Declared first in the
// dispatch fixture so it runs before any DOWN sets m_previousEvent.
TEST_F(TouchEventDispatcherDispatchTest,
       CancelActiveTouches_NoActive_DoesNotCrash) {
  EXPECT_NO_THROW(dispatcher.cancelActiveTouches());
}

// DOWN reaches the child target and is routed through sendEvent without
// crashing, even though the emitter is a no-op stub.
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_Down_WithTarget_DoesNotCrash) {
  constexpr int32_t id = 100;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  releaseTouch(id);
}

// No target at the touch point: findTargetForTouchPoint returns nullptr and
// dispatch is a no-op.
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_Down_NoTargetAtPoint_DoesNotCrash) {
  ON_CALL(*child_, containsPoint(_)).WillByDefault(Return(false));
  constexpr int32_t id = 101;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  releaseTouch(id);
}

// A full DOWN -> UP lifecycle on the same touch id completes without crashing.
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_DownUp_Lifecycle_DoesNotCrash) {
  constexpr int32_t id = 102;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_UP, id, 50, 50), root_));
}

// DOWN -> MOVE -> UP lifecycle; MOVE exercises canIgnoreMoveEvent's branch
// (previous event set by the DOWN).
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_DownMoveUp_Lifecycle_DoesNotCrash) {
  constexpr int32_t id = 103;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_MOVE, id, 51, 50), root_));
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_UP, id, 51, 50), root_));
}

// MOVE with displacement >1px exercises canIgnoreMoveEvent's "return false"
// (don't ignore, actually dispatch) branch — complementing Dispatch_DownMoveUp
// whose 1px MOVE takes the "ignore" (return true) path.
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_Move_LargeDisplacement_DoesNotCrash) {
  constexpr int32_t id = 106;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_MOVE, id, 80, 50), root_));
  releaseTouch(id);
}

// A touch target whose own isHandlingTouches() is true causes the dispatcher
// to cancel the touch via cancelPreviousTouchEvent; verify that path doesn't
// crash. (isAncestorHandlingTouches checks the target chain, not the root.)
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_AncestorHandlingTouches_DoesNotCrash) {
  ON_CALL(*child_, isHandlingTouches()).WillByDefault(Return(true));
  constexpr int32_t id = 104;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  releaseTouch(id);
}

// cancelActiveTouches after an active DOWN emits a CANCEL without crashing.
TEST_F(TouchEventDispatcherDispatchTest,
       CancelActiveTouches_WithActive_DoesNotCrash) {
  constexpr int32_t id = 105;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  EXPECT_NO_THROW(dispatcher.cancelActiveTouches());
}

// ---- dispatchTouchEvent(ArkUI_UIInputEvent*) overload ----
// These drive the ArkUI_UIInputEvent* overload through the real
// TouchEvent(event) ctor + findTargetAndSendTouchEvent path (covered by
// neither the const TouchEvent& overload nor TouchEventTest).

TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_ArkUIInputEvent_Down_DoesNotCrash) {
  ArkUI_UIInputEvent ev = makeArkUIEvent(UI_TOUCH_EVENT_ACTION_DOWN, 200, 50, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&ev, root_));
  ArkUI_UIInputEvent cancel =
      makeArkUIEvent(UI_TOUCH_EVENT_ACTION_CANCEL, 200, 50, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&cancel, root_));
}

TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_ArkUIInputEvent_DownUp_Lifecycle_DoesNotCrash) {
  ArkUI_UIInputEvent down =
      makeArkUIEvent(UI_TOUCH_EVENT_ACTION_DOWN, 201, 50, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&down, root_));
  ArkUI_UIInputEvent up =
      makeArkUIEvent(UI_TOUCH_EVENT_ACTION_UP, 201, 50, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&up, root_));
}

TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_ArkUIInputEvent_DownMoveUp_Lifecycle_DoesNotCrash) {
  constexpr int32_t id = 202;
  ArkUI_UIInputEvent down = makeArkUIEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&down, root_));
  ArkUI_UIInputEvent move = makeArkUIEvent(UI_TOUCH_EVENT_ACTION_MOVE, id, 60, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&move, root_));
  ArkUI_UIInputEvent up = makeArkUIEvent(UI_TOUCH_EVENT_ACTION_UP, id, 60, 50);
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(&up, root_));
}

// DOWN registers the target (child not handling), then the child starts
// handling touches: a subsequent MOVE on the same id triggers the
// non-DOWN ancestor-handling branch -> cancelPreviousTouchEvent + erase.
// This is the only path covering cancelPreviousTouchEvent (TouchEventDispatcher
// .cpp:298), previously 0% covered.
TEST_F(TouchEventDispatcherDispatchTest,
       Dispatch_Move_AncestorHandlingTriggersCancel_DoesNotCrash) {
  constexpr int32_t id = 203;
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_DOWN, id, 50, 50), root_));
  // Child now handles touches -> MOVE on the registered id must cancel it.
  ON_CALL(*child_, isHandlingTouches()).WillByDefault(Return(true));
  EXPECT_NO_THROW(dispatcher.dispatchTouchEvent(
      makeTouchEvent(UI_TOUCH_EVENT_ACTION_MOVE, id, 60, 50), root_));
}
