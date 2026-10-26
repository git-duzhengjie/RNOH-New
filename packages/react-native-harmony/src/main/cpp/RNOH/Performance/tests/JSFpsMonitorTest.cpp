/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

// Reimplement pure algorithm parts from JSFpsMonitor.cpp (private methods).
// The class requires UITicker + TaskExecutor mocks to test through public API.
// Keep in sync with RNOH/Performance/JSFpsMonitor.cpp.

namespace {

constexpr double MS_TO_NS = 1e6;
constexpr double SEC_TO_NS = 1e9;
constexpr int64_t FPS_CALCULATION_ON_PERIOD_NS = 1'000'000'000;
constexpr int64_t VSYNC_60HZ_PERIOD_NS = 1'000'000'000 / 60;

struct UIFrame {
  int64_t startNs;
  int64_t endNs;
  bool hasJSHit{false};
  bool didJSTickOnTime{false};
  explicit UIFrame(int64_t start, int64_t end) noexcept
      : startNs(start), endNs(end) {}
};

// addTimestampAndCleanup: ms→ns conversion + push + sliding window cleanup
void addTimestampAndCleanup(
    std::deque<int64_t>& container,
    double timestampMs) {
  auto timestampNs = static_cast<int64_t>(timestampMs * MS_TO_NS);
  container.push_back(timestampNs);
  auto cutoffNs = timestampNs - FPS_CALCULATION_ON_PERIOD_NS;
  while (!container.empty() && container.front() < cutoffNs) {
    container.pop_front();
  }
}

// createUIFrame: window start = empty ? (vsyncNs - period) : last.endNs
UIFrame createUIFrame(
    const std::deque<UIFrame>& uiFrames,
    int64_t vsyncNs,
    int64_t vsyncPeriodNs) {
  int64_t windowStartNs =
      uiFrames.empty() ? (vsyncNs - vsyncPeriodNs) : uiFrames.back().endNs;
  return UIFrame(windowStartNs, vsyncNs);
}

// cleanUpOldUIFrames: remove frames where endNs < (cutoff - 1s)
void cleanUpOldUIFrames(std::deque<UIFrame>& uiFrames, int64_t cutoffNs) {
  auto cutoff = cutoffNs - FPS_CALCULATION_ON_PERIOD_NS;
  while (!uiFrames.empty() && uiFrames.front().endNs < cutoff) {
    uiFrames.pop_front();
  }
}

// correlateCurrentUIFrame: timing tolerance + delegates to correlateJSWork
void correlateCurrentUIFrame(
    UIFrame& frame,
    int64_t execNs,
    int64_t vsyncNs,
    int64_t vsyncPeriodNs,
    const std::deque<int64_t>& jsCompletionNs,
    const std::deque<int64_t>& jsEnqueuedNs) {
  const int64_t onTimeToleranceNs = vsyncPeriodNs + (vsyncPeriodNs / 2);
  frame.didJSTickOnTime = (execNs - vsyncNs) <= onTimeToleranceNs;

  if (!frame.didJSTickOnTime) {
    return;
  }

  // Case 1: JS work completed in this frame
  for (auto const& jsCompletion : jsCompletionNs) {
    if (jsCompletion >= frame.startNs && jsCompletion < frame.endNs) {
      frame.hasJSHit = true;
      return;
    }
  }

  // Case 2: No UI work enqueued in this frame
  bool workEnqueuedInFrame = std::any_of(
      jsEnqueuedNs.begin(), jsEnqueuedNs.end(), [&](const auto& timestamp) {
        return timestamp >= frame.startNs && timestamp < frame.endNs;
      });
  if (!workEnqueuedInFrame) {
    frame.hasJSHit = true;
  }
}

// FPS aggregation: hits / windowDurationSec, with divide-by-zero guard
double computeFps(const std::deque<UIFrame>& uiFrames) {
  if (uiFrames.empty()) {
    return 0.0;
  }
  int jsHitFrames = static_cast<int>(
      std::count_if(uiFrames.begin(), uiFrames.end(), [](const UIFrame& f) {
        return f.hasJSHit;
      }));
  double windowDurationSec =
      (uiFrames.back().endNs - uiFrames.front().startNs) / SEC_TO_NS;
  return windowDurationSec > 0 ? jsHitFrames / windowDurationSec : 0.0;
}

} // namespace

