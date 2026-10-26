/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "../PerformanceMetricsRegistry.h"
#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace rnoh;
using RNOHMarkerId = RNOHMarker::RNOHMarkerId;

class PerformanceMetricsRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto& reg = PerformanceMetricsRegistry::getInstance();
    reg.addNewRNInstanceMetrics(kInstanceId);
  }

  void TearDown() override {
    auto& reg = PerformanceMetricsRegistry::getInstance();
    reg.removeRNInstanceMetrics(kInstanceId);
  }

  static constexpr size_t kInstanceId = 99999;
};

TEST_F(PerformanceMetricsRegistryTest, AddNewInstance_InitializesAllMetrics) {
  auto snap =
      PerformanceMetricsRegistry::getInstance().getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->count("BUNDLE_SIZE"), 1);
  EXPECT_EQ(snap->count("TURBO_MODULE_SETUP"), 1);
  EXPECT_EQ(snap->count("APP_STARTUP"), 1);
  EXPECT_EQ(snap->count("TURBO_MODULE_MAIN_THREAD_COUNT"), 1);
}

TEST_F(PerformanceMetricsRegistryTest, AddNewInstance_IdempotentOnDuplicate) {
  PerformanceMetricsRegistry::getInstance().addNewRNInstanceMetrics(
      kInstanceId);
  auto snap =
      PerformanceMetricsRegistry::getInstance().getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->at("BUNDLE_SIZE"), 0);
  EXPECT_EQ(snap->at("TURBO_MODULE_MAIN_THREAD_COUNT"), 0);
}

TEST_F(PerformanceMetricsRegistryTest, RemoveInstance_SnapshotReturnsNullopt) {
  PerformanceMetricsRegistry::getInstance().removeRNInstanceMetrics(
      kInstanceId);
  auto snap =
      PerformanceMetricsRegistry::getInstance().getSnapshot(kInstanceId);
  EXPECT_FALSE(snap.has_value());
}

TEST_F(
    PerformanceMetricsRegistryTest,
    OnMarkerReceived_StartStopPair_RecordsDuration) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(
      RNOHMarkerId::RUN_JS_BUNDLE_START, kInstanceId, "", 100.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::RUN_JS_BUNDLE_STOP, kInstanceId, "", 200.0, 0);
  auto snap = reg.getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_DOUBLE_EQ(snap->at("SCRIPT_EXECUTION").asDouble(), 100.0);
}

TEST_F(
    PerformanceMetricsRegistryTest,
    OnMarkerReceived_BundleSize_RecordsValue) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(RNOHMarkerId::BUNDLE_SIZE, kInstanceId, "", 0.0, 50000);
  auto snap = reg.getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->at("BUNDLE_SIZE"), 50000);
}

TEST_F(
    PerformanceMetricsRegistryTest,
    OnMarkerReceived_AppStartupStart_RecordsBothMetrics) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(
      RNOHMarkerId::APP_STARTUP_START, kInstanceId, "", 1000.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::APP_STARTUP_STOP, kInstanceId, "", 3000.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::CONTENT_APPEARED, kInstanceId, "", 3500.0, 0);
  auto snap = reg.getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_DOUBLE_EQ(snap->at("APP_STARTUP").asDouble(), 2000.0);
  EXPECT_DOUBLE_EQ(snap->at("ROOT_VIEW_TTI").asDouble(), 2500.0);
}

TEST_F(
    PerformanceMetricsRegistryTest,
    OnMarkerReceived_TurboModuleMainThread_IncrementsCounter) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(
      RNOHMarkerId::TURBO_MODULE_MAIN_THREAD, kInstanceId, "", 0.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::TURBO_MODULE_MAIN_THREAD, kInstanceId, "", 0.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::TURBO_MODULE_MAIN_THREAD, kInstanceId, "", 0.0, 0);
  auto snap = reg.getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_EQ(snap->at("TURBO_MODULE_MAIN_THREAD_COUNT"), 3);
}

