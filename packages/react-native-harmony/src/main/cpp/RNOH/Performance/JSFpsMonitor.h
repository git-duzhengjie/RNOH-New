/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "RNOH/Performance/RNOHMarker.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "RNOH/UITicker.h"

namespace rnoh {

class JSFpsMonitor : public std::enable_shared_from_this<JSFpsMonitor> {
 public:
  using Shared = std::shared_ptr<JSFpsMonitor>;

  static Shared create(
      UITicker::Shared ticker,
      std::shared_ptr<TaskExecutor> taskExecutor);

  ~JSFpsMonitor();

  void start();
  void stop();
  void setPublishCallback(std::function<void(double)> cb);

 private:
  class JSProductivityMarkerListener : public RNOHMarker::RNOHMarkerListener {
   public:
    explicit JSProductivityMarkerListener(std::weak_ptr<JSFpsMonitor> monitor);
    void onMarkerReceived(
        RNOHMarker::RNOHMarkerId markerId,
        size_t rnInstanceId,
        const std::string& tag,
        double timestamp,
        uint64_t value) override;

   private:
    std::weak_ptr<JSFpsMonitor> m_monitor;
  };
  struct UIFrame {
    int64_t startNs;
    int64_t endNs;
    bool hasJSHit{false};
    bool didJSTickOnTime{false};
    explicit UIFrame(int64_t start, int64_t end) noexcept
        : startNs(start), endNs(end) {}
  };

  /// Duration of the sliding window for FPS calculations in nanoseconds
  static constexpr int64_t FPS_CALCULATION_ON_PERIOD_NS = 1'000'000'000;

  static constexpr int64_t VSYNC_60HZ_PERIOD_NS = 1'000'000'000 / 60;

  UITicker::Shared m_uiTicker;
  std::shared_ptr<TaskExecutor> m_taskExecutor;

  std::function<void()> tickerUnsubscribe{nullptr};

  std::deque<int64_t> jsCompletionNs;
  std::deque<int64_t> jsEnqueuedNs;
  std::deque<int64_t> bridgeBusyTransitions;
  std::deque<int64_t> bridgeIdleTransitions;
  std::deque<UIFrame> uiFrames;
  mutable std::mutex m_dataMutex;

  std::atomic<bool> isMonitoring{false};

  std::function<void(double)> onFps{};

  JSFpsMonitor(
      UITicker::Shared ticker,
      std::shared_ptr<TaskExecutor> taskExecutor);
  void registerListener();

  void onJSWorkCompleted(double timestampMs);
  void onJSWorkEnqueued(double timestampMs);

  void onVSyncFrame(std::chrono::steady_clock::time_point timestamp);

  void correlateJSWorkWithUIFrame(UIFrame& frame);

  /**
   * Method to get the VSync period, which can be automatically adjusted by the
   * system based on the workload
   * @return The VSync period.
   */
  int64_t getVSyncPeriod();
  JSFpsMonitor::UIFrame createUIFrame(int64_t vsyncNs, int64_t vsyncPeriodNs);
  void cleanUpOldUIFrames(int64_t cutoffNs);
  void correlateCurrentUIFrame(
      UIFrame& frame,
      int64_t execNs,
      int64_t vsyncNs,
      int64_t vsyncPeriodNs);

  void addTimestampAndCleanup(
      std::deque<int64_t>& container,
      double timestampMs);
  template <typename T, typename GetTimestampFunc>
  void cleanUpOldEntries(
      std::deque<T>& container,
      int64_t cutoffTime,
      GetTimestampFunc getTimestamp);

  std::shared_ptr<JSProductivityMarkerListener> m_markerListener;
};

} // namespace rnoh
