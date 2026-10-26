/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

// Reimplement pure algorithm parts from HarmonyTimerRegistry.cpp (private
// methods). The class requires TaskExecutor + VSyncListener + TimerManager
// mocks. Keep in sync with RNOH/HarmonyTimerRegistry.cpp.

namespace {

struct Timer {
  uint32_t id;
  double deadlineMs;
  double durationMs;
  bool repeats;
};

// getNextDeadline: min aggregation over all timer deadlines
double getNextDeadline(const std::unordered_map<uint32_t, Timer>& timers) {
  double next = std::numeric_limits<double>::max();
  for (const auto& [id, timer] : timers) {
    next = std::min(timer.deadlineMs, next);
  }
  return next;
}

// selectExpiredTimers: select (deadline <= now), sort by deadline, return IDs
std::vector<uint32_t> selectExpiredTimers(
    const std::unordered_map<uint32_t, Timer>& timers,
    double now) {
  std::vector<Timer> expired;
  for (const auto& [id, timer] : timers) {
    if (timer.deadlineMs <= now) {
      expired.push_back(timer);
    }
  }
  std::sort(expired.begin(), expired.end(), [](const auto& a, const auto& b) {
    return a.deadlineMs < b.deadlineMs;
  });
  std::vector<uint32_t> ids;
  std::transform(
      expired.begin(),
      expired.end(),
      std::back_inserter(ids),
      [](const auto& t) { return t.id; });
  return ids;
}

// advanceOrRemoveTimers: recurring → deadline += duration; one-shot → erase
void advanceOrRemoveTimers(
    std::unordered_map<uint32_t, Timer>& timers,
    const std::vector<uint32_t>& ids) {
  for (auto id : ids) {
    auto it = timers.find(id);
    if (it == timers.end())
      continue;
    if (it->second.repeats) {
      it->second.deadlineMs += it->second.durationMs;
    } else {
      timers.erase(it);
    }
  }
}

// decideWakeUp: dedup + delay threshold dispatch
enum class WakeUpDecision { Skip, UseDelayedTask, UseVSync };
WakeUpDecision
decideWakeUp(double nextDeadline, double now, double currentNextTimerDeadline) {
  constexpr double MINIMUM_SLEEP_DELAY = 1000.0;
  auto delay = nextDeadline - now;
  if (delay >= MINIMUM_SLEEP_DELAY) {
    if (nextDeadline >= currentNextTimerDeadline) {
      return WakeUpDecision::Skip;
    }
    return WakeUpDecision::UseDelayedTask;
  }
  return WakeUpDecision::UseVSync;
}

} // namespace

// === getNextDeadline ===

TEST(HarmonyTimerRegistry, GetNextDeadline_NoTimers_ReturnsMax) {
  std::unordered_map<uint32_t, Timer> timers;
  EXPECT_EQ(getNextDeadline(timers), std::numeric_limits<double>::max());
}

TEST(HarmonyTimerRegistry, GetNextDeadline_SingleTimer_ReturnsItsDeadline) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
  };
  EXPECT_EQ(getNextDeadline(timers), 1000.0);
}

TEST(HarmonyTimerRegistry, GetNextDeadline_MultipleTimers_ReturnsEarliest) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 3000.0, 500.0, false}},
      {2, {2, 1000.0, 500.0, false}},
      {3, {3, 2000.0, 500.0, false}},
  };
  EXPECT_EQ(getNextDeadline(timers), 1000.0);
}

TEST(
    HarmonyTimerRegistry,
    GetNextDeadline_AllSameDeadline_ReturnsThatDeadline) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 5000.0, 500.0, false}},
      {2, {2, 5000.0, 500.0, false}},
  };
  EXPECT_EQ(getNextDeadline(timers), 5000.0);
}

// === selectExpiredTimers ===

TEST(HarmonyTimerRegistry, SelectExpired_NoTimersExpired_ReturnsEmpty) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 5000.0, 500.0, false}},
  };
  auto expired = selectExpiredTimers(timers, 1000.0);
  EXPECT_TRUE(expired.empty());
}

