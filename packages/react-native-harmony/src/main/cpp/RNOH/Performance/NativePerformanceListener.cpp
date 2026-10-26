/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "NativePerformanceListener.h"

#include <cmath>

#include <cxxreact/ReactMarker.h>

#include <glog/logging.h>

namespace rnoh {

void NativePerformanceRNOHMarkerListener::onMarkerReceived(
    RNOHMarkerId markerId,
    size_t /*rnInstanceId*/,
    const std::string& /*tag*/,
    double timestamp,
    uint64_t /*value*/) {
  if (timestamp == 0) {
    return;
  }

  logStartupMarkers(markerId, timestamp);
}

void NativePerformanceRNOHMarkerListener::logStartupMarkers(RNOHMarkerId markerId,double timestamp) {

  using namespace facebook::react::ReactMarker;
  auto& startupLogger = StartupLogger::getInstance();

  switch (markerId) {
    case RNOHMarkerId::APP_STARTUP_START:
      if (!std::isnan(startupLogger.getAppStartupStartTime())) {
        // ReactInstance::loadScript logs APP_STARTUP_START again after
        // onCreateRNInstance; logMarkerDone would reset StartupLogger.
        break;
      }
      logMarkerDone(ReactMarker::APP_STARTUP_START, timestamp);
      break;
    case RNOHMarkerId::APP_STARTUP_STOP:
      logMarkerDone(ReactMarker::APP_STARTUP_STOP, timestamp);
      break;
    case RNOHMarkerId::INIT_REACT_RUNTIME_START:
      logMarkerDone(ReactMarker::INIT_REACT_RUNTIME_START, timestamp);
      break;
    case RNOHMarkerId::INIT_REACT_RUNTIME_STOP:
      logMarkerDone(ReactMarker::INIT_REACT_RUNTIME_STOP, timestamp);
      break;
    case RNOHMarkerId::NATIVE_REQUIRE_START:
      logMarkerDone(ReactMarker::NATIVE_REQUIRE_START, timestamp);
      break;
    case RNOHMarkerId::NATIVE_REQUIRE_STOP:
      logMarkerDone(ReactMarker::NATIVE_REQUIRE_STOP, timestamp);
      break;
    case RNOHMarkerId::RUN_JS_BUNDLE_START:
      logMarkerDone(ReactMarker::RUN_JS_BUNDLE_START, timestamp);
      break;
    case RNOHMarkerId::RUN_JS_BUNDLE_STOP:
      logMarkerDone(ReactMarker::RUN_JS_BUNDLE_STOP, timestamp);
      break;
    default:
      break;
  }
}
} // namespace rnoh

