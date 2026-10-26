/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheTimeUtil.h"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — zero platform dependencies (only <chrono>/<cstdint>)
// Module: RNOH/ImageCache/src/utils/ImageCacheTimeUtil
//
// Tested function: ImageCacheGetTimestampMs() -> int64_t
//
// P5 constraint: steady_clock is monotonic but wall-clock-dependent.
// Assertions MUST NOT check exact timestamps — only monotonicity and
// non-negativity (per rnoh-gtest-case "时间依赖测试策略").
// ============================================================================

TEST(ImageCacheTimeUtil, GetTimestampMs_FirstCall_ReturnsNonNegative) {
  int64_t ts = ImageCacheGetTimestampMs();
  EXPECT_GE(ts, 0);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_ConsecutiveCalls_NonDecreasing) {
  int64_t t1 = ImageCacheGetTimestampMs();
  int64_t t2 = ImageCacheGetTimestampMs();
  EXPECT_GE(t2, t1);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_AfterShortSleep_Increases) {
  int64_t t1 = ImageCacheGetTimestampMs();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  int64_t t2 = ImageCacheGetTimestampMs();
  EXPECT_GT(t2, t1);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_ReturnsStrictlyPositive) {
  int64_t ts = ImageCacheGetTimestampMs();
  EXPECT_GT(ts, 0);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_TenRapidCalls_AllNonNegative) {
  for (int i = 0; i < 10; ++i) {
    EXPECT_GE(ImageCacheGetTimestampMs(), 0);
  }
}

TEST(ImageCacheTimeUtil, GetTimestampMs_TenRapidCalls_NonDecreasing) {
  int64_t prev = ImageCacheGetTimestampMs();
  for (int i = 0; i < 10; ++i) {
    int64_t curr = ImageCacheGetTimestampMs();
    EXPECT_GE(curr, prev);
    prev = curr;
  }
}

TEST(ImageCacheTimeUtil, GetTimestampMs_After5msSleep_DiffAtLeast5ms) {
  int64_t t1 = ImageCacheGetTimestampMs();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  int64_t t2 = ImageCacheGetTimestampMs();
  EXPECT_GE(t2 - t1, 5);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_HundredCalls_LastNotBeforeFirst) {
  int64_t first = ImageCacheGetTimestampMs();
  int64_t last = first;
  for (int i = 0; i < 100; ++i) {
    last = ImageCacheGetTimestampMs();
  }
  EXPECT_GE(last, first);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_ThreeIntervals_EachLargerThanPrevious) {
  int64_t t1 = ImageCacheGetTimestampMs();
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  int64_t t2 = ImageCacheGetTimestampMs();
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  int64_t t3 = ImageCacheGetTimestampMs();
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  int64_t t4 = ImageCacheGetTimestampMs();
  EXPECT_GT(t2, t1);
  EXPECT_GT(t3, t2);
  EXPECT_GT(t4, t3);
}

TEST(ImageCacheTimeUtil, GetTimestampMs_After10msSleep_DiffAtLeast10ms) {
  int64_t t1 = ImageCacheGetTimestampMs();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  int64_t t2 = ImageCacheGetTimestampMs();
  EXPECT_GE(t2 - t1, 10);
}
