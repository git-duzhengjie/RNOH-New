/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::FrameBasedAnimationDriver` — the
 * driver that interpolates a value across a fixed set of frame keys
 * (typically 60 FPS). The driver reads the elapsed time since animation
 * start, computes the frame index, and interpolates between the two
 * neighbouring frame values using a per-frame slope (`m_speeds`).
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * a value node as the animated node. The driver is constructed directly
 * so the test can call `runAnimationStep(frameTimeNanos)` and observe
 * the resulting value via the manager.
 *
 * Tests exercise:
 *   - Constructor parses `frames`, `toValue`, `iterations` from config
 *   - `iterations == 0` short-circuits: the animation is immediately
 *     finished
 *   - Single-step animation advances the value toward `toValue`
 *   - Many steps converge to `toValue` exactly
 *   - `hasFinished()` becomes true after the last frame
 *   - Frame index calculation: at the time corresponding to frame K,
 *     the value is `(toValue - fromValue) * frames[K] + fromValue`
 *   - Looping with `iterations > 1` wraps `m_startTimeNanos` and resets
 *     the value to `fromValue` between iterations
 *
 * Time-dependent handling per guide §4.6: the frame interpolation uses
 * `frameTimeNanos` as input, but tests supply deterministic frame times
 * (16ms increments) and only assert direction and convergence — never
 * precise timing values.
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Drivers/FrameBasedAnimationDriver.h"
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
    double value) {
  manager.createNode(tag, makeValueNodeConfig(value, 0.0));
}

folly::dynamic makeFrameBasedConfig(
    std::vector<double> frames,
    double toValue = 0.0,
    int64_t iterations = 1) {
  folly::dynamic framesArr = folly::dynamic::array;
  for (auto v : frames) {
    framesArr.push_back(v);
  }
  return folly::dynamic::object("frames", framesArr)("toValue", toValue)(
      "iterations", static_cast<int64_t>(iterations));
}
} // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

TEST(
    FrameBasedAnimationDriver,
    Constructor_ExtractsFramesAndToValueFromConfig) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  FrameBasedAnimationDriver driver(
      /*animationId=*/5,
      /*animatedNode=*/1,
      *manager,
      makeFrameBasedConfig({0.0, 0.5, 1.0}, /*toValue=*/100.0),
      [](bool, std::optional<double>) {});

  EXPECT_EQ(driver.getId(), 5);
  EXPECT_EQ(driver.getAnimatedValueTag(), 1);
  EXPECT_FALSE(driver.hasFinished());
}

TEST(
    FrameBasedAnimationDriver,
    Constructor_ZeroIterations_ImmediatelyFinished) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  FrameBasedAnimationDriver driver(
      1,
      1,
      *manager,
      makeFrameBasedConfig({0.0, 0.5, 1.0}, 0.0, /*iterations=*/0),
      [](bool, std::optional<double>) {});

  EXPECT_TRUE(driver.hasFinished());
}

TEST(FrameBasedAnimationDriver, RunAnimationStep_AlreadyFinished_DoesNothing) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 42.0);

  FrameBasedAnimationDriver driver(
      1,
      1,
      *manager,
      makeFrameBasedConfig({0.0, 1.0}, 0.0, /*iterations=*/0),
      [](bool, std::optional<double>) {});

  driver.runAnimationStep(16'000'000);

  // The animated value should be unchanged: the production code returns
  // early when `m_hasFinished` is already true.
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 42.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Single-step motion
// ---------------------------------------------------------------------------

TEST(
    FrameBasedAnimationDriver,
    RunAnimationStep_FirstStep_AdvancesValueTowardToValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  FrameBasedAnimationDriver driver(
      1,
      1,
      *manager,
      // Three frames: 0.0, 0.5, 1.0
      makeFrameBasedConfig({0.0, 0.5, 1.0}, /*toValue=*/100.0),
      [](bool, std::optional<double>) {});

  // The first runAnimationStep seeds m_startTimeNanos = frameTimeNanos.
  // The production code uses `m_startTimeNanos == 0` as the sentinel for
  // "first step", so the seed value must be non-zero — otherwise the
  // second step would re-seed and reset m_fromValue, leaving the value
  // pinned at 0.
  driver.runAnimationStep(1'000'000);
  // 17ms later: timeFromStartMillis = 17, frameIndex = 1 (17/16.67 ≈ 1.02).
  driver.runAnimationStep(18'000'000);

  double value = manager->getValueNodeByTag(1).getOutputAsDouble();
  // After one full 16ms frame the value should have advanced away from
  // 0 toward toValue (100). It must be strictly between 0 and 100.
  EXPECT_GT(value, 0.0);
  EXPECT_LT(value, 100.0);
}

// ---------------------------------------------------------------------------
// Convergence
// ---------------------------------------------------------------------------

TEST(
    FrameBasedAnimationDriver,
    RunAnimationStep_PastLastFrame_FinishesAndSnapsToToValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  // Three frames at 16ms intervals = 48ms total duration.
  FrameBasedAnimationDriver driver(
      1,
      1,
      *manager,
      makeFrameBasedConfig({0.0, 0.5, 1.0}, /*toValue=*/100.0),
      [](bool, std::optional<double>) {});

  // Seed with a non-zero start time (see FirstStep test for rationale).
  driver.runAnimationStep(1'000'000);
  // 80ms later = 80ms total elapsed time, well past the last frame.
  driver.runAnimationStep(81'000'000);

  EXPECT_TRUE(driver.hasFinished());
  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 100.0, 1e-9);
}

TEST(
    FrameBasedAnimationDriver,
    RunAnimationStep_ManyStepsBetweenFrames_InterpolatesLinearly) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  // Single frame at 0, then 1 → linear ramp from 0 to toValue=100 across
  // one 16.6667ms frame (FRAME_TIME_MILLIS = 1000.0/60.0).
  FrameBasedAnimationDriver driver(
      1,
      1,
      *manager,
      makeFrameBasedConfig({0.0, 1.0}, /*toValue=*/100.0),
      [](bool, std::optional<double>) {});

  driver.runAnimationStep(1'000'000);
  // 8.333ms later (half of 16.6667ms): the interpolated distance is 0.5,
  // so the value should be 0 + (100 - 0) * 0.5 = 50.
  driver.runAnimationStep(9'333'000);

  EXPECT_NEAR(manager->getValueNodeByTag(1).getOutputAsDouble(), 50.0, 0.5);
}

// ---------------------------------------------------------------------------
// Looping
// ---------------------------------------------------------------------------

TEST(
    FrameBasedAnimationDriver,
    RunAnimationStep_TwoIterationsAfterLastFrame_ResetsAndLoops) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  FrameBasedAnimationDriver driver(
      1,
      1,
      *manager,
      makeFrameBasedConfig({0.0, 1.0}, /*toValue=*/100.0, /*iterations=*/2),
      [](bool, std::optional<double>) {});

  // Iteration 1: seed, then advance past the last frame.
  driver.runAnimationStep(1'000'000);
  driver.runAnimationStep(50'000'000);

  // Iteration 2 should have started (m_startTimeNanos reset to 0). The
  // animation is not yet finished (only one iteration done).
  EXPECT_FALSE(driver.hasFinished());

  // Continue iterating: re-seed m_startTimeNanos, then advance past the
  // last frame again to finish iteration 2.
  driver.runAnimationStep(51'000'000);
  EXPECT_FALSE(driver.hasFinished());
  driver.runAnimationStep(100'000'000);
  EXPECT_TRUE(driver.hasFinished());
}

// ---------------------------------------------------------------------------
// Destructor callback
// ---------------------------------------------------------------------------

TEST(
    FrameBasedAnimationDriver,
    Destructor_WhenNotFinished_InvokesEndCallbackWithFinishedFalse) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 7.0);

  bool callbackInvoked = false;
  bool reportedFinished = true;
  std::optional<double> reportedValue;
  {
    FrameBasedAnimationDriver driver(
        1,
        1,
        *manager,
        makeFrameBasedConfig({0.0, 1.0}, 100.0),
        [&](bool finished, std::optional<double> value) {
          callbackInvoked = true;
          reportedFinished = finished;
          reportedValue = value;
        });
    EXPECT_FALSE(driver.hasFinished());
  }

  EXPECT_TRUE(callbackInvoked);
  EXPECT_FALSE(reportedFinished);
  ASSERT_TRUE(reportedValue.has_value());
  EXPECT_NEAR(reportedValue.value(), 7.0, 1e-9);
}
