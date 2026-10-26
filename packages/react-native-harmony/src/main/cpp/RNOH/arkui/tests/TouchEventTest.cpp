/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests for rnoh::TouchEvent ArkUI_UIInputEvent constructor.
//
// The stub arkui/ui_input_event.h defines ArkUI_UIInputEvent as a host
// struct; we populate it, run it through rnoh::TouchEvent's ctor, and assert
// the parsed action/timestamp and the generated touch identifiers (mouse id
// maps to 0, touch id maps to id+1 via TOUCH_IDENTIFIER_POOL_OFFSET).

#include <gtest/gtest.h>
#include "RNOH/arkui/TouchEvent.h"
#include "arkui/ui_input_event.h"

using namespace rnoh;

namespace {
// ARKUI_MOUSE_POINTER_ID (TouchEvent.h:15 = 1001) maps to RN_MOUSE_POINTER_ID
// (0); any other pointerId maps to pointerId + TOUCH_IDENTIFIER_POOL_OFFSET
// (1).
constexpr int32_t kMousePointerId = 1001;

ArkUI_UIInputEvent makeEvent(uint32_t action, uint64_t timestamp) {
  ArkUI_UIInputEvent ev;
  ev.action = action;
  ev.timestamp = timestamp;
  return ev;
}

void addPointer(
    ArkUI_UIInputEvent& ev,
    int32_t id,
    float pressure,
    float x,
    float y,
    float displayX,
    float displayY) {
  ev.pointers.push_back({id, pressure, x, y, displayX, displayY});
}
} // namespace

TEST(TouchEvent, Down_SingleTouch_ParsesActionAndTimestamp) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_DOWN, 1'000'000'000ull);
  addPointer(ev, 5, 0.5f, 10.0f, 20.0f, 30.0f, 40.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.action, UI_TOUCH_EVENT_ACTION_DOWN);
  EXPECT_EQ(te.timestamp, 1'000'000'000ull);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
}

TEST(TouchEvent, Down_SingleTouch_GeneratesTouchIdentifier) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_DOWN, 0);
  addPointer(ev, 5, 0.5f, 10.0f, 20.0f, 30.0f, 40.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  // pointerId 5 -> 5 + TOUCH_IDENTIFIER_POOL_OFFSET(1) = 6
  EXPECT_EQ(te.activeTouchPoints[0].id, 6);
  EXPECT_EQ(te.activeTouchPoints[0].nodeX, 10);
  EXPECT_EQ(te.activeTouchPoints[0].nodeY, 20);
  EXPECT_EQ(te.activeTouchPoints[0].screenX, 30);
  EXPECT_EQ(te.activeTouchPoints[0].screenY, 40);
  EXPECT_FLOAT_EQ(te.activeTouchPoints[0].force, 0.5f);
}

TEST(TouchEvent, Down_MousePointer_MapsToZeroIdentifier) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_DOWN, 0);
  addPointer(ev, kMousePointerId, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  // mouse pointerId 1001 -> RN_MOUSE_POINTER_ID (0)
  EXPECT_EQ(te.activeTouchPoints[0].id, 0);
}

TEST(TouchEvent, Up_SingleTouch_ParsesUpAction) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_UP, 0);
  addPointer(ev, 3, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.action, UI_TOUCH_EVENT_ACTION_UP);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  // pointerId 3 -> 4
  EXPECT_EQ(te.activeTouchPoints[0].id, 4);
}

TEST(TouchEvent, Cancel_SingleTouch_ParsesCancelAction) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_CANCEL, 0);
  addPointer(ev, 7, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.action, UI_TOUCH_EVENT_ACTION_CANCEL);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  // pointerId 7 -> 8
  EXPECT_EQ(te.activeTouchPoints[0].id, 8);
}

TEST(TouchEvent, Move_MultipleTouches_ReturnsAllPoints) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_MOVE, 2'000'000'000ull);
  addPointer(ev, 1, 0.1f, 5.0f, 6.0f, 7.0f, 8.0f);
  addPointer(ev, 2, 0.2f, 15.0f, 16.0f, 17.0f, 18.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.action, UI_TOUCH_EVENT_ACTION_MOVE);
  EXPECT_EQ(te.timestamp, 2'000'000'000ull);
  // MOVE uses getTouchesFromUIInputEvent: all pointers, in order
  EXPECT_EQ(te.activeTouchPoints.size(), 2u);
  EXPECT_EQ(te.activeTouchPoints[0].id, 2); // 1 + 1
  EXPECT_EQ(te.activeTouchPoints[1].id, 3); // 2 + 1
  EXPECT_FLOAT_EQ(te.activeTouchPoints[1].force, 0.2f);
}

TEST(TouchEvent, Down_ActiveTouch_MatchesByDisplayXY) {
  // Two pointers; the active touch is the one whose displayX/Y match
  // GetDisplayX/GetDisplayY (driven by activeIdx). Verify the active touch
  // (index 1, id=9) is selected, not the first pointer.
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_DOWN, 0);
  ev.activeIdx = 1;
  addPointer(ev, 5, 0.0f, 0.0f, 0.0f, 999.0f, 888.0f);  // not active
  addPointer(ev, 9, 0.9f, 11.0f, 22.0f, 100.0f, 200.0f); // active
  TouchEvent te(&ev);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  // active pointer id 9 -> 10
  EXPECT_EQ(te.activeTouchPoints[0].id, 10);
  EXPECT_EQ(te.activeTouchPoints[0].nodeX, 11);
  EXPECT_EQ(te.activeTouchPoints[0].nodeY, 22);
}

// MOVE with a single touch still uses getTouchesFromUIInputEvent (1 point).
TEST(TouchEvent, Move_SingleTouch_ReturnsOnePoint) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_MOVE, 0);
  addPointer(ev, 4, 0.3f, 1.0f, 2.0f, 3.0f, 4.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.action, UI_TOUCH_EVENT_ACTION_MOVE);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  EXPECT_EQ(te.activeTouchPoints[0].id, 5); // 4 + 1
}

// A pointerId of 0 (not the mouse id) maps to 0 + 1 = 1.
TEST(TouchEvent, Down_ZeroPointerId_GeneratesOne) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_DOWN, 0);
  addPointer(ev, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.activeTouchPoints.size(), 1u);
  EXPECT_EQ(te.activeTouchPoints[0].id, 1);
}

// MOVE mixing a mouse pointer (1001 -> 0) and a touch pointer (2 -> 3).
TEST(TouchEvent, Move_MixMouseAndTouch_GeneratesIdentifiers) {
  auto ev = makeEvent(UI_TOUCH_EVENT_ACTION_MOVE, 0);
  addPointer(ev, kMousePointerId, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  addPointer(ev, 2, 0.2f, 5.0f, 6.0f, 7.0f, 8.0f);
  TouchEvent te(&ev);
  EXPECT_EQ(te.activeTouchPoints.size(), 2u);
  EXPECT_EQ(te.activeTouchPoints[0].id, 0); // mouse -> 0
  EXPECT_EQ(te.activeTouchPoints[1].id, 3); // 2 + 1
}
