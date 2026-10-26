/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "JSFpsMonitor.h"
#include <glog/logging.h>

namespace rnoh {

static constexpr double MS_TO_NS = 1e6; // 1ms = 1,000,000ns
static constexpr double SEC_TO_NS = 1e9; // 1s = 1,000,000,000ns

JSFpsMonitor::Shared JSFpsMonitor::create(
    UITicker::Shared ticker,
    std::shared_ptr<TaskExecutor> taskExecutor) {
  auto monitor =
      Shared(new JSFpsMonitor(std::move(ticker), std::move(taskExecutor)));
  monitor->registerListener();
  return monitor;
}

JSFpsMonitor::JSFpsMonitor(
    UITicker::Shared ticker,
    std::shared_ptr<TaskExecutor> taskExecutor)
    : m_uiTicker(std::move(ticker)), m_taskExecutor(std::move(taskExecutor)) {}

JSFpsMonitor::~JSFpsMonitor() {
  RNOHMarker::removeListener(m_markerListener);
  stop();
}

void JSFpsMonitor::registerListener() {
  m_markerListener =
      std::make_shared<JSProductivityMarkerListener>(weak_from_this());
  RNOHMarker::addListener(m_markerListener);
}

void JSFpsMonitor::start() {
  if (tickerUnsubscribe) {
    return; // Already started or missing dependencies
  }

  tickerUnsubscribe = m_uiTicker->subscribe(
      [this](std::chrono::steady_clock::time_point timestamp) {
        this->m_taskExecutor->runTask(
            TaskThread::JS, [this, timestamp]() { onVSyncFrame(timestamp); });
      });

  isMonitoring.store(true);
  LOG(INFO) << "JSFpsMonitor started";
}

void JSFpsMonitor::stop() {
  isMonitoring.store(false);

  if (tickerUnsubscribe) {
    tickerUnsubscribe();
    tickerUnsubscribe = nullptr;
  }

  LOG(INFO) << "JSFpsMonitor stopped";
}

void JSFpsMonitor::onJSWorkCompleted(double timestampMs) {
  addTimestampAndCleanup(jsCompletionNs, timestampMs);
}

void JSFpsMonitor::onJSWorkEnqueued(double timestampMs) {
  addTimestampAndCleanup(jsEnqueuedNs, timestampMs);
}

void JSFpsMonitor::onVSyncFrame(
    std::chrono::steady_clock::time_point timestamp) {
  if (!isMonitoring.load()) {
    return;
  }

  // VSync signal timestamp
  int64_t vsyncNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        timestamp.time_since_epoch())
                        .count();
  // Actual execution time on JS thread when processing this callback
  int64_t execNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count();
  int64_t vsyncPeriodNs = getVSyncPeriod();

  {
    std::scoped_lock lock{m_dataMutex};

    UIFrame currentFrame = createUIFrame(vsyncNs, vsyncPeriodNs);
    uiFrames.push_back(currentFrame);

    cleanUpOldUIFrames(vsyncNs);
    correlateCurrentUIFrame(uiFrames.back(), execNs, vsyncNs, vsyncPeriodNs);

    // Compute and publish FPS
    if (!uiFrames.empty() && onFps) {
      int jsHitFrames =
          std::count_if(uiFrames.begin(), uiFrames.end(), [](const UIFrame& f) {
            return f.hasJSHit;
          });
      double windowDurationSec =
          (uiFrames.back().endNs - uiFrames.front().startNs) / SEC_TO_NS;
      double jsFps =
          windowDurationSec > 0 ? jsHitFrames / windowDurationSec : 0.0;

      // NOTE: Calling onFps while holding mutex could cause deadlocks if the
      // callback tries to call back into JSFpsMonitor methods.
      onFps(jsFps);
    }
  }
}

void JSFpsMonitor::correlateJSWorkWithUIFrame(UIFrame& frame) {
  // If JS didn't run in time, this frame cannot be counted as a hit
  if (!frame.didJSTickOnTime) {
    return;
  }
  // Case 1: JS work was completed in this frame
  for (auto const& jsCompletion : jsCompletionNs) {
    if (jsCompletion >= frame.startNs && jsCompletion < frame.endNs) {
      frame.hasJSHit = true;
      return;
    }
  }
  // Case 2: Bridge ended idle AND no UI work enqueued in this frame
  bool workEnqueuedInFrame = std::any_of(
      jsEnqueuedNs.begin(), jsEnqueuedNs.end(), [&](const auto& timestamp) {
        return timestamp >= frame.startNs && timestamp < frame.endNs;
      });
  if (!workEnqueuedInFrame) {
    frame.hasJSHit = true;
  }
}

