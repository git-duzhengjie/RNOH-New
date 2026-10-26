/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "CircularBuffer.h"
#include "LeastSquareSolver.h"

#define private public
#include "VelocityTracker.h"
#undef private

#include <gtest/gtest.h>

using namespace rnoh;

namespace {
constexpr double kVelocityEpsilon = 1e-6;

class VelocityTrackerTestAccess {
 public:
  static void
  addAt(VelocityTracker& tracker, double x, double y, int64_t time) {
    tracker.samples_.push({x, y, time});
  }

  static size_t sampleCount(const VelocityTracker& tracker) {
    return tracker.samples_.size();
  }
};
} // namespace

TEST(VelocityTracker, Constructor_WithNoSamples_ReturnsZeroVelocity) {
  VelocityTracker tracker;

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_DOUBLE_EQ(velocityX, 0.0);
  EXPECT_DOUBLE_EQ(velocityY, 0.0);
}

TEST(VelocityTracker, Add_DefaultClock_StoresSample) {
  VelocityTracker tracker;

  tracker.add(10.0, 20.0);

  EXPECT_EQ(VelocityTrackerTestAccess::sampleCount(tracker), 1u);
}

TEST(VelocityTracker, GetVelocity_WithOneSample_ReturnsZeroVelocity) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 20.0, 0);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_DOUBLE_EQ(velocityX, 0.0);
  EXPECT_DOUBLE_EQ(velocityY, 0.0);
}

TEST(VelocityTracker, GetVelocity_WithTwoSamples_ReturnsZeroVelocity) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 10.0, 10);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_DOUBLE_EQ(velocityX, 0.0);
  EXPECT_DOUBLE_EQ(velocityY, 0.0);
}

TEST(VelocityTracker, GetVelocity_RightwardMotion_ReturnsPositiveXVelocity) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 5.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 5.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 20.0, 5.0, 20);
  VelocityTrackerTestAccess::addAt(tracker, 30.0, 5.0, 30);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, 1.0, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_LeftwardMotion_ReturnsNegativeXVelocity) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 30.0, 5.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 20.0, 5.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 5.0, 20);
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 5.0, 30);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, -1.0, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_DiagonalMotion_ReturnsBothVelocities) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 30.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 5.0, 20.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 10.0, 20);
  VelocityTrackerTestAccess::addAt(tracker, 15.0, 0.0, 30);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, 0.5, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, -1.0, kVelocityEpsilon);
}

TEST(VelocityTracker, Reset_AfterMotion_ClearsVelocityAndAllowsReuse) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 0.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 20.0, 0.0, 20);
  tracker.reset();

  auto [resetVelocityX, resetVelocityY] = tracker.getVelocity();
  EXPECT_DOUBLE_EQ(resetVelocityX, 0.0);
  EXPECT_DOUBLE_EQ(resetVelocityY, 0.0);

  VelocityTrackerTestAccess::addAt(tracker, 20.0, 0.0, 100);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 0.0, 110);
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 120);
  auto [reusedVelocityX, reusedVelocityY] = tracker.getVelocity();
  EXPECT_NEAR(reusedVelocityX, -1.0, kVelocityEpsilon);
  EXPECT_NEAR(reusedVelocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_SampleBeyondHorizon_IgnoresOldOutlier) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 10000.0, 0.0, 0);
  for (int64_t time = 40; time <= 320; time += 40) {
    VelocityTrackerTestAccess::addAt(
        tracker, static_cast<double>(time), 0.0, time);
  }

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, 1.0, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_AfterMotionPause_UsesLatestMotionOnly) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 0.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 20.0, 0.0, 20);
  VelocityTrackerTestAccess::addAt(tracker, 1000.0, 0.0, 100);
  VelocityTrackerTestAccess::addAt(tracker, 1000.0, 0.0, 110);
  VelocityTrackerTestAccess::addAt(tracker, 1000.0, 0.0, 120);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, 0.0, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_ExceedingHistoryCapacity_IgnoresOldSamples) {
  VelocityTracker tracker;
  for (int64_t index = 0; index < 25; ++index) {
    auto time = index * 10;
    auto position = index < 5 ? 10000.0 : static_cast<double>(time);
    VelocityTrackerTestAccess::addAt(tracker, position, 0.0, time);
  }

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, 1.0, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_StationarySamples_ReturnsZeroVelocity) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 42.0, -7.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 42.0, -7.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 42.0, -7.0, 20);
  VelocityTrackerTestAccess::addAt(tracker, 42.0, -7.0, 30);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_NEAR(velocityX, 0.0, kVelocityEpsilon);
  EXPECT_NEAR(velocityY, 0.0, kVelocityEpsilon);
}

TEST(VelocityTracker, GetVelocity_RepeatedTimestamps_ReturnsZeroVelocity) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 5.0, 10);
  VelocityTrackerTestAccess::addAt(tracker, 20.0, 10.0, 10);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_DOUBLE_EQ(velocityX, 0.0);
  EXPECT_DOUBLE_EQ(velocityY, 0.0);
}

TEST(VelocityTracker, GetVelocity_PauseBeforeEnoughFreshSamples_ReturnsZero) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 100.0, 0.0, 100);
  VelocityTrackerTestAccess::addAt(tracker, 110.0, 0.0, 150);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_DOUBLE_EQ(velocityX, 0.0);
  EXPECT_DOUBLE_EQ(velocityY, 0.0);
}

TEST(
    VelocityTracker,
    GetVelocity_SamplesPastHorizonWithoutFreshFit_ReturnsZero) {
  VelocityTracker tracker;
  VelocityTrackerTestAccess::addAt(tracker, 0.0, 0.0, 0);
  VelocityTrackerTestAccess::addAt(tracker, 10.0, 0.0, 100);
  VelocityTrackerTestAccess::addAt(tracker, 20.0, 0.0, 400);

  auto [velocityX, velocityY] = tracker.getVelocity();

  EXPECT_DOUBLE_EQ(velocityX, 0.0);
  EXPECT_DOUBLE_EQ(velocityY, 0.0);
}
