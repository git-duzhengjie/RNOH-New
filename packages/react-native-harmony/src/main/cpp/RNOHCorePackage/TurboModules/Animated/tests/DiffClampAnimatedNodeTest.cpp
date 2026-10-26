/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::DiffClampAnimatedNode` — the node
 * that clamps the cumulative *difference* between consecutive input
 * values to `[min, max]`. Unlike a regular clamp, the diff-clamp
 * accumulates deltas so the output tracks the input's relative motion
 * rather than its absolute value.
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * a parent value node with it. The diff-clamp node is constructed
 * directly so the test can call `update()` and observe the result.
 *
 * Tests exercise:
 *   - Constructor extracts `input`, `min`, `max` from config
 *   - Initial state: zero diff, zero output
 *   - Input moves within `[min, max]` → output tracks the cumulative diff
 *   - Input moves past `max` → output clamps to `max`
 *   - Input moves past `min` → output clamps to `min`
 *   - Input reverses direction → output reverses within the clamp range
 *   - Multiple sequential updates accumulate correctly
 *   - Initial input non-zero (e.g. parent starts at 50, then 51)
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/DiffClampAnimatedNode.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;
using rnoh::test::makeAnimatedNodesManager;

namespace {

folly::dynamic makeValueNodeConfig(double value, double offset = 0.0) {
  return folly::dynamic::object("type", "value")("value", value)(
      "offset", offset);
}

void registerParentValue(
    AnimatedNodesManager& manager,
    facebook::react::Tag parentTag,
    double initialValue) {
  manager.createNode(parentTag, makeValueNodeConfig(initialValue, 0.0));
}

folly::dynamic
makeDiffClampConfig(facebook::react::Tag inputTag, double min, double max) {
  // Production code reads `config["input"]` via `asDouble()` (the JS
  // side serialises the tag as a floating-point number).
  return folly::dynamic::object("input", static_cast<double>(inputTag))(
      "min", min)("max", max);
}
} // namespace

TEST(DiffClampAnimatedNode, Constructor_ExtractsInputMinMaxFromConfig) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  // The constructor must store the input tag and bounds. After a single
  // update() with a non-moving parent, the output is the diff (0) clamped
  // to [min, max].
  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);
  EXPECT_NO_THROW({ node.update(); });
  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

TEST(DiffClampAnimatedNode, Update_InitialZeroInput_OutputIsZero) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

TEST(
    DiffClampAnimatedNode,
    Update_InitialNonZeroInput_FirstDeltaEqualsInputValue) {
  // When the input has a non-zero starting value, the first update
  // treats that value as the baseline (diff = inputValue - 0 baseline).
  // The production code initialises m_lastInputValue to 0, so the first
  // diff equals the initial input value itself — clamped to [min, max].
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 5.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);
  node.update();

  // First call: diff = 5 - 0 = 5, clamped to [−10, 10] → 5
  EXPECT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);
}

TEST(
    DiffClampAnimatedNode,
    Update_InputMovesWithinRange_OutputAccumulatesDiff) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);

  // First update: diff = 0 - 0 = 0, output = 0
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);

  // Move input to 3 → diff = 3 - 0 = 3, output = 0 + 3 = 3
  manager->setValue(1, 3.0);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 3.0, 1e-9);

  // Move input to 5 → diff = 5 - 3 = 2, output = 3 + 2 = 5
  manager->setValue(1, 5.0);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);
}

TEST(DiffClampAnimatedNode, Update_InputMovesPastMax_OutputClampsToMax) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);

  // Push the cumulative diff well above max (10).
  manager->setValue(1, 50.0);
  node.update();

  // diff = 50 - 0 = 50, clamped: min(max(0 + 50, -10), 10) = 10
  EXPECT_NEAR(node.getOutputAsDouble(), 10.0, 1e-9);
}

TEST(DiffClampAnimatedNode, Update_InputMovesPastMin_OutputClampsToMin) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);

  // Push the cumulative diff well below min (-10).
  manager->setValue(1, -50.0);
  node.update();

  // diff = -50 - 0 = -50, clamped: min(max(0 + -50, -10), 10) = -10
  EXPECT_NEAR(node.getOutputAsDouble(), -10.0, 1e-9);
}

TEST(
    DiffClampAnimatedNode,
    Update_InputReversesDirection_OutputReversesWithinClampRange) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);

  // Drive the output up to +5.
  manager->setValue(1, 5.0);
  node.update();
  ASSERT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);

  // Reverse direction: drive the input back down to 2.
  // diff = 2 - 5 = -3, output = 5 + (-3) = 2 (still within [−10, 10])
  manager->setValue(1, 2.0);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 2.0, 1e-9);
}

TEST(DiffClampAnimatedNode, Update_SequentialUpdates_AccumulateCorrectly) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -100.0, 100.0), *manager);

  // Drive a sequence of small increments and verify the cumulative diff
  // at each step.
  double prevV = 0.0;
  double expected = 0.0;
  for (double v : {1.0, 2.5, 4.0, 6.0, 9.5}) {
    manager->setValue(1, v);
    node.update();
    expected += v - prevV;
    prevV = v;
    EXPECT_NEAR(node.getOutputAsDouble(), expected, 1e-9);
  }
  EXPECT_NEAR(node.getOutputAsDouble(), 9.5, 1e-9);
}

TEST(
    DiffClampAnimatedNode,
    Update_AfterClampAtMax_SubsequentDeltasReduceFromClampedValue) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  DiffClampAnimatedNode node(makeDiffClampConfig(1, -10.0, 10.0), *manager);

  // Push to clamp at +10.
  manager->setValue(1, 100.0);
  node.update();
  ASSERT_NEAR(node.getOutputAsDouble(), 10.0, 1e-9);

  // Reverse slightly: diff = 90 - 100 = -10, output = 10 + (-10) = 0
  manager->setValue(1, 90.0);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}
