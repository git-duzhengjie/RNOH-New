/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef PERFORMANCEMETRICSREGISTRY_H
#define PERFORMANCEMETRICSREGISTRY_H

#include <mutex>

#include "RNOHMarker.h"

namespace rnoh {

enum class PerformanceMetric {
  BUNDLE_SIZE,
  SCRIPT_DOWNLOAD,
  SCRIPT_EXECUTION,
  REACT_INSTANCE_INIT,
  INIT_JS_RUNTIME,
  ROOT_VIEW_TTI,
  APP_STARTUP,
  TURBO_MODULE_MAIN_THREAD_COUNT,
  TURBO_MODULE_SETUP,
};

inline const char* to_cstring(PerformanceMetric metric) {
  switch (metric) {
    case PerformanceMetric::BUNDLE_SIZE:
      return "BUNDLE_SIZE";
    case PerformanceMetric::INIT_JS_RUNTIME:
      return "INIT_JS_RUNTIME";
    case PerformanceMetric::SCRIPT_DOWNLOAD:
      return "SCRIPT_DOWNLOAD";
    case PerformanceMetric::SCRIPT_EXECUTION:
      return "SCRIPT_EXECUTION";
    case PerformanceMetric::REACT_INSTANCE_INIT:
      return "REACT_INSTANCE_INIT";
    case PerformanceMetric::ROOT_VIEW_TTI:
      return "ROOT_VIEW_TTI";
    case PerformanceMetric::APP_STARTUP:
      return "APP_STARTUP";
    case PerformanceMetric::TURBO_MODULE_SETUP:
      return "TURBO_MODULE_SETUP";
    case PerformanceMetric::TURBO_MODULE_MAIN_THREAD_COUNT:
      return "TURBO_MODULE_MAIN_THREAD_COUNT";
    default:
      return "UNKNOWN_METRIC";
  }
}

class PerformanceMetricsRegistry : public RNOHMarker::RNOHMarkerListener {
 public:
  using RNOHMarkerId = RNOHMarker::RNOHMarkerId;

  static constexpr std::size_t m_numDurationsPerMetric = 2;
  struct RNInstanceMetrics {
    std::unordered_map<
        PerformanceMetric,
        std::array<double, m_numDurationsPerMetric>>
        durationByMetric;
    std::unordered_map<PerformanceMetric, uint64_t> valueByMetric;
  };

  PerformanceMetricsRegistry() = default;
  PerformanceMetricsRegistry(const PerformanceMetricsRegistry&) = delete;
  PerformanceMetricsRegistry& operator=(const PerformanceMetricsRegistry&) =
      delete;
  PerformanceMetricsRegistry(PerformanceMetricsRegistry&&) = delete;
  PerformanceMetricsRegistry& operator=(PerformanceMetricsRegistry&&) = delete;

  static PerformanceMetricsRegistry& getInstance() {
    static PerformanceMetricsRegistry pl;
    return pl;
  }

  void addNewRNInstanceMetrics(size_t rnInstanceId) {
    std::scoped_lock lock{m_mutex};
    if (m_metricsByInstanceId.contains(rnInstanceId)) {
      return;
    }

    m_metricsByInstanceId[rnInstanceId] = {};

    for (const auto& metric : m_durationMetrics) {
      m_metricsByInstanceId[rnInstanceId].durationByMetric[metric] = {0, 0};
    }

    for (const auto& metric : m_valueMetrics) {
      m_metricsByInstanceId[rnInstanceId].valueByMetric[metric] = 0;
    }
  }

  void removeRNInstanceMetrics(size_t rnInstanceId) {
    std::scoped_lock lock{m_mutex};
    m_metricsByInstanceId.erase(rnInstanceId);
  }

