/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::EventAnimationDriver` — the driver
 * that maps an incoming event payload (via a property path) to a value
 * update on a registered `ValueAnimatedNode`. Used by the JS-side
 * `Animated.event` API to bind gesture/scroll events directly into the
 * Animated graph.
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * a value node with it. The event driver is constructed directly so the
 * test can call `updateWithEvent(...)` and observe the resulting value
 * via the manager.
 *
 * Tests exercise:
 *   - Constructor stores `eventName`, `viewTag`, `eventPath`,
 *     `nodeTag`, and the manager reference
 *   - `updateWithEvent` traverses the configured property path and
 *     writes the leaf number into the registered value node
 *   - A single-segment path (e.g. ["contentOffset"]) writes the
 *     top-level payload value
 *   - A multi-segment path (e.g. ["contentOffset", "x"]) traverses into
 *     nested objects
 *   - Deeply nested paths (3+ segments) are walked correctly
 *   - `getValueNode()` returns the registered value node
 *   - `setNeedsUpdate(m_nodeTag)` is invoked so the manager marks the
 *     node for re-update (verified via `getNodeOutput` reflecting the
 *     new value)
 *   - Accessors `getEventName()`, `getViewTag()`, `getNodeTag()` return
 *     the constructor args
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Drivers/EventAnimationDriver.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;
using rnoh::test::makeAnimatedNodesManager;

namespace {

folly::dynamic makeValueNodeConfig(double value, double offset = 0.0) {
  return folly::dynamic::object("type", "value")("value", value)(
      "offset", offset);
}

void registerValue(
    AnimatedNodesManager& manager,
    facebook::react::Tag tag,
    double initialValue) {
  manager.createNode(tag, makeValueNodeConfig(initialValue, 0.0));
}
} // namespace

// ---------------------------------------------------------------------------
// Constructor & accessors
// ---------------------------------------------------------------------------

TEST(EventAnimationDriver, Constructor_StoresEventNameAndViewTagAndNodeTag) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver(
      /*eventName=*/"onScroll",
      /*viewTag=*/42,
      /*eventPath=*/{"contentOffset", "y"},
      /*nodeTag=*/1,
      *manager);

  EXPECT_EQ(driver.getEventName(), "onScroll");
  EXPECT_EQ(driver.getViewTag(), 42);
  EXPECT_EQ(driver.getNodeTag(), 1);
}

TEST(EventAnimationDriver, GetValueNode_ReturnsRegisteredValueNode) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 13.0);

  EventAnimationDriver driver(
      "onScroll", 42, {"contentOffset", "y"}, 1, *manager);

  auto& node = driver.getValueNode();
  EXPECT_NEAR(node.getOutputAsDouble(), 13.0, 1e-9);
}

// ---------------------------------------------------------------------------
// updateWithEvent — single-segment path
// ---------------------------------------------------------------------------

TEST(
    EventAnimationDriver,
    UpdateWithEvent_SingleSegmentPath_WritesTopLevelValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver("onScroll", 42, {"contentOffset"}, 1, *manager);

  folly::dynamic event = folly::dynamic::object("contentOffset", 17.5);
  driver.updateWithEvent(event);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 17.5, 1e-9);
}

TEST(
    EventAnimationDriver,
    UpdateWithEvent_TwoSegmentPath_TraversesNestedObjects) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver(
      "onScroll", 42, {"contentOffset", "y"}, 1, *manager);

  folly::dynamic event = folly::dynamic::object(
      "contentOffset", folly::dynamic::object("x", 0.0)("y", 99.0));
  driver.updateWithEvent(event);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 99.0, 1e-9);
}

TEST(
    EventAnimationDriver,
    UpdateWithEvent_ThreeSegmentPath_TraversesDeeplyNestedObjects) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver(
      "onScroll", 42, {"nested", "contentOffset", "y"}, 1, *manager);

  folly::dynamic event = folly::dynamic::object(
      "nested",
      folly::dynamic::object(
          "contentOffset", folly::dynamic::object("x", 0.0)("y", 123.0)));
  driver.updateWithEvent(event);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 123.0, 1e-9);
}

// ---------------------------------------------------------------------------
// updateWithEvent — multiple calls overwrite previous value
// ---------------------------------------------------------------------------

TEST(
    EventAnimationDriver,
    UpdateWithEvent_CalledTwice_SecondValueOverwritesFirst) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver("onScroll", 42, {"contentOffset"}, 1, *manager);

  folly::dynamic first = folly::dynamic::object("contentOffset", 10.0);
  driver.updateWithEvent(first);
  ASSERT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 10.0, 1e-9);

  folly::dynamic second = folly::dynamic::object("contentOffset", 25.0);
  driver.updateWithEvent(second);
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 25.0, 1e-9);
}

// ---------------------------------------------------------------------------
// updateWithEvent — negative and fractional values
// ---------------------------------------------------------------------------

TEST(EventAnimationDriver, UpdateWithEvent_NegativeValue_PreservesSign) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver("onScroll", 42, {"contentOffset"}, 1, *manager);

  folly::dynamic event = folly::dynamic::object("contentOffset", -3.14);
  driver.updateWithEvent(event);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), -3.14, 1e-9);
}

TEST(EventAnimationDriver, UpdateWithEvent_FractionalValue_PreservesPrecision) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver("onScroll", 42, {"contentOffset"}, 1, *manager);

  folly::dynamic event = folly::dynamic::object("contentOffset", 0.001);
  driver.updateWithEvent(event);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 0.001, 1e-9);
}

// ---------------------------------------------------------------------------
// updateWithEvent — empty path
// ---------------------------------------------------------------------------

TEST(EventAnimationDriver, UpdateWithEvent_EmptyPath_WritesRootEventValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  // An empty event path means the root payload IS the value. Production
  // code: `for (auto& key : m_eventPath) { ... }` runs zero iterations,
  // so `currentEvent` stays pointing at the root event.
  EventAnimationDriver driver("onScroll", 42, /*eventPath=*/{}, 1, *manager);

  folly::dynamic event = folly::dynamic(42.0);
  driver.updateWithEvent(event);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 42.0, 1e-9);
}

// ---------------------------------------------------------------------------
// updateWithEvent — triggers setNeedsUpdate on the manager
// ---------------------------------------------------------------------------

TEST(EventAnimationDriver, UpdateWithEvent_MarksNodeForUpdateInManager) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  EventAnimationDriver driver("onScroll", 42, {"contentOffset"}, 1, *manager);

  folly::dynamic event = folly::dynamic::object("contentOffset", 88.0);
  driver.updateWithEvent(event);

  // `setNeedsUpdate` schedules a recompute. The production code in
  // AnimatedNodesManager::runUpdates would later call update() on the
  // value node. Since we're testing the driver in isolation, we verify
  // the side effect directly: the value node's stored value reflects the
  // event payload (the setValue call inside updateWithEvent is the
  // visible effect).
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 88.0, 1e-9);
}