TEST(HarmonyTimerRegistry, SelectExpired_OneTimerExpired_ReturnsIt) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
      {2, {2, 3000.0, 500.0, false}},
  };
  auto expired = selectExpiredTimers(timers, 1500.0);
  ASSERT_EQ(expired.size(), 1u);
  EXPECT_EQ(expired[0], 1u);
}

TEST(HarmonyTimerRegistry, SelectExpired_MultipleExpired_SortedByDeadline) {
  std::unordered_map<uint32_t, Timer> timers = {
      {3, {3, 3000.0, 500.0, false}},
      {1, {1, 1000.0, 500.0, false}},
      {2, {2, 2000.0, 500.0, false}},
  };
  auto expired = selectExpiredTimers(timers, 5000.0);
  ASSERT_EQ(expired.size(), 3u);
  EXPECT_EQ(expired[0], 1u); // earliest deadline first
  EXPECT_EQ(expired[1], 2u);
  EXPECT_EQ(expired[2], 3u);
}

TEST(HarmonyTimerRegistry, SelectExpired_ExactlyAtDeadline_Included) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
  };
  auto expired = selectExpiredTimers(timers, 1000.0);
  ASSERT_EQ(expired.size(), 1u);
  EXPECT_EQ(expired[0], 1u);
}

TEST(HarmonyTimerRegistry, SelectExpired_JustBeforeDeadline_NotIncluded) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
  };
  auto expired = selectExpiredTimers(timers, 999.0);
  EXPECT_TRUE(expired.empty());
}

// === advanceOrRemoveTimers ===

TEST(HarmonyTimerRegistry, Advance_OneShotTimer_RemovedAfterTrigger) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
  };
  advanceOrRemoveTimers(timers, {1});
  EXPECT_EQ(timers.count(1), 0u);
}

TEST(HarmonyTimerRegistry, Advance_RecurringTimer_DeadlineAdvanced) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, true}},
  };
  advanceOrRemoveTimers(timers, {1});
  EXPECT_EQ(timers.count(1), 1u);
  EXPECT_EQ(timers.at(1).deadlineMs, 1500.0); // 1000 + 500
}

TEST(
    HarmonyTimerRegistry,
    Advance_RecurringTimerMultipleAdvances_DeadlineCumulative) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 300.0, true}},
  };
  advanceOrRemoveTimers(timers, {1});
  advanceOrRemoveTimers(timers, {1});
  advanceOrRemoveTimers(timers, {1});
  EXPECT_EQ(timers.at(1).deadlineMs, 1900.0); // 1000 + 300*3
}

TEST(
    HarmonyTimerRegistry,
    Advance_MixedTimers_OneShotRemovedRecurringAdvanced) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}}, // one-shot
      {2, {2, 2000.0, 700.0, true}}, // recurring
  };
  advanceOrRemoveTimers(timers, {1, 2});
  EXPECT_EQ(timers.count(1), 0u);
  EXPECT_EQ(timers.count(2), 1u);
  EXPECT_EQ(timers.at(2).deadlineMs, 2700.0); // 2000 + 700
}

TEST(HarmonyTimerRegistry, Advance_NonExistentId_NoEffect) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
  };
  advanceOrRemoveTimers(timers, {999});
  EXPECT_EQ(timers.size(), 1u);
}

TEST(HarmonyTimerRegistry, Advance_EmptyIdList_NoEffect) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, false}},
  };
  advanceOrRemoveTimers(timers, {});
  EXPECT_EQ(timers.size(), 1u);
}

// === decideWakeUp ===

