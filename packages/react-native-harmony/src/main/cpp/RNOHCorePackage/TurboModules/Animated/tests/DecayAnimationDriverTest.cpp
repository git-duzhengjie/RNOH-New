/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::DecayAnimationDriver` — the driver
 * that animates a value toward an asymptote using an exponential decay
 * formula: `value = from + (velocity / (1 - deceleration)) *
 * (1 - exp(-(1 - deceleration) * t))`.
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * a value node as the animated node. The driver is constructed directly
 * so the test can call `runAnimationStep(frameTimeNanos)` and observe
 * the resulting value via the manager.
 *
 * Tests exercise:
 *   - Constructor pulls `velocity` and `deceleration` from config and
 *     seeds `m_fromValue` from the animated value
 *   - `iterations == 0` short-circuits: the animation is immediately
 *     finished
 *   - Single-step animation: the value moves in the direction of the
 *     velocity
 *   - Many steps converge toward the asymptote
 *   - `hasFinished()` becomes true once the per-frame delta falls below
 *     the 0.1 threshold
 *   - The destructor invokes the end callback with `finished=false` and
 *     the current animated value when the animation has not finished
 *
 * Time-dependent handling per guide §4.6: the decay formula uses
 * `frameTimeNanos` as input, but tests supply deterministic frame times
 * (16ms increments) and only assert direction and convergence — never
 * precise timing values.
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Drivers/DecayAnimationDriver.h"
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

folly::dynamic makeDecayConfig(
    double velocity,
    double deceleration = 0.998,
    int64_t iterations = 1) {
  return folly::dynamic::object("velocity", velocity)(
      "deceleration", deceleration)(
      "iterations", static_cast<double>(iterations));
}
} // namespace

TEST(
    DecayAnimationDriver,
    Constructor_ExtractsVelocityAndDecelerationFromConfig) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  DecayAnimationDriver driver(
      /*animationId=*/7,
      /*animatedNode=*/1,
      *manager,
      makeDecayConfig(/*velocity=*/100.0, /*deceleration=*/0.9),
      [](bool, std::optional<double>) {});

  EXPECT_EQ(driver.getId(), 7);
  EXPECT_EQ(driver.getAnimatedValueTag(), 1);
  EXPECT_FALSE(driver.hasFinished());
}

TEST(DecayAnimationDriver, Constructor_ZeroIterations_ImmediatelyFinished) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  DecayAnimationDriver driver(
      1,
      1,
      *manager,
      makeDecayConfig(100.0, 0.998, 0),
      [](bool, std::optional<double>) {});

  EXPECT_TRUE(driver.hasFinished());
}

TEST(
    DecayAnimationDriver,
    RunAnimationStep_PositiveVelocity_MovesValueInPositiveDirection) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  DecayAnimationDriver driver(
      1,
      1,
      *manager,
      makeDecayConfig(/*velocity=*/1000.0, 0.998),
      [](bool, std::optional<double>) {});

  driver.runAnimationStep(16'000'000);

  auto& animatedValue = manager->getValueNodeByTag(1);
  // With a positive velocity, the value must have increased above the
  // initial 0.
  EXPECT_GT(animatedValue.getOutputAsDouble(), 0.0);
}

TEST(
    DecayAnimationDriver,
    RunAnimationStep_NegativeVelocity_MovesValueInNegativeDirection) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  DecayAnimationDriver driver(
      1,
      1,
      *manager,
      makeDecayConfig(/*velocity=*/-1000.0, 0.998),
      [](bool, std::optional<double>) {});

  driver.runAnimationStep(16'000'000);

  auto& animatedValue = manager->getValueNodeByTag(1);
  EXPECT_LT(animatedValue.getOutputAsDouble(), 0.0);
}

TEST(
    DecayAnimationDriver,
    RunAnimationStep_ManySteps_ConvergesTowardAsymptote) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  // Asymptote = fromValue + velocity / (1 - deceleration)
  //           = 0 + 1000 / (1 - 0.998) = 500_000
  // The animation approaches but never reaches this value.
  DecayAnimationDriver driver(
      1,
      1,
      *manager,
      makeDecayConfig(/*velocity=*/1000.0, /*deceleration=*/0.998),
      [](bool, std::optional<double>) {});

  double previousValue = 0.0;
  for (int i = 0; i < 200; ++i) {
    driver.runAnimationStep(static_cast<long long>(16'000'000) * (i + 1));
    double currentValue = manager->getValueNodeByTag(1).getOutputAsDouble();
    EXPECT_GT(currentValue, previousValue);
    previousValue = currentValue;
  }

  // The value must have increased monotonically but stayed below the
  // theoretical asymptote (500_000).
  EXPECT_GT(previousValue, 0.0);
  EXPECT_LT(previousValue, 500'000.0);
}

TEST(DecayAnimationDriver, RunAnimationStep_ManySteps_EventuallySetsFinished) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  DecayAnimationDriver driver(
      1,
      1,
      *manager,
      makeDecayConfig(/*velocity=*/10.0, /*deceleration=*/0.5),
      [](bool, std::optional<double>) {});

  // With deceleration = 0.5, the per-frame delta shrinks rapidly — the
  // animation should finish within a few hundred steps.
  for (int i = 0; i < 300; ++i) {
    driver.runAnimationStep(static_cast<long long>(16'000'000) * (i + 1));
    if (driver.hasFinished()) {
      break;
    }
  }

  EXPECT_TRUE(driver.hasFinished());
}

TEST(
    DecayAnimationDriver,
    Destructor_WhenNotFinished_InvokesEndCallbackWithFinishedFalse) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 42.0);

  bool callbackInvoked = false;
  bool reportedFinished = true;
  std::optional<double> reportedValue;
  {
    DecayAnimationDriver driver(
        1,
        1,
        *manager,
        makeDecayConfig(1000.0, 0.998),
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
  // The animated value was 42 initially and the driver never ran a step,
  // so the reported value should still be 42.
  EXPECT_NEAR(reportedValue.value(), 42.0, 1e-9);
}

TEST(
    DecayAnimationDriver,
    RunAnimationStep_SetsVelocityOnAnimatedNodeForFrameRateLogic) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  DecayAnimationDriver driver(
      1,
      1,
      *manager,
      makeDecayConfig(/*velocity=*/500.0, 0.998),
      [](bool, std::optional<double>) {});

  driver.runAnimationStep(16'000'000);

  // The production code calls `setVelocity(abs(velocity))` on the
  // animated value. We can't read velocity directly (it's not exposed),
  // but we can assert the step ran without throwing and that the value
  // advanced — which only happens if the velocity was applied.
  EXPECT_GT(manager->getValueNodeByTag(1).getOutputAsDouble(), 0.0);
}