TEST_F(
    PerformanceMetricsRegistryTest,
    OnMarkerReceived_UnregisteredInstance_NoEffect) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(RNOHMarkerId::BUNDLE_SIZE, 88888, "", 0.0, 42);
  auto snap = reg.getSnapshot(88888);
  EXPECT_FALSE(snap.has_value());
}

TEST_F(
    PerformanceMetricsRegistryTest,
    GetSnapshot_UnregisteredInstance_ReturnsNullopt) {
  auto snap = PerformanceMetricsRegistry::getInstance().getSnapshot(77777);
  EXPECT_FALSE(snap.has_value());
}

TEST_F(
    PerformanceMetricsRegistryTest,
    GetSnapshot_CompleteLifecycle_AllDurationsCorrect) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(RNOHMarkerId::DOWNLOAD_START, kInstanceId, "", 10.0, 0);
  reg.onMarkerReceived(RNOHMarkerId::DOWNLOAD_END, kInstanceId, "", 50.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::REACT_INSTANCE_INIT_START, kInstanceId, "", 100.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::REACT_INSTANCE_INIT_STOP, kInstanceId, "", 300.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::INIT_JS_RUNTIME_START, kInstanceId, "", 300.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::INIT_JS_RUNTIME_STOP, kInstanceId, "", 500.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::TURBO_MODULE_SETUP_START, kInstanceId, "", 500.0, 0);
  reg.onMarkerReceived(
      RNOHMarkerId::TURBO_MODULE_SETUP_STOP, kInstanceId, "", 800.0, 0);
  auto snap = reg.getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_DOUBLE_EQ(snap->at("SCRIPT_DOWNLOAD").asDouble(), 40.0);
  EXPECT_DOUBLE_EQ(snap->at("REACT_INSTANCE_INIT").asDouble(), 200.0);
  EXPECT_DOUBLE_EQ(snap->at("INIT_JS_RUNTIME").asDouble(), 200.0);
  EXPECT_DOUBLE_EQ(snap->at("TURBO_MODULE_SETUP").asDouble(), 300.0);
}

TEST_F(
    PerformanceMetricsRegistryTest,
    GetSnapshot_OnlyStartNoStop_NegativeDuration) {
  auto& reg = PerformanceMetricsRegistry::getInstance();
  reg.onMarkerReceived(
      RNOHMarkerId::RUN_JS_BUNDLE_START, kInstanceId, "", 100.0, 0);
  // No STOP marker → duration = [1](0) - [0](100) = -100
  auto snap = reg.getSnapshot(kInstanceId);
  ASSERT_TRUE(snap.has_value());
  EXPECT_DOUBLE_EQ(snap->at("SCRIPT_EXECUTION").asDouble(), -100.0);
}

class ToCStringTest : public ::testing::TestWithParam<
                          std::pair<PerformanceMetric, const char*>> {};

TEST_P(ToCStringTest, ReturnsCorrectString) {
  auto [metric, expected] = GetParam();
  EXPECT_STREQ(to_cstring(metric), expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllMetrics,
    ToCStringTest,
    ::testing::Values(
        std::make_pair(PerformanceMetric::BUNDLE_SIZE, "BUNDLE_SIZE"),
        std::make_pair(PerformanceMetric::INIT_JS_RUNTIME, "INIT_JS_RUNTIME"),
        std::make_pair(PerformanceMetric::SCRIPT_DOWNLOAD, "SCRIPT_DOWNLOAD"),
        std::make_pair(PerformanceMetric::SCRIPT_EXECUTION, "SCRIPT_EXECUTION"),
        std::make_pair(
            PerformanceMetric::REACT_INSTANCE_INIT,
            "REACT_INSTANCE_INIT"),
        std::make_pair(PerformanceMetric::ROOT_VIEW_TTI, "ROOT_VIEW_TTI"),
        std::make_pair(PerformanceMetric::APP_STARTUP, "APP_STARTUP"),
        std::make_pair(
            PerformanceMetric::TURBO_MODULE_SETUP,
            "TURBO_MODULE_SETUP"),
        std::make_pair(
            PerformanceMetric::TURBO_MODULE_MAIN_THREAD_COUNT,
            "TURBO_MODULE_MAIN_THREAD_COUNT")));
