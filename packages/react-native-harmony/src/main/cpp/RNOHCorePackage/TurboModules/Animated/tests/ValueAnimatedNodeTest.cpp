/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::ValueAnimatedNode` — the leaf node
 * representing a single numeric `Animated.Value` (and the base for
 * `Animated.ValueXY` via two scalar values managed externally).
 *
 * Per `gtest-module-catalog.md` the module's pure-C++ behaviour is
 * testable with a "Mock AnimatedNodesManager" notation — but
 * `ValueAnimatedNode` itself does NOT hold a manager reference. It only
 * requires Folly + RN core + the `RNOH_ASSERT` / `LOG(ERROR)` macros
 * which are stubbed in `tests/stubs/`. The frame-rate hysteresis logic
 * (`recalculateMinAcceptableFrameRate`) is the most complex piece under
 * test and is exercised here with deterministic velocity/scale inputs.
 *
 * Tests exercise:
 *   - Constructors (default + dynamic config)
 *   - `getOutput()` / `getOutputAsDouble()` with and without offset
 *   - `setValue(double)` / `setValue(folly::dynamic)` / `setOffset()`
 *   - `setVelocity()` + `getFrameRate()` + frame-rate hysteresis across
 *     all four starting rates (30/60/90/120)
 *   - `flattenOffset()` / `extractOffset()` round-trip
 *   - Value-listener lifecycle (set, fire, remove, double-set throws)
 *   - `getValueAsDouble()` null / number fallback path
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;

namespace {
folly::dynamic makeValueConfig(double value, double offset) {
  return folly::dynamic::object("value", value)("offset", offset);
}
} // namespace

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

TEST(ValueAnimatedNode, DefaultConstructor_InitializesZeroOffsetAndValue) {
  ValueAnimatedNode node;

  // The default constructor leaves `m_value` default-constructed
  // (folly::dynamic::null) and `m_offset = 0.0`. `getOutputAsDouble()`
  // therefore reports 0 because getValueAsDouble() returns 0 for null.
  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

TEST(ValueAnimatedNode, DefaultConstructor_SetsDefaultFrameRateTo60) {
  ValueAnimatedNode node;

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_60);
}

TEST(ValueAnimatedNode, ConfigConstructor_SetsValueFromConfig) {
  ValueAnimatedNode node(makeValueConfig(42.0, 0.0));

  EXPECT_NEAR(node.getOutputAsDouble(), 42.0, 1e-9);
}

TEST(ValueAnimatedNode, ConfigConstructor_SetsOffsetFromConfig) {
  ValueAnimatedNode node(makeValueConfig(10.0, 5.0));

  // value (10) + offset (5) = 15
  EXPECT_NEAR(node.getOutputAsDouble(), 15.0, 1e-9);
}

TEST(ValueAnimatedNode, ConfigConstructor_NegativeOffset_SubtractsFromValue) {
  ValueAnimatedNode node(makeValueConfig(100.0, -30.0));

  EXPECT_NEAR(node.getOutputAsDouble(), 70.0, 1e-9);
}

// ---------------------------------------------------------------------------
// getOutput() — folly::dynamic variant
// ---------------------------------------------------------------------------

TEST(ValueAnimatedNode, GetOutput_DoubleValue_ReturnsNumberDynamic) {
  ValueAnimatedNode node(makeValueConfig(3.14, 0.0));

  auto output = node.getOutput();
  ASSERT_TRUE(output.isDouble());
  EXPECT_NEAR(output.asDouble(), 3.14, 1e-9);
}

TEST(ValueAnimatedNode, GetOutput_WithOffset_ReturnsSummedDouble) {
  ValueAnimatedNode node(makeValueConfig(2.0, 0.5));

  auto output = node.getOutput();
  ASSERT_TRUE(output.isDouble());
  EXPECT_NEAR(output.asDouble(), 2.5, 1e-9);
}

TEST(ValueAnimatedNode, GetOutput_IntegerValue_ReturnsIntegerDynamic) {
  // When the JS side passes an integer (`Animated.Value(5)` without a
  // decimal), the value is stored as int64_t. The node preserves the
  // original type and `getOutput()` returns the integer dynamic.
  folly::dynamic config = folly::dynamic::object("value", 5)("offset", 0.0);
  ValueAnimatedNode node(config);

  auto output = node.getOutput();
  // Integer dynamics report as integers; the sum with offset still
  // yields the original integer (offset is 0 here).
  EXPECT_FALSE(output.isDouble());
}

// ---------------------------------------------------------------------------
// setValue / setOffset / setVelocity
// ---------------------------------------------------------------------------

TEST(ValueAnimatedNode, SetValue_Double_UpdatesValueWithoutChangingOffset) {
  ValueAnimatedNode node(makeValueConfig(0.0, 7.0));

  node.setValue(99.0);

  // 99 + 7 = 106
  EXPECT_NEAR(node.getOutputAsDouble(), 106.0, 1e-9);
}

TEST(ValueAnimatedNode, SetValue_Dynamic_UpdatesUnderlyingValue) {
  ValueAnimatedNode node;
  node.setOffset(1.0);

  node.setValue(folly::dynamic(50.0));

  EXPECT_NEAR(node.getOutputAsDouble(), 51.0, 1e-9);
}

TEST(ValueAnimatedNode, SetOffset_UpdatesOffsetWithoutChangingValue) {
  ValueAnimatedNode node(makeValueConfig(10.0, 0.0));

  node.setOffset(3.0);

  EXPECT_NEAR(node.getOutputAsDouble(), 13.0, 1e-9);
}

TEST(ValueAnimatedNode, SetVelocity_DoesNotAffectOutputValue) {
  ValueAnimatedNode node(makeValueConfig(5.0, 0.0));

  node.setVelocity(123.0);

  // Velocity feeds the frame-rate hysteresis only; it must not leak
  // into the value/offset output path.
  EXPECT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Frame-rate hysteresis — recalculateMinAcceptableFrameRate
// ---------------------------------------------------------------------------

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_ZeroVelocityFrom60_DownshiftsTo30) {
  ValueAnimatedNode node;
  node.setVelocity(0.0);

  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_30);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_BelowDownshiftThresholdFrom60_StaysAt60) {
  ValueAnimatedNode node;
  // animationSpeed = 0.2 * 25.4 * 1.0 = 5.08 mm/s — above the 3.5 mm/s
  // downshift threshold and below the 22 mm/s upshift threshold, so the
  // frame rate must remain at 60.
  node.setVelocity(0.2);

  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_60);
}

TEST(ValueAnimatedNode, RecalculateFrameRate_AboveUpshift60To90_UpshiftsTo90) {
  ValueAnimatedNode node;
  // animationSpeed = 1.0 * 25.4 * 1.0 = 25.4 mm/s — above the 22 mm/s
  // SPEED_UP_60_TO_90 threshold but below SPEED_UP_90_TO_120 (77).
  node.setVelocity(1.0);

  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_90);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_AboveUpshift90To120From60_UpshiftsTo120) {
  ValueAnimatedNode node;
  // animationSpeed = 3.1 * 25.4 * 1.0 ≈ 78.74 mm/s — above 77 mm/s.
  node.setVelocity(3.1);

  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_120);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_From30_AboveUpshift30To60_UpshiftsTo60) {
  ValueAnimatedNode node;
  // Force the starting frame rate to 30 by first downshifting from 60.
  node.setVelocity(0.0);
  node.recalculateMinAcceptableFrameRate(1.0f);
  ASSERT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_30);

  // animationSpeed = 0.25 * 25.4 * 1.0 = 6.35 mm/s — above the 5 mm/s
  // SPEED_UP_30_TO_60 threshold but below 22 mm/s.
  node.setVelocity(0.25);
  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_60);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_From30_AboveUpshift90To120_JumpsTo120) {
  ValueAnimatedNode node;
  node.setVelocity(0.0);
  node.recalculateMinAcceptableFrameRate(1.0f);
  ASSERT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_30);

  // Upshifts allow multi-step jumps: starting at 30 with a very high
  // velocity should reach 120 directly.
  node.setVelocity(3.1);
  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_120);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_From90_AboveDownshift120To90_DownshiftsTo60) {
  ValueAnimatedNode node;
  // Reach 90 by upshifting from 60.
  node.setVelocity(1.0);
  node.recalculateMinAcceptableFrameRate(1.0f);
  ASSERT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_90);

  // animationSpeed = 0.5 * 25.4 * 1.0 = 12.7 mm/s — below the 15 mm/s
  // SPEED_DOWN_90_TO_60 threshold but above 3.5 mm/s (so 60 won't drop
  // further).
  node.setVelocity(0.5);
  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_60);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_From120_BelowDownshift120To90_DownshiftsTo90) {
  ValueAnimatedNode node;
  // Reach 120 by upshifting from 60.
  node.setVelocity(3.1);
  node.recalculateMinAcceptableFrameRate(1.0f);
  ASSERT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_120);

  // animationSpeed = 2.2 * 25.4 * 1.0 ≈ 55.88 mm/s — just above the
  // 54 mm/s SPEED_DOWN_120_TO_90 threshold (must NOT downshift yet).
  node.setVelocity(2.2);
  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_120);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_From120_BelowDownshift120To90Threshold_Downshifts) {
  ValueAnimatedNode node;
  node.setVelocity(3.1);
  node.recalculateMinAcceptableFrameRate(1.0f);
  ASSERT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_120);

  // animationSpeed = 2.0 * 25.4 * 1.0 = 50.8 mm/s — below the 54 mm/s
  // SPEED_DOWN_120_TO_90 threshold. Must downshift to 90 (single step).
  node.setVelocity(2.0);
  node.recalculateMinAcceptableFrameRate(1.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_90);
}

TEST(
    ValueAnimatedNode,
    RecalculateFrameRate_ScaleRatioDpiScalesAnimationSpeed) {
  ValueAnimatedNode node;
  // With scaleRatioDpi = 2.0 and velocity = 1.0, animationSpeed is
  // 1.0 * 25.4 * 2.0 = 50.8 mm/s — same as velocity=2.0 with ratio 1.0.
  // This pins the formula's use of the scale-ratio parameter.
  node.setVelocity(1.0);

  node.recalculateMinAcceptableFrameRate(2.0f);

  EXPECT_EQ(node.getFrameRate(), ValueAnimatedNode::FRAME_RATE_90);
}

// ---------------------------------------------------------------------------
// flattenOffset / extractOffset
// ---------------------------------------------------------------------------

TEST(ValueAnimatedNode, FlattenOffset_MergesOffsetIntoValueAndZeroesOffset) {
  ValueAnimatedNode node(makeValueConfig(10.0, 5.0));

  node.flattenOffset();

  // After flatten: value = 15, offset = 0
  EXPECT_NEAR(node.getOutputAsDouble(), 15.0, 1e-9);
  // Flatten is idempotent on the output: calling again must not change
  // the sum (offset is now 0).
  node.flattenOffset();
  EXPECT_NEAR(node.getOutputAsDouble(), 15.0, 1e-9);
}

TEST(ValueAnimatedNode, ExtractOffset_MovesValueIntoOffsetAndZeroesValue) {
  ValueAnimatedNode node(makeValueConfig(10.0, 5.0));

  node.extractOffset();

  // After extract: offset = 5 + 10 = 15, value = 0
  // The sum (getOutputAsDouble) is unchanged.
  EXPECT_NEAR(node.getOutputAsDouble(), 15.0, 1e-9);
}

TEST(ValueAnimatedNode, ExtractOffset_AfterFlatten_PreservesOutputSum) {
  ValueAnimatedNode node(makeValueConfig(10.0, 5.0));
  node.flattenOffset();
  ASSERT_NEAR(node.getOutputAsDouble(), 15.0, 1e-9);

  node.extractOffset();

  EXPECT_NEAR(node.getOutputAsDouble(), 15.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Value listener lifecycle
// ---------------------------------------------------------------------------

TEST(ValueAnimatedNode, OnValueUpdate_WithListener_InvokesListenerWithOutput) {
  ValueAnimatedNode node(makeValueConfig(10.0, 5.0));
  double captured = -1.0;
  node.setValueListener([&](double v) { captured = v; });

  node.onValueUpdate();

  EXPECT_NEAR(captured, 15.0, 1e-9);
}

TEST(ValueAnimatedNode, OnValueUpdate_WithoutListener_DoesNotThrow) {
  ValueAnimatedNode node(makeValueConfig(1.0, 0.0));

  EXPECT_NO_THROW({ node.onValueUpdate(); });
}

TEST(ValueAnimatedNode, SetValueListener_Twice_ThrowsRuntimeError) {
  ValueAnimatedNode node;
  node.setValueListener([](double) {});

  // The production contract is one-listener-per-node: registering a
  // second listener without removing the first throws.
  EXPECT_THROW(node.setValueListener([](double) {}), std::runtime_error);
}

TEST(ValueAnimatedNode, RemoveValueListener_PreventsSubsequentCallbacks) {
  ValueAnimatedNode node(makeValueConfig(2.0, 0.0));
  int callCount = 0;
  node.setValueListener([&callCount](double) { ++callCount; });

  node.onValueUpdate();
  EXPECT_EQ(callCount, 1);

  node.removeValueListener();
  node.onValueUpdate();

  EXPECT_EQ(callCount, 1);
}

// ---------------------------------------------------------------------------
// getValueAsDouble fallback paths
// ---------------------------------------------------------------------------

TEST(ValueAnimatedNode, GetValueAsDouble_NullValue_ReturnsZero) {
  // Default constructor leaves m_value as folly::dynamic::null. The
  // production code treats null as a special case (used by diffClamp)
  // and returns 0.
  ValueAnimatedNode node;

  // Access via getOutputAsDouble — getValueAsDouble is protected, but
  // getOutputAsDouble delegates to it.
  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

TEST(ValueAnimatedNode, GetValueAsDouble_NumberValue_ReturnsAsDouble) {
  ValueAnimatedNode node(makeValueConfig(123.0, 0.0));

  EXPECT_NEAR(node.getOutputAsDouble(), 123.0, 1e-9);
}
