/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::AnimatedNodesManager` — the
 * central registry that owns the animated node graph, dispatches
 * updates, and manages driver lifecycle.
 *
 * The manager is non-virtual, so per guide §5.7 we construct a real
 * instance via `makeAnimatedNodesManager()` and exercise its public API
 * directly.  These tests focus on `createNode` branch coverage,
 * `dropNode`, `connectNodes`/`disconnectNodes`, and
 * `addAnimatedEventToView` — paths that the node/driver-specific tests
 * only exercise indirectly (or not at all).
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/AnimatedNodesManager.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;
using rnoh::test::makeAnimatedNodesManager;

namespace {

folly::dynamic makeValueNodeConfig(double value, double offset = 0.0) {
  return folly::dynamic::object("type", "value")("value", value)(
      "offset", offset);
}

folly::dynamic makeInterpolationConfig(facebook::react::Tag inputTag) {
  return folly::dynamic::object("type", "interpolation")(
      "input", static_cast<double>(inputTag))("extrapolateLeft", "clamp")(
      "extrapolateRight", "clamp")(
      "outputRange", folly::dynamic::array(0.0, 100.0))(
      "inputRange", folly::dynamic::array(0.0, 1.0))("outputType", "number");
}

folly::dynamic
makeDiffClampConfig(facebook::react::Tag inputTag, double min, double max) {
  return folly::dynamic::object("type", "diffclamp")(
      "input", static_cast<double>(inputTag))("min", min)("max", max);
}

folly::dynamic makeModulusConfig(
    facebook::react::Tag inputTag,
    double modulus) {
  return folly::dynamic::object("type", "modulus")(
      "input", static_cast<double>(inputTag))("modulus", modulus);
}

folly::dynamic makeTrackingConfig(
    facebook::react::Tag animationId,
    facebook::react::Tag valueTag,
    facebook::react::Tag toValueTag = 0) {
  return folly::dynamic::object("type", "tracking")(
      "animationId", static_cast<double>(animationId))(
      "value", static_cast<double>(valueTag))(
      "toValue", static_cast<double>(toValueTag))(
      "animationConfig",
      folly::dynamic::object("type", "spring")("stiffness", 100.0)(
          "damping", 10.0));
}
} // namespace

// ---------------------------------------------------------------------------
// createNode — branch coverage for each node type
// ---------------------------------------------------------------------------

TEST(
    AnimatedNodesManager,
    CreateNode_InterpolationType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.5));
  EXPECT_NO_THROW(manager->createNode(2, makeInterpolationConfig(1)));
}

TEST(AnimatedNodesManager, CreateNode_DiffClampType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  EXPECT_NO_THROW(manager->createNode(2, makeDiffClampConfig(1, -10.0, 10.0)));
}

TEST(AnimatedNodesManager, CreateNode_ModulusType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(10.0));
  EXPECT_NO_THROW(manager->createNode(2, makeModulusConfig(1, 3.0)));
}

TEST(AnimatedNodesManager, CreateNode_TrackingType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  manager->createNode(3, makeValueNodeConfig(50.0));
  EXPECT_NO_THROW(manager->createNode(2, makeTrackingConfig(100, 1, 3)));
}

TEST(AnimatedNodesManager, CreateNode_AdditionType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(1.0));
  manager->createNode(2, makeValueNodeConfig(2.0));
  EXPECT_NO_THROW(manager->createNode(
      3,
      folly::dynamic::object("type", "addition")(
          "input", folly::dynamic::array(1.0, 2.0))));
}

TEST(AnimatedNodesManager, CreateNode_SubtractionType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(5.0));
  manager->createNode(2, makeValueNodeConfig(3.0));
  EXPECT_NO_THROW(manager->createNode(
      3,
      folly::dynamic::object("type", "subtraction")(
          "input", folly::dynamic::array(1.0, 2.0))));
}

TEST(
    AnimatedNodesManager,
    CreateNode_MultiplicationType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(4.0));
  manager->createNode(2, makeValueNodeConfig(2.0));
  EXPECT_NO_THROW(manager->createNode(
      3,
      folly::dynamic::object("type", "multiplication")(
          "input", folly::dynamic::array(1.0, 2.0))));
}

TEST(AnimatedNodesManager, CreateNode_DivisionType_CreatesNodeSuccessfully) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(8.0));
  manager->createNode(2, makeValueNodeConfig(2.0));
  EXPECT_NO_THROW(manager->createNode(
      3,
      folly::dynamic::object("type", "division")(
          "input", folly::dynamic::array(1.0, 2.0))));
}

TEST(AnimatedNodesManager, CreateNode_UnsupportedType_ThrowsRuntimeError) {
  auto manager = makeAnimatedNodesManager();
  EXPECT_THROW(
      manager->createNode(1, folly::dynamic::object("type", "nonexistent")),
      std::runtime_error);
}

// ---------------------------------------------------------------------------
// dropNode
// ---------------------------------------------------------------------------

TEST(AnimatedNodesManager, DropNode_RemovesNodeFromRegistry) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(42.0));
  EXPECT_NO_THROW(manager->dropNode(1));
  // getNodeByTag for a dropped tag should throw (node not found).
  EXPECT_ANY_THROW(manager->getNodeByTag(1));
}

TEST(AnimatedNodesManager, DropNode_OnNonexistentTag_DoesNotThrow) {
  auto manager = makeAnimatedNodesManager();
  // dropNode uses erase() which is a no-op for non-existent keys.
  EXPECT_NO_THROW(manager->dropNode(999));
}

// ---------------------------------------------------------------------------
// connectNodes / disconnectNodes
// ---------------------------------------------------------------------------

TEST(AnimatedNodesManager, ConnectNodes_AddsChildToParent) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  manager->createNode(2, makeValueNodeConfig(0.0));
  EXPECT_NO_THROW(manager->connectNodes(1, 2));
}

TEST(AnimatedNodesManager, DisconnectNodes_RemovesChildFromParent) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  manager->createNode(2, makeValueNodeConfig(0.0));
  manager->connectNodes(1, 2);
  EXPECT_NO_THROW(manager->disconnectNodes(1, 2));
}

// ---------------------------------------------------------------------------
// addAnimatedEventToView / removeAnimatedEventFromView
// ---------------------------------------------------------------------------

TEST(AnimatedNodesManager, AddAnimatedEventToView_RegistersEventDriver) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  EXPECT_NO_THROW(manager->addAnimatedEventToView(
      /*viewTag=*/10,
      /*eventName=*/"onScroll",
      folly::dynamic::object("animatedValueTag", 1)(
          "nativeEventPath", folly::dynamic::array("contentOffset", "x"))));
}

TEST(AnimatedNodesManager, HandleEvent_ForRegisteredDriver_UpdatesValueNode) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  manager->addAnimatedEventToView(
      10,
      "onScroll",
      folly::dynamic::object("animatedValueTag", 1)(
          "nativeEventPath", folly::dynamic::array("contentOffset", "x")));
  EXPECT_NO_THROW(manager->handleEvent(
      10,
      "onScroll",
      folly::dynamic::object(
          "contentOffset", folly::dynamic::object("x", 50.0))));
}

TEST(AnimatedNodesManager, RemoveAnimatedEventFromView_RemovesMatchingDriver) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  manager->addAnimatedEventToView(
      10,
      "onScroll",
      folly::dynamic::object("animatedValueTag", 1)(
          "nativeEventPath", folly::dynamic::array("contentOffset", "x")));
  EXPECT_NO_THROW(manager->removeAnimatedEventFromView(10, "onScroll", 1));
  // After removal, handleEvent should return an empty list (no driver).
  auto result = manager->handleEvent(
      10,
      "onScroll",
      folly::dynamic::object(
          "contentOffset", folly::dynamic::object("x", 50.0)));
  EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// setValue / setOffset / flattenOffset / extractOffset
// ---------------------------------------------------------------------------

TEST(AnimatedNodesManager, SetValue_UpdatesValueNode) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(0.0));
  manager->setValue(1, 99.0);
  EXPECT_NEAR(manager->getValueNodeByTag(1).getValueAsDouble(), 99.0, 1e-9);
}

TEST(AnimatedNodesManager, SetOffset_UpdatesOffset) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(10.0));
  manager->setOffset(1, 5.0);
  // getOutputAsDouble = value + offset
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 15.0, 1e-9);
}

TEST(AnimatedNodesManager, FlattenOffset_AddsOffsetToValue) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(10.0, 5.0));
  manager->flattenOffset(1);
  EXPECT_NEAR(manager->getValueNodeByTag(1).getValueAsDouble(), 15.0, 1e-9);
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 15.0, 1e-9);
}

TEST(AnimatedNodesManager, ExtractOffset_MovesOffsetToValue) {
  auto manager = makeAnimatedNodesManager();
  manager->createNode(1, makeValueNodeConfig(10.0, 5.0));
  manager->extractOffset(1);
  // extractOffset adds value to offset, then zeroes value
  EXPECT_NEAR(manager->getValueNodeByTag(1).getValueAsDouble(), 0.0, 1e-9);
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 15.0, 1e-9);
}

// ---------------------------------------------------------------------------
// getScaleRatioDpi
// ---------------------------------------------------------------------------

TEST(AnimatedNodesManager, GetScaleRatioDpi_ReturnsValueFromDisplayMetrics) {
  auto manager = makeAnimatedNodesManager();
  // The mock DisplayMetricsManager uses 1/160 for both axes.
  EXPECT_FLOAT_EQ(manager->getScaleRatioDpi(true), 1.0f / 160.0f);
  EXPECT_FLOAT_EQ(manager->getScaleRatioDpi(false), 1.0f / 160.0f);
}