TEST(HarmonyTimerRegistry, DecideWakeUp_DelayOverOneSecond_UsesDelayedTask) {
  EXPECT_EQ(
      decideWakeUp(2000.0, 500.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseDelayedTask);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_DelayExactlyOneSecond_UsesDelayedTask) {
  EXPECT_EQ(
      decideWakeUp(1500.0, 500.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseDelayedTask);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_DelayUnderOneSecond_UsesVSync) {
  EXPECT_EQ(
      decideWakeUp(1499.0, 500.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseVSync);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_DelayZero_UsesVSync) {
  EXPECT_EQ(
      decideWakeUp(500.0, 500.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseVSync);
}

TEST(
    HarmonyTimerRegistry,
    DecideWakeUp_NewDeadlineBeforeCurrent_SchedulesDelayedTask) {
  // nextDeadline=2000 < currentNextTimerDeadline=3000 → schedule new
  EXPECT_EQ(
      decideWakeUp(2000.0, 500.0, 3000.0), WakeUpDecision::UseDelayedTask);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_NewDeadlineAfterCurrent_Skips) {
  // nextDeadline=3000 >= currentNextTimerDeadline=2000 → skip (dedup)
  EXPECT_EQ(decideWakeUp(3000.0, 500.0, 2000.0), WakeUpDecision::Skip);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_NewDeadlineEqualsCurrent_Skips) {
  // nextDeadline=2000 >= currentNextTimerDeadline=2000 → skip (boundary)
  EXPECT_EQ(decideWakeUp(2000.0, 500.0, 2000.0), WakeUpDecision::Skip);
}

TEST(
    HarmonyTimerRegistry,
    DecideWakeUp_NoPreviousDeadline_SchedulesDelayedTask) {
  // currentNextTimerDeadline = max → always schedule
  EXPECT_EQ(
      decideWakeUp(5000.0, 1000.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseDelayedTask);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_ShortDelayWithDedup_UsesVSync) {
  // delay < 1000 → VSync regardless of dedup state
  EXPECT_EQ(decideWakeUp(800.0, 500.0, 700.0), WakeUpDecision::UseVSync);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_NegativeDelay_UsesVSync) {
  // nextDeadline < now (past deadline) → delay < 0 < 1000 → VSync
  EXPECT_EQ(
      decideWakeUp(300.0, 500.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseVSync);
}

// === Boundary Tests ===

TEST(HarmonyTimerRegistry, GetNextDeadline_WithNegativeDeadline_ReturnsNegative) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, -1000.0, 500.0, false}},
  };
  EXPECT_EQ(getNextDeadline(timers), -1000.0);
}

TEST(HarmonyTimerRegistry, GetNextDeadline_WithZeroDeadline_ReturnsZero) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 0.0, 500.0, false}},
  };
  EXPECT_EQ(getNextDeadline(timers), 0.0);
}

TEST(HarmonyTimerRegistry, SelectExpired_WithNegativeNow_SelectsExpired) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, -500.0, 500.0, false}},
      {2, {2, -1000.0, 500.0, false}},
  };
  auto expired = selectExpiredTimers(timers, -100.0);
  EXPECT_EQ(expired.size(), 2u); // Both timers are expired
}

TEST(HarmonyTimerRegistry, SelectExpired_WithEmptyTimers_ReturnsEmpty) {
  std::unordered_map<uint32_t, Timer> timers;
  auto expired = selectExpiredTimers(timers, 1000.0);
  EXPECT_TRUE(expired.empty());
}

TEST(HarmonyTimerRegistry, Advance_WithZeroDuration_RecurringTimer_KeepsSameDeadline) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 0.0, true}},
  };
  advanceOrRemoveTimers(timers, {1});
  EXPECT_EQ(timers.at(1).deadlineMs, 1000.0); // 1000 + 0
}

TEST(HarmonyTimerRegistry, Advance_WithMultipleIds_AdvancesAll) {
  std::unordered_map<uint32_t, Timer> timers = {
      {1, {1, 1000.0, 500.0, true}},
      {2, {2, 2000.0, 700.0, true}},
  };
  advanceOrRemoveTimers(timers, {1, 2});
  EXPECT_EQ(timers.at(1).deadlineMs, 1500.0);
  EXPECT_EQ(timers.at(2).deadlineMs, 2700.0);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_WithNegativeDeadline_UsesVSync) {
  // nextDeadline < 0, now = 0, delay = -1000 < 1000 → VSync
  EXPECT_EQ(
      decideWakeUp(-1000.0, 0.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseVSync);
}

TEST(HarmonyTimerRegistry, DecideWakeUp_WithVeryLargeDelay_UsesDelayedTask) {
  // delay = 1000000 >> 1000 → DelayedTask
  EXPECT_EQ(
      decideWakeUp(1001000.0, 1000.0, std::numeric_limits<double>::max()),
      WakeUpDecision::UseDelayedTask);
}