// === addTimestampAndCleanup ===

TEST(JSFpsMonitor, AddTimestamp_ConvertsMsToNs) {
  std::deque<int64_t> container;
  addTimestampAndCleanup(container, 5.0);
  ASSERT_EQ(container.size(), 1u);
  EXPECT_EQ(container.back(), 5'000'000); // 5ms = 5,000,000ns
}

TEST(JSFpsMonitor, AddTimestamp_MultipleTimestamps_AllKept) {
  std::deque<int64_t> container;
  addTimestampAndCleanup(container, 1.0);
  addTimestampAndCleanup(container, 2.0);
  addTimestampAndCleanup(container, 3.0);
  EXPECT_EQ(container.size(), 3u);
  EXPECT_EQ(container.front(), 1'000'000);
  EXPECT_EQ(container.back(), 3'000'000);
}

TEST(JSFpsMonitor, AddTimestamp_OldEntriesBeyondWindow_CleanedUp) {
  std::deque<int64_t> container;
  addTimestampAndCleanup(container, 0.0); // 0ns
  addTimestampAndCleanup(container, 500.0); // 500ms = 500,000,000ns
  addTimestampAndCleanup(container, 1500.0); // 1500ms = 1,500,000,000ns
  // Cutoff at 1500ms is 500ms. The 0ms entry (0ns < 500,000,000ns) is removed.
  // The 500ms entry (500,000,000ns >= 500,000,000ns) is kept.
  EXPECT_EQ(container.size(), 2u);
  EXPECT_EQ(container.front(), 500'000'000);
  EXPECT_EQ(container.back(), 1'500'000'000);
}

TEST(JSFpsMonitor, AddTimestamp_AllWithinWindow_AllKept) {
  std::deque<int64_t> container;
  addTimestampAndCleanup(container, 100.0);
  addTimestampAndCleanup(container, 200.0);
  addTimestampAndCleanup(container, 900.0);
  // All within 1s window, none should be removed
  EXPECT_EQ(container.size(), 3u);
}

TEST(JSFpsMonitor, AddTimestamp_ZeroTimestamp_AddsSingleEntry) {
  std::deque<int64_t> container;
  addTimestampAndCleanup(container, 0.0);
  EXPECT_EQ(container.size(), 1u);
  EXPECT_EQ(container.front(), 0);
}

// === createUIFrame ===

TEST(JSFpsMonitor, CreateUIFrame_EmptyFrames_UsesVsyncMinusPeriod) {
  std::deque<UIFrame> uiFrames;
  int64_t vsyncNs = 1'000'000'000; // 1s
  auto frame = createUIFrame(uiFrames, vsyncNs, VSYNC_60HZ_PERIOD_NS);
  EXPECT_EQ(frame.startNs, vsyncNs - VSYNC_60HZ_PERIOD_NS);
  EXPECT_EQ(frame.endNs, vsyncNs);
}

TEST(JSFpsMonitor, CreateUIFrame_NonEmptyFrames_UsesLastEndNs) {
  std::deque<UIFrame> uiFrames;
  uiFrames.emplace_back(0, 16'666'666);
  int64_t vsyncNs = 33'333'333;
  auto frame = createUIFrame(uiFrames, vsyncNs, VSYNC_60HZ_PERIOD_NS);
  EXPECT_EQ(frame.startNs, 16'666'666);
  EXPECT_EQ(frame.endNs, vsyncNs);
}

// === cleanUpOldUIFrames ===

TEST(JSFpsMonitor, CleanUpOldUIFrames_RemovesOldEntries) {
  std::deque<UIFrame> frames;
  frames.emplace_back(0, 16'666'666); // old: endNs < 1e9
  frames.emplace_back(1'100'000'000, 1'116'666'666); // recent: endNs > 1e9
  frames.emplace_back(1'900'000'000, 1'916'666'666); // recent
  // cutoffNs = 2e9, cutoff = 1e9. Frame 1 endNs=16,666,666 < 1e9 → removed
  cleanUpOldUIFrames(frames, 2'000'000'000);
  EXPECT_EQ(frames.size(), 2u);
  EXPECT_EQ(frames.front().startNs, 1'100'000'000);
}

TEST(JSFpsMonitor, CleanUpOldUIFrames_AllRecent_KeepsAll) {
  std::deque<UIFrame> frames;
  frames.emplace_back(1'000'000'000, 1'016'666'666);
  frames.emplace_back(1'100'000'000, 1'116'666'666);
  cleanUpOldUIFrames(frames, 1'900'000'000);
  EXPECT_EQ(frames.size(), 2u);
}

TEST(JSFpsMonitor, CleanUpOldUIFrames_Empty_NoOp) {
  std::deque<UIFrame> frames;
  cleanUpOldUIFrames(frames, 1'000'000'000);
  EXPECT_TRUE(frames.empty());
}

// === correlateCurrentUIFrame ===

TEST(JSFpsMonitor, Correlate_ExecOnTime_NoJsWork_HasJSHit) {
  UIFrame frame(0, 16'666'666);
  std::deque<int64_t> completions;
  std::deque<int64_t> enqueued;
  // execNs = 16,666,666ns (exactly at vsync), vsyncNs = 0, period = 16,666,666
  // onTimeTolerance = 16,666,666 + 8,333,333 = 25,000,000
  // (16,666,666 - 0) <= 25,000,000 → on time
  // No completions, no enqueued → hasJSHit = true (Case 2)
  correlateCurrentUIFrame(
      frame, 16'666'666, 0, 16'666'666, completions, enqueued);
  EXPECT_TRUE(frame.didJSTickOnTime);
  EXPECT_TRUE(frame.hasJSHit);
}

TEST(JSFpsMonitor, Correlate_ExecTooLate_NoJSHit) {
  UIFrame frame(0, 16'666'666);
  std::deque<int64_t> completions;
  std::deque<int64_t> enqueued;
  // execNs = 30,000,000ns, vsyncNs = 0, period = 16,666,666
  // onTimeTolerance = 25,000,000
  // (30,000,000 - 0) > 25,000,000 → NOT on time → no hit
  correlateCurrentUIFrame(
      frame, 30'000'000, 0, 16'666'666, completions, enqueued);
  EXPECT_FALSE(frame.didJSTickOnTime);
  EXPECT_FALSE(frame.hasJSHit);
}

TEST(JSFpsMonitor, Correlate_JsCompletionInFrame_HasJSHit) {
  UIFrame frame(0, 16'666'666);
  std::deque<int64_t> completions = {8'000'000}; // within [0, 16,666,666)
  std::deque<int64_t> enqueued;
  correlateCurrentUIFrame(
      frame, 16'666'666, 0, 16'666'666, completions, enqueued);
  EXPECT_TRUE(frame.didJSTickOnTime);
  EXPECT_TRUE(frame.hasJSHit);
}

TEST(JSFpsMonitor, Correlate_JsCompletionOutsideFrame_NoHit) {
  UIFrame frame(0, 16'666'666);
  std::deque<int64_t> completions = {20'000'000}; // outside [0, 16,666,666)
  std::deque<int64_t> enqueued = {
      5'000'000}; // enqueued in frame → Case 2 fails
  correlateCurrentUIFrame(
      frame, 16'666'666, 0, 16'666'666, completions, enqueued);
  EXPECT_TRUE(frame.didJSTickOnTime);
  EXPECT_FALSE(frame.hasJSHit);
}

TEST(JSFpsMonitor, Correlate_NoCompletionButWorkEnqueued_NoHit) {
  UIFrame frame(0, 16'666'666);
  std::deque<int64_t> completions;
  std::deque<int64_t> enqueued = {5'000'000}; // work enqueued in frame
  correlateCurrentUIFrame(
      frame, 16'666'666, 0, 16'666'666, completions, enqueued);
  EXPECT_TRUE(frame.didJSTickOnTime);
  EXPECT_FALSE(frame.hasJSHit); // Case 2: work was enqueued, so no hit
}

TEST(JSFpsMonitor, Correlate_ExecExactlyAtTolerance_OnTime) {
  UIFrame frame(0, 16'666'666);
  std::deque<int64_t> completions;
  std::deque<int64_t> enqueued;
  // VSYNC_60HZ_PERIOD_NS = 1,000,000,000 / 60 = 16,666,666 (integer)
  // onTimeTolerance = 16,666,666 + 8,333,333 = 24,999,999
  // execNs = 24,999,999 → (24,999,999 - 0) <= 24,999,999 → on time (boundary)
  int64_t period = 1'000'000'000 / 60; // 16,666,666
  int64_t tolerance = period + period / 2; // 24,999,999
  correlateCurrentUIFrame(frame, tolerance, 0, period, completions, enqueued);
  EXPECT_TRUE(frame.didJSTickOnTime);
}

// === computeFps ===

TEST(JSFpsMonitor, ComputeFps_AllFramesHit_ReturnsFrameCountPerSec) {
  std::deque<UIFrame> frames;
  // 60 frames spanning 1 second (0 to 1e9 ns)
  for (int i = 0; i < 60; ++i) {
    UIFrame f(i * 16'666'666LL, (i + 1) * 16'666'666LL);
    f.hasJSHit = true;
    frames.push_back(f);
  }
  double fps = computeFps(frames);
  EXPECT_NEAR(fps, 60.0, 1.0); // ~60 FPS
}

TEST(JSFpsMonitor, ComputeFps_NoFrames_ReturnsZero) {
  std::deque<UIFrame> frames;
  EXPECT_DOUBLE_EQ(computeFps(frames), 0.0);
}

TEST(JSFpsMonitor, ComputeFps_NoHits_ReturnsZero) {
  std::deque<UIFrame> frames;
  for (int i = 0; i < 10; ++i) {
    UIFrame f(i * 16'666'666LL, (i + 1) * 16'666'666LL);
    f.hasJSHit = false;
    frames.push_back(f);
  }
  double fps = computeFps(frames);
  EXPECT_DOUBLE_EQ(fps, 0.0);
}

TEST(JSFpsMonitor, ComputeFps_HalfFramesHit_ReturnsHalfFps) {
  std::deque<UIFrame> frames;
  for (int i = 0; i < 60; ++i) {
    UIFrame f(i * 16'666'666LL, (i + 1) * 16'666'666LL);
    f.hasJSHit = (i % 2 == 0);
    frames.push_back(f);
  }
  double fps = computeFps(frames);
  EXPECT_NEAR(fps, 30.0, 1.0); // ~30 FPS (half of 60)
}

TEST(JSFpsMonitor, ComputeFps_ZeroDurationWindow_ReturnsZero) {
  std::deque<UIFrame> frames;
  UIFrame f(1'000'000'000, 1'000'000'000); // start == end
  f.hasJSHit = true;
  frames.push_back(f);
  EXPECT_DOUBLE_EQ(computeFps(frames), 0.0);
}

TEST(JSFpsMonitor, ComputeFps_SingleHitFrame_ReturnsNonZeroFps) {
  std::deque<UIFrame> frames;
  UIFrame f(0, 16'666'666);
  f.hasJSHit = true;
  frames.push_back(f);
  double fps = computeFps(frames);
  EXPECT_GT(fps, 0.0);
}