JSFpsMonitor::JSProductivityMarkerListener::JSProductivityMarkerListener(
    std::weak_ptr<JSFpsMonitor> monitor)
    : m_monitor(std::move(monitor)) {}

void JSFpsMonitor::JSProductivityMarkerListener::onMarkerReceived(
    RNOHMarker::RNOHMarkerId markerId,
    size_t rnInstanceId,
    const std::string& tag,
    double timestamp,
    uint64_t value) {
  auto monitor = m_monitor.lock();
  if (monitor == nullptr) {
    return;
  }
  // Track JS work completion events that contribute to UI updates. It assumes
  // that the events below are sent by the currently active RNInstance.
  switch (markerId) {
    case RNOHMarker::RNOHMarkerId::FABRIC_COMMIT_START:
    case RNOHMarker::RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_START:
    case RNOHMarker::RNOHMarkerId::FABRIC_BATCH_EXECUTION_START:
    case RNOHMarker::RNOHMarkerId::FABRIC_FINISH_TRANSACTION_START:
      monitor->onJSWorkEnqueued(timestamp);
      break;
    case RNOHMarker::RNOHMarkerId::FABRIC_COMMIT_END:
    case RNOHMarker::RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_END:
    case RNOHMarker::RNOHMarkerId::FABRIC_BATCH_EXECUTION_END:
    case RNOHMarker::RNOHMarkerId::FABRIC_FINISH_TRANSACTION_END:
      monitor->onJSWorkCompleted(timestamp);
      break;

    default:
      // Only track markers that indicate JS work completion affecting UI
      break;
  }
}

void JSFpsMonitor::setPublishCallback(std::function<void(double)> cb) {
  onFps = std::move(cb);
}

int64_t JSFpsMonitor::getVSyncPeriod() {
  int64_t vsyncPeriodNs = VSYNC_60HZ_PERIOD_NS;
  if (m_uiTicker) {
    if (auto period = m_uiTicker->getPeriodNs()) {
      vsyncPeriodNs = *period;
    }
  }
  return vsyncPeriodNs;
}

JSFpsMonitor::UIFrame JSFpsMonitor::createUIFrame(
    int64_t vsyncNs,
    int64_t vsyncPeriodNs) {
  int64_t windowStartNs =
      uiFrames.empty() ? (vsyncNs - vsyncPeriodNs) : uiFrames.back().endNs;
  return UIFrame(windowStartNs, vsyncNs);
}

void JSFpsMonitor::cleanUpOldUIFrames(int64_t cutoffNs) {
  auto cutoff = cutoffNs - FPS_CALCULATION_ON_PERIOD_NS;
  cleanUpOldEntries(
      uiFrames, cutoff, [](const UIFrame& frame) { return frame.endNs; });
}

void JSFpsMonitor::correlateCurrentUIFrame(
    UIFrame& frame,
    int64_t execNs,
    int64_t vsyncNs,
    int64_t vsyncPeriodNs) {
  const int64_t onTimeToleranceNs = vsyncPeriodNs + (vsyncPeriodNs / 2);
  frame.didJSTickOnTime = (execNs - vsyncNs) <= onTimeToleranceNs;
  correlateJSWorkWithUIFrame(frame);
}

void JSFpsMonitor::addTimestampAndCleanup(
    std::deque<int64_t>& container,
    double timestampMs) {
  std::scoped_lock lock{m_dataMutex};
  auto timestampNs = static_cast<int64_t>(timestampMs * MS_TO_NS);
  container.push_back(timestampNs);

  // Clean old entries outside window
  auto cutoffNs = timestampNs - FPS_CALCULATION_ON_PERIOD_NS;
  cleanUpOldEntries(
      container, cutoffNs, [](const int64_t& timestamp) { return timestamp; });
}

template <typename T, typename GetTimestampFunc>
void JSFpsMonitor::cleanUpOldEntries(
    std::deque<T>& container,
    int64_t cutoffTime,
    GetTimestampFunc getTimestamp) {
  while (!container.empty() && getTimestamp(container.front()) < cutoffTime) {
    container.pop_front();
  }
}

} // namespace rnoh
