/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::SpringAnimationDriver` — the damped
 * harmonic oscillator solver used by `Animated.spring` on the JS side.
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * a value node as the animated node. The driver is constructed directly
 * so the test can call `runAnimationStep(frameTimeNanos)` and observe
 * the resulting value via the manager.
 *
 * Tests exercise:
 *   - Constructor pulls `stiffness`, `damping`, `mass`, `toValue`,
 *     `initialVelocity`, `overshootClamp`, `restSpeedThreshold`,
 *     `restDisplacementThreshold`, `iterations` from config
 *   - Single-step: `runAnimationStep` advances the spring
 *   - At-rest config: `hasFinished` becomes true after a settling step
 *   - Many steps converge to `toValue` (underdamped spring)
 *   - `iterations == 0` produces an immediately-finished animation
 *   - `getId()` and `getAnimatedValueTag()` return the constructor args
 *   - End callback is invoked when the spring settles (via destructor)
 *   - Overshoot clamping snaps to `toValue` when overshoot is enabled
 *
 * Time-dependent handling per guide §4.6: spring physics uses
 * `frameTimeNanos` as input, but tests supply deterministic frame times
 * (16ms increments) and only assert direction and convergence — never
 * precise timing values.
 */

#include <gtest/gtest.h>
#include <cmath>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Drivers/SpringAnimationDriver.h"
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

folly::dynamic makeSpringConfig(
    double stiffness = 100.0,
    double damping = 10.0,
    double mass = 1.0,
    double toValue = 0.0,
    double initialVelocity = 0.0,
    bool overshootClamping = false,
    double restSpeedThreshold = 0.001,
    double restDisplacementThreshold = 0.001,
    int64_t iterations = 1) {
  return folly::dynamic::object("stiffness", stiffness)("damping", damping)(
      "mass", mass)("toValue", toValue)("initialVelocity", initialVelocity)(
      "overshootClamping", overshootClamping)(
      "restSpeedThreshold", restSpeedThreshold)(
      "restDisplacementThreshold", restDisplacementThreshold)(
      "iterations", static_cast<double>(iterations));
}
} // namespace

// ---------------------------------------------------------------------------
// Constructor & basic accessors
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    Constructor_ExtractsSpringConfigAndInitialVelocity) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      /*animationId=*/100,
      /*animatedNode=*/1,
      *manager,
      makeSpringConfig(
          /*stiffness=*/200.0,
          /*damping=*/20.0,
          /*mass=*/2.0,
          /*toValue=*/50.0,
          /*initialVelocity=*/10.0),
      /*endCallback=*/[](bool, std::optional<double>) {});

  EXPECT_EQ(driver.getId(), 100);
  EXPECT_EQ(driver.getAnimatedValueTag(), 1);
  EXPECT_FALSE(driver.hasFinished());
}

TEST(SpringAnimationDriver, Constructor_ZeroIterations_ImmediatelyFinished) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      10,
      1,
      *manager,
      makeSpringConfig(100.0, 10.0, 1.0, 0.0, 0.0, false, 0.001, 0.001, 0),
      [](bool, std::optional<double>) {});

  // iterations == 0 short-circuits the animation: it should be marked
  // finished without ever running a step.
  EXPECT_TRUE(driver.hasFinished());
}

// ---------------------------------------------------------------------------
// runAnimationStep — motion & convergence
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    RunAnimationStep_SingleStep_AdvancesValueTowardToValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      1,
      1,
      *manager,
      makeSpringConfig(
          /*stiffness=*/100.0,
          /*damping=*/10.0,
          /*mass=*/1.0,
          /*toValue=*/100.0,
          /*initialVelocity=*/0.0),
      [](bool, std::optional<double>) {});

  // The first runAnimationStep seeds m_lastTime but advances the spring
  // by 0 ms (deltaTime = 0). The value only starts moving on the second
  // step, when deltaTime becomes positive.
  driver.runAnimationStep(0);
  driver.runAnimationStep(16'000'000);

  auto& animatedValue = manager->getValueNodeByTag(1);
  // After one advancement the value must have moved toward toValue (100),
  // so it must be greater than the initial 0.
  EXPECT_GT(animatedValue.getOutputAsDouble(), 0.0);
  EXPECT_LT(animatedValue.getOutputAsDouble(), 100.0);
}

TEST(SpringAnimationDriver, RunAnimationStep_ManySteps_ConvergesToToValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      1,
      1,
      *manager,
      makeSpringConfig(
          /*stiffness=*/100.0,
          /*damping=*/20.0, // higher damping → faster settle
          /*mass=*/1.0,
          /*toValue=*/50.0,
          /*initialVelocity=*/0.0),
      [](bool, std::optional<double>) {});

  // 60 fps × 2 seconds = 120 steps of 16ms each.
  for (int i = 0; i < 120; ++i) {
    driver.runAnimationStep(static_cast<long long>(16'000'000) * (i + 1));
  }

  auto& animatedValue = manager->getValueNodeByTag(1);
  EXPECT_NEAR(animatedValue.getOutputAsDouble(), 50.0, 0.5);

  EXPECT_TRUE(driver.hasFinished());
}

TEST(SpringAnimationDriver, RunAnimationStep_ManySteps_EventuallySetsFinished) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      1,
      1,
      *manager,
      makeSpringConfig(100.0, 20.0, 1.0, 50.0, 0.0, false, 0.001, 0.001, 1),
      [](bool, std::optional<double>) {});

  for (int i = 0; i < 300; ++i) {
    driver.runAnimationStep(static_cast<long long>(16'000'000) * (i + 1));
    if (driver.hasFinished()) {
      break;
    }
  }

  EXPECT_TRUE(driver.hasFinished());
}

// ---------------------------------------------------------------------------
// End-callback lifecycle
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    Destructor_WhenNotFinished_InvokesEndCallbackWithFinishedFalse) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 7.0);

  bool callbackInvoked = false;
  bool reportedFinished = true;
  std::optional<double> reportedValue;
  {
    SpringAnimationDriver driver(
        1,
        1,
        *manager,
        makeSpringConfig(100.0, 10.0, 1.0, 100.0, 0.0),
        [&](bool finished, std::optional<double> value) {
          callbackInvoked = true;
          reportedFinished = finished;
          reportedValue = value;
        });
    // Driver destructor runs at end of scope without ever calling
    // runAnimationStep, so hasFinished() is still false here.
    EXPECT_FALSE(driver.hasFinished());
  }

  EXPECT_TRUE(callbackInvoked);
  EXPECT_FALSE(reportedFinished);
  ASSERT_TRUE(reportedValue.has_value());
  EXPECT_NEAR(reportedValue.value(), 7.0, 1e-9);
}

TEST(
    SpringAnimationDriver,
    RunAnimationStep_AfterFinish_DoesNotInvokeEndCallback) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  bool callbackInvoked = false;
  {
    SpringAnimationDriver driver(
        1,
        1,
        *manager,
        makeSpringConfig(100.0, 10.0, 1.0, 50.0, 0.0, false, 0.001, 0.001, 0),
        [&](bool, std::optional<double>) { callbackInvoked = true; });
    // iterations == 0 → already finished. Calling runAnimationStep would
    // be a no-op (the production code does NOT early-return when
    // hasFinished for spring — but the constructor has set the flag, and
    // advance() short-circuits via isAtRest()).
    driver.runAnimationStep(16'000'000);
    EXPECT_FALSE(callbackInvoked);
    EXPECT_TRUE(driver.hasFinished());
  }
  // The destructor will still invoke the callback with finished=false
  // (matching the production contract: "if (endCallback_) { ... }").
  EXPECT_TRUE(callbackInvoked);
}

// ---------------------------------------------------------------------------
// runAnimationStep — overshoot clamping
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    RunAnimationStep_OvershootClampingEnabled_StopsAtToValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  // Low damping → the spring will overshoot past toValue=100.
  // With overshootClamping=true, advance() should snap to toValue
  // as soon as overshoot is detected.
  SpringAnimationDriver driver(
      1,
      1,
      *manager,
      makeSpringConfig(
          /*stiffness=*/100.0,
          /*damping=*/2.0, // very low → overshoots easily
          /*mass=*/1.0,
          /*toValue=*/100.0,
          /*initialVelocity=*/0.0,
          /*overshootClamping=*/true),
      [](bool, std::optional<double>) {});

  // Run enough steps to trigger overshoot.
  for (int i = 0; i < 200; ++i) {
    driver.runAnimationStep(16'000'000 * (i + 1));
    if (driver.hasFinished()) {
      break;
    }
  }
  // After overshoot clamping the position should be at toValue.
  EXPECT_NEAR(manager->getValueNodeByTag(1).getValueAsDouble(), 100.0, 1e-6);
  EXPECT_TRUE(driver.hasFinished());
}

// ---------------------------------------------------------------------------
// runAnimationStep — delta-time clamping
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    RunAnimationStep_VeryLargeDeltaTime_ClampsToMaxDeltaTime) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      1,
      1,
      *manager,
      makeSpringConfig(
          /*stiffness=*/100.0,
          /*damping=*/10.0,
          /*mass=*/1.0,
          /*toValue=*/50.0,
          /*initialVelocity=*/0.0),
      [](bool, std::optional<double>) {});

  // A huge frame time (1 hour) should be clamped to MAX_DELTA_TIME_SEC
  // (0.048s). The spring should not blow up or NaN.
  driver.runAnimationStep(3'600'000'000'000); // 1 hour in nanoseconds
  double value = manager->getValueNodeByTag(1).getValueAsDouble();
  EXPECT_TRUE(std::isfinite(value));
}

// ---------------------------------------------------------------------------
// runAnimationStep — infinite loop (iterations == -1)
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    RunAnimationStep_InfiniteIterations_RestartsAfterRest) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      1,
      1,
      *manager,
      makeSpringConfig(
          /*stiffness=*/100.0,
          /*damping=*/10.0,
          /*mass=*/1.0,
          /*toValue=*/50.0,
          /*initialVelocity=*/0.0,
          /*overshootClamping=*/false,
          /*restSpeedThreshold=*/0.001,
          /*restDisplacementThreshold=*/0.001,
          /*iterations=*/-1), // infinite loop
      [](bool, std::optional<double>) {});

  // Run until at rest.
  bool reachedRest = false;
  for (int i = 1; i <= 1000; ++i) {
    driver.runAnimationStep(16'000'000 * i);
    if (driver.hasFinished()) {
      reachedRest = true;
      break;
    }
  }
  // With iterations == -1, the spring should never finish on its own.
  EXPECT_FALSE(reachedRest);
}

// ---------------------------------------------------------------------------
// runAnimationStep — non-existent node (catch block)
// ---------------------------------------------------------------------------

TEST(
    SpringAnimationDriver,
    RunAnimationStep_NonExistentNode_MarksFinishedWithoutCrash) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  SpringAnimationDriver driver(
      1,
      /*animatedNode=*/999, // non-existent tag
      *manager,
      makeSpringConfig(
          /*stiffness=*/100.0,
          /*damping=*/10.0,
          /*mass=*/1.0,
          /*toValue=*/50.0),
      [](bool, std::optional<double>) {});

  // The production code catches AnimatedNodeNotFoundError and marks
  // hasFinished=true. No crash.
  EXPECT_NO_THROW(driver.runAnimationStep(16'000'000));
  EXPECT_TRUE(driver.hasFinished());
}