  void onMarkerReceived(
      RNOHMarkerId markerId,
      size_t rnInstanceId,
      const std::string& tag,
      double timestamp,
      uint64_t value) override {
    std::scoped_lock lock{m_mutex};
    if (!m_metricsByInstanceId.contains(rnInstanceId)) {
      return;
    }

    auto& instanceMetrics = m_metricsByInstanceId.at(rnInstanceId);

    switch (markerId) {
      case RNOHMarkerId::TURBO_MODULE_SETUP_START:
        instanceMetrics
            .durationByMetric[PerformanceMetric::TURBO_MODULE_SETUP][0] =
            timestamp;
        break;
      case RNOHMarkerId::TURBO_MODULE_SETUP_STOP:
        instanceMetrics
            .durationByMetric[PerformanceMetric::TURBO_MODULE_SETUP][1] =
            timestamp;
        break;
      case RNOHMarkerId::BUNDLE_SIZE:
        instanceMetrics.valueByMetric[PerformanceMetric::BUNDLE_SIZE] = value;
        break;
      case RNOHMarkerId::DOWNLOAD_START:
        instanceMetrics
            .durationByMetric[PerformanceMetric::SCRIPT_DOWNLOAD][0] =
            timestamp;
        break;
      case RNOHMarkerId::DOWNLOAD_END:
        instanceMetrics
            .durationByMetric[PerformanceMetric::SCRIPT_DOWNLOAD][1] =
            timestamp;
        break;
      case RNOHMarkerId::RUN_JS_BUNDLE_START:
        instanceMetrics
            .durationByMetric[PerformanceMetric::SCRIPT_EXECUTION][0] =
            timestamp;
        break;
      case RNOHMarkerId::RUN_JS_BUNDLE_STOP:
        instanceMetrics
            .durationByMetric[PerformanceMetric::SCRIPT_EXECUTION][1] =
            timestamp;
        break;
      case RNOHMarkerId::REACT_INSTANCE_INIT_START:
        instanceMetrics
            .durationByMetric[PerformanceMetric::REACT_INSTANCE_INIT][0] =
            timestamp;
        break;
      case RNOHMarkerId::REACT_INSTANCE_INIT_STOP:
        instanceMetrics
            .durationByMetric[PerformanceMetric::REACT_INSTANCE_INIT][1] =
            timestamp;
        break;
      case RNOHMarkerId::INIT_JS_RUNTIME_START:
        instanceMetrics
            .durationByMetric[PerformanceMetric::INIT_JS_RUNTIME][0] =
            timestamp;
        break;
      case RNOHMarkerId::INIT_JS_RUNTIME_STOP:
        instanceMetrics
            .durationByMetric[PerformanceMetric::INIT_JS_RUNTIME][1] =
            timestamp;
        break;
      case RNOHMarkerId::APP_STARTUP_START:
        instanceMetrics.durationByMetric[PerformanceMetric::ROOT_VIEW_TTI][0] =
            timestamp;
        instanceMetrics.durationByMetric[PerformanceMetric::APP_STARTUP][0] =
            timestamp;
        break;
      case RNOHMarkerId::APP_STARTUP_STOP:
        instanceMetrics.durationByMetric[PerformanceMetric::APP_STARTUP][1] =
            timestamp;
        break;
      case RNOHMarkerId::CONTENT_APPEARED:
        instanceMetrics.durationByMetric[PerformanceMetric::ROOT_VIEW_TTI][1] =
            timestamp;
        break;
      case RNOHMarkerId::TURBO_MODULE_MAIN_THREAD:
        instanceMetrics
            .valueByMetric[PerformanceMetric::TURBO_MODULE_MAIN_THREAD_COUNT]++;
        break;
      default:
        break;
    }
  }

  std::optional<folly::dynamic> getSnapshot(size_t rnInstanceId) {
    std::scoped_lock lock{m_mutex};
    if (!m_metricsByInstanceId.contains(rnInstanceId)) {
      return std::nullopt;
    }

    auto perfMetrics = m_metricsByInstanceId.at(rnInstanceId);

    folly::dynamic obj = folly::dynamic::object();
    for (auto& metric :
         PerformanceMetricsRegistry::getInstance().m_valueMetrics) {
      obj[to_cstring(metric)] = perfMetrics.valueByMetric[metric];
    }

    for (auto& metric :
         PerformanceMetricsRegistry::getInstance().m_durationMetrics) {
      obj[to_cstring(metric)] = perfMetrics.durationByMetric[metric][1] -
          perfMetrics.durationByMetric[metric][0];
    }

    return obj;
  }

 private:
  mutable std::mutex m_mutex;
  std::vector<PerformanceMetric> m_durationMetrics = {
      PerformanceMetric::TURBO_MODULE_SETUP,
      PerformanceMetric::INIT_JS_RUNTIME,
      PerformanceMetric::SCRIPT_DOWNLOAD,
      PerformanceMetric::SCRIPT_EXECUTION,
      PerformanceMetric::REACT_INSTANCE_INIT,
      PerformanceMetric::APP_STARTUP,
      PerformanceMetric::ROOT_VIEW_TTI};
  std::vector<PerformanceMetric> m_valueMetrics = {
      PerformanceMetric::BUNDLE_SIZE,
      PerformanceMetric::TURBO_MODULE_MAIN_THREAD_COUNT};

  std::unordered_map<size_t, RNInstanceMetrics> m_metricsByInstanceId;
};

} // namespace rnoh

#endif
