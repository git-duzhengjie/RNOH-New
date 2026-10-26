/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <optional>
#include "../RNOHMarker.h"

using namespace rnoh;
using RNOHMarkerId = RNOHMarker::RNOHMarkerId;

// Reimplement onMarkerReceived + logStartupMarkers decision logic.
// The production class delegates to ReactMarker::StartupLogger (upstream core static
// singleton) and ReactMarker::logMarkerDone (static function), both need
// stubbing. Keep in sync with RNOH/Performance/NativePerformanceListener.cpp.

enum class MarkerAction {
  Skip,
  LogAppStartupStart,
  LogAppStartupStop,
  LogInitReactRuntimeStart,
  LogInitReactRuntimeStop,
  LogNativeRequireStart,
  LogNativeRequireStop,
  LogRunJsBundleStart,
  LogRunJsBundleStop,
};

// Reimplement onMarkerReceived + logStartupMarkers combined decision.
// startupStartTimeIsNaN: whether StartupLogger::getAppStartupStartTime()
// returns NaN
std::optional<MarkerAction> decideMarkerAction(
    RNOHMarkerId markerId,
    double timestamp,
    bool startupStartTimeIsNaN) {
  // onMarkerReceived: timestamp == 0 → early return, no processing
  if (timestamp == 0) {
    return std::nullopt;
  }

  // logStartupMarkers: switch dispatch
  switch (markerId) {
    case RNOHMarkerId::APP_STARTUP_START:
      // NaN double-record guard: if already recorded, skip
      if (!startupStartTimeIsNaN) {
        return MarkerAction::Skip;
      }
      return MarkerAction::LogAppStartupStart;
    case RNOHMarkerId::APP_STARTUP_STOP:
      return MarkerAction::LogAppStartupStop;
    case RNOHMarkerId::INIT_REACT_RUNTIME_START:
      return MarkerAction::LogInitReactRuntimeStart;
    case RNOHMarkerId::INIT_REACT_RUNTIME_STOP:
      return MarkerAction::LogInitReactRuntimeStop;
    case RNOHMarkerId::NATIVE_REQUIRE_START:
      return MarkerAction::LogNativeRequireStart;
    case RNOHMarkerId::NATIVE_REQUIRE_STOP:
      return MarkerAction::LogNativeRequireStop;
    case RNOHMarkerId::RUN_JS_BUNDLE_START:
      return MarkerAction::LogRunJsBundleStart;
    case RNOHMarkerId::RUN_JS_BUNDLE_STOP:
      return MarkerAction::LogRunJsBundleStop;
    default:
      return MarkerAction::Skip;
  }
}

// === timestamp == 0 guard ===

TEST(NativePerformanceListener, TimestampZero_ReturnsNullopt) {
  auto action = decideMarkerAction(RNOHMarkerId::APP_STARTUP_START, 0.0, true);
  EXPECT_FALSE(action.has_value());
}

TEST(NativePerformanceListener, TimestampZero_WithAnyMarker_ReturnsNullopt) {
  EXPECT_FALSE(decideMarkerAction(RNOHMarkerId::RUN_JS_BUNDLE_START, 0.0, true)
                   .has_value());
  EXPECT_FALSE(decideMarkerAction(RNOHMarkerId::APP_STARTUP_STOP, 0.0, false)
                   .has_value());
}

// === APP_STARTUP_START NaN double-record guard ===

TEST(NativePerformanceListener, AppStartupStart_StartupTimeIsNaN_LogsMarker) {
  auto action =
      decideMarkerAction(RNOHMarkerId::APP_STARTUP_START, 1000.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogAppStartupStart);
}

TEST(NativePerformanceListener, AppStartupStart_StartupTimeNotNaN_Skips) {
  auto action =
      decideMarkerAction(RNOHMarkerId::APP_STARTUP_START, 1000.0, false);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::Skip);
}

TEST(
    NativePerformanceListener,
    AppStartupStart_TimestampZeroAndNotNaN_ReturnsNullopt) {
  // timestamp guard takes priority over NaN guard
  auto action = decideMarkerAction(RNOHMarkerId::APP_STARTUP_START, 0.0, false);
  EXPECT_FALSE(action.has_value());
}

// === Other markers: always log (no NaN guard) ===

TEST(NativePerformanceListener, AppStartupStop_AlwaysLogs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::APP_STARTUP_STOP, 2000.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogAppStartupStop);
}

TEST(NativePerformanceListener, AppStartupStop_NotNaN_StillLogs) {
  // APP_STARTUP_STOP has no NaN guard — always logs regardless of
  // startupStartTime
  auto action =
      decideMarkerAction(RNOHMarkerId::APP_STARTUP_STOP, 2000.0, false);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogAppStartupStop);
}

TEST(NativePerformanceListener, InitReactRuntimeStart_Logs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::INIT_REACT_RUNTIME_START, 500.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogInitReactRuntimeStart);
}

TEST(NativePerformanceListener, InitReactRuntimeStop_Logs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::INIT_REACT_RUNTIME_STOP, 800.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogInitReactRuntimeStop);
}

TEST(NativePerformanceListener, NativeRequireStart_Logs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::NATIVE_REQUIRE_START, 300.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogNativeRequireStart);
}

TEST(NativePerformanceListener, NativeRequireStop_Logs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::NATIVE_REQUIRE_STOP, 400.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogNativeRequireStop);
}

TEST(NativePerformanceListener, RunJsBundleStart_Logs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::RUN_JS_BUNDLE_START, 600.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogRunJsBundleStart);
}

TEST(NativePerformanceListener, RunJsBundleStop_Logs) {
  auto action =
      decideMarkerAction(RNOHMarkerId::RUN_JS_BUNDLE_STOP, 700.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::LogRunJsBundleStop);
}

// === Unknown markers: skip ===

TEST(NativePerformanceListener, UnknownMarker_ReturnsSkip) {
  auto action =
      decideMarkerAction(RNOHMarkerId::CONTENT_APPEARED, 1000.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::Skip);
}

TEST(NativePerformanceListener, FabricMarker_ReturnsSkip) {
  auto action =
      decideMarkerAction(RNOHMarkerId::FABRIC_COMMIT_START, 1000.0, true);
  ASSERT_TRUE(action.has_value());
  EXPECT_EQ(*action, MarkerAction::Skip);
}
