/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <limits>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <folly/dynamic.h>
#include "rnoh_app_napi_stubs.h"

using namespace rnoh;

// Forward declarations of functions from generated RNOHAppNapiBridge_gen.cpp
napi_value onInit(napi_env env, napi_callback_info info);
napi_value registerWorkerTurboModuleProvider(napi_env env, napi_callback_info info);
napi_value getNextRNInstanceId(napi_env env, napi_callback_info info);
napi_value createInspectorHostTarget(napi_env env, napi_callback_info info);
napi_value releaseInspectorHostTarget(napi_env env, napi_callback_info info);
napi_value resumeDebugger(napi_env env, napi_callback_info info);
napi_value onCreateRNInstance(napi_env env, napi_callback_info info);
napi_value onDestroyRNInstance(napi_env env, napi_callback_info info);
napi_value loadScript(napi_env env, napi_callback_info info);
napi_value updateSurfaceConstraints(napi_env env, napi_callback_info info);
napi_value measureSurface(napi_env env, napi_callback_info info);
napi_value createSurface(napi_env env, napi_callback_info info);
napi_value startSurface(napi_env env, napi_callback_info info);
napi_value setSurfaceProps(napi_env env, napi_callback_info info);
napi_value stopSurface(napi_env env, napi_callback_info info);
napi_value destroySurface(napi_env env, napi_callback_info info);
napi_value setSurfaceDisplayMode(napi_env env, napi_callback_info info);
napi_value emitComponentEvent(napi_env env, napi_callback_info info);
napi_value callRNFunction(napi_env env, napi_callback_info info);
napi_value onMemoryLevel(napi_env env, napi_callback_info info);
napi_value startJSFpsMonitor(napi_env env, napi_callback_info info);
napi_value stopJsFpsMonitor(napi_env env, napi_callback_info info);
napi_value updateState(napi_env env, napi_callback_info info);
napi_value logMarker(napi_env env, napi_callback_info info);
napi_value initializePerformanceMetrics(napi_env env, napi_callback_info info);
napi_value onArkTSMessage(napi_env env, napi_callback_info info);
napi_value attachRootView(napi_env env, napi_callback_info info);
napi_value detachRootView(napi_env env, napi_callback_info info);
napi_value setUIContext(napi_env env, napi_callback_info info);
napi_value getNativeNodeIdByTag(napi_env env, napi_callback_info info);
napi_value setParallelizationEnabled(napi_env env, napi_callback_info info);
napi_value setDeviceInfo(napi_env env, napi_callback_info info);
napi_value registerFont(napi_env env, napi_callback_info info);
napi_value getComponentInstancesCount(napi_env env, napi_callback_info info);
napi_value getJSRuntimeHeapUsage(napi_env env, napi_callback_info info);
napi_value getPerformanceMetricsSnapshot(napi_env env, napi_callback_info info);
napi_value invoke(napi_env env, std::function<napi_value()> operation);
int64_t normalizeHermesMemoryWaterMarkMB(double hermesMemoryWaterMark);
std::shared_ptr<RNInstanceInternal> maybeGetInstanceById(size_t instanceId);

// extern globals from generated RNOHAppNapiBridge_gen.cpp
extern std::mutex RN_INSTANCE_BY_ID_MTX;
extern std::unordered_map<size_t, std::shared_ptr<RNInstanceInternal>> RN_INSTANCE_BY_ID;
extern std::unordered_map<size_t, std::shared_ptr<InspectorHostTarget>> INSPECTOR_HOST_TARGET_BY_INSTANCE_ID;

// ============================================================================
// Helper function tests
// ============================================================================

TEST(RNOHAppNapiBridge, NormalizeHermesMemoryWaterMark_Normal) {
  EXPECT_EQ(normalizeHermesMemoryWaterMarkMB(512.0), 512);
}

TEST(RNOHAppNapiBridge, NormalizeHermesMemoryWaterMark_Zero) {
  EXPECT_EQ(normalizeHermesMemoryWaterMarkMB(0.0), DEFAULT_HERMES_MEMORY_WATER_MARK_MB);
}

TEST(RNOHAppNapiBridge, NormalizeHermesMemoryWaterMark_Negative) {
  EXPECT_EQ(normalizeHermesMemoryWaterMarkMB(-1.0), DEFAULT_HERMES_MEMORY_WATER_MARK_MB);
}

TEST(RNOHAppNapiBridge, NormalizeHermesMemoryWaterMark_Infinity) {
  EXPECT_EQ(normalizeHermesMemoryWaterMarkMB(std::numeric_limits<double>::infinity()), DEFAULT_HERMES_MEMORY_WATER_MARK_MB);
}

TEST(RNOHAppNapiBridge, NormalizeHermesMemoryWaterMark_NaN) {
  EXPECT_EQ(normalizeHermesMemoryWaterMarkMB(std::numeric_limits<double>::quiet_NaN()), DEFAULT_HERMES_MEMORY_WATER_MARK_MB);
}

TEST(RNOHAppNapiBridge, MaybeGetInstanceById_NotFound) {
  EXPECT_EQ(maybeGetInstanceById(99999), nullptr);
}

TEST(RNOHAppNapiBridge, MaybeGetInstanceById_Found) {
  auto inst = std::make_shared<RNInstanceInternal>();
  {
    std::lock_guard<std::mutex> lock(RN_INSTANCE_BY_ID_MTX);
    RN_INSTANCE_BY_ID[88888] = inst;
  }
  EXPECT_EQ(maybeGetInstanceById(88888), inst);
  {
    std::lock_guard<std::mutex> lock(RN_INSTANCE_BY_ID_MTX);
    RN_INSTANCE_BY_ID.erase(88888);
  }
}

TEST(RNOHAppNapiBridge, Invoke_Success) {
  bool called = false;
  auto result = invoke(nullptr, [&]() -> napi_value {
    called = true;
    return nullptr;
  });
  EXPECT_TRUE(called);
}

TEST(RNOHAppNapiBridge, Invoke_Throws) {
  auto result = invoke(nullptr, [&]() -> napi_value {
    throw std::runtime_error("test error");
    return nullptr;
  });
  EXPECT_EQ(result, nullptr);
}

TEST(RNOHAppNapiBridge, GetNextRNInstanceId) {
  auto result = getNextRNInstanceId(nullptr, nullptr);
  (void)result;
}

// ============================================================================
// Tests with no instance in map (covers !rnInstance true branch)
// ============================================================================

TEST(RNOHAppNapiBridge, LoadScript_NoInstance) {
  auto result = loadScript(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, UpdateSurfaceConstraints_NoInstance) {
  auto result = updateSurfaceConstraints(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, MeasureSurface_NoInstance) {
  auto result = measureSurface(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, CreateSurface_NoInstance) {
  auto result = createSurface(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, StartSurface_NoInstance) {
  auto result = startSurface(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, StopSurface_NoInstance) {
  auto result = stopSurface(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, DestroySurface_NoInstance) {
  auto result = destroySurface(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, SetSurfaceDisplayMode_NoInstance) {
  auto result = setSurfaceDisplayMode(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, EmitComponentEvent_NoInstance) {
  auto result = emitComponentEvent(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, CallRNFunction_NoInstance) {
  auto result = callRNFunction(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, StartJSFpsMonitor_NoInstance) {
  auto result = startJSFpsMonitor(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, StopJsFpsMonitor_NoInstance) {
  auto result = stopJsFpsMonitor(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, UpdateState_NoInstance) {
  auto result = updateState(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, OnArkTSMessage_NoInstance) {
  auto result = onArkTSMessage(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, AttachRootView_NoInstance) {
  auto result = attachRootView(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, DetachRootView_NoInstance) {
  auto result = detachRootView(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, SetUIContext_NoInstance) {
  auto result = setUIContext(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, GetNativeNodeIdByTag_NoInstance) {
  auto result = getNativeNodeIdByTag(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, RegisterFont_NoInstance) {
  auto result = registerFont(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, GetComponentInstancesCount_NoInstance) {
  auto result = getComponentInstancesCount(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, GetJSRuntimeHeapUsage_NoInstance) {
  auto result = getJSRuntimeHeapUsage(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, GetPerformanceMetricsSnapshot_NoInstance) {
  auto result = getPerformanceMetricsSnapshot(nullptr, nullptr);
  (void)result;
}

// ============================================================================
// Tests with instance in map (covers !rnInstance false branch)
// ============================================================================

class MockTaskExecutor : public TaskExecutor {
 public:
  void runTask(TaskThread, Task&& task) override { task(); }
  void runSyncTask(TaskThread, Task&& task) override { task(); }
};

class RNOHAppNapiBridgeWithInstance : public ::testing::Test {
 protected:
  void SetUp() override {
    inst = std::make_shared<RNInstanceCAPI>();
    inst->m_taskExecutor = std::make_shared<MockTaskExecutor>();
    inst->m_reactInstance = std::make_shared<facebook::react::ReactInstance>();
    inst->m_fontRegistry = std::make_shared<FontRegistry>();
    std::lock_guard<std::mutex> lock(RN_INSTANCE_BY_ID_MTX);
    RN_INSTANCE_BY_ID[0] = inst;
  }
  void TearDown() override {
    std::lock_guard<std::mutex> lock(RN_INSTANCE_BY_ID_MTX);
    RN_INSTANCE_BY_ID.erase(0);
  }
  std::shared_ptr<RNInstanceInternal> inst;
};

TEST_F(RNOHAppNapiBridgeWithInstance, LoadScript_WithInstance) {
  auto result = loadScript(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, CreateSurface_WithInstance) {
  auto result = createSurface(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, DestroySurface_WithInstance) {
  auto result = destroySurface(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, SetSurfaceDisplayMode_WithInstance) {
  auto result = setSurfaceDisplayMode(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, EmitComponentEvent_WithInstance) {
  auto result = emitComponentEvent(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, CallRNFunction_WithInstance) {
  auto result = callRNFunction(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, OnMemoryLevel_WithInstance) {
  auto result = onMemoryLevel(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, GetJSRuntimeHeapUsage_WithInstance) {
  auto result = getJSRuntimeHeapUsage(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, UpdateState_WithInstance) {
  auto result = updateState(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, GetNativeNodeIdByTag_WithInstance) {
  auto result = getNativeNodeIdByTag(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, GetComponentInstancesCount_WithInstance) {
  auto result = getComponentInstancesCount(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, RegisterFont_WithInstance) {
  inst->m_fontRegistry = std::make_shared<FontRegistry>();
  auto result = registerFont(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, OnDestroyRNInstance_WithInstance) {
  auto result = onDestroyRNInstance(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, StartJSFpsMonitor_WithInstance) {
  auto result = startJSFpsMonitor(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, StopJsFpsMonitor_WithInstance) {
  auto result = stopJsFpsMonitor(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, AttachRootView_WithInstance) {
  auto result = attachRootView(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, DetachRootView_WithInstance) {
  auto result = detachRootView(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, SetUIContext_WithInstance) {
  auto result = setUIContext(nullptr, nullptr);
  (void)result;
}

TEST_F(RNOHAppNapiBridgeWithInstance, GetPerformanceMetricsSnapshot_WithInstance) {
  auto result = getPerformanceMetricsSnapshot(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, OnDestroyRNInstance_NoInstance) {
  auto result = onDestroyRNInstance(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, RegisterWorkerTurboModuleProvider) {
  auto result = registerWorkerTurboModuleProvider(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, RegisterWorkerTurboModuleProvider_Twice) {
  registerWorkerTurboModuleProvider(nullptr, nullptr);
  auto result = registerWorkerTurboModuleProvider(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, OnInit_Default) {
  auto result = onInit(nullptr, nullptr);
  (void)result;
}

// ============================================================================
// Inspector tests
// ============================================================================

TEST(RNOHAppNapiBridge, CreateInspectorHostTarget_FuseboxDisabled) {
  auto result = createInspectorHostTarget(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, ReleaseInspectorHostTarget_NotFound) {
  auto result = releaseInspectorHostTarget(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, ResumeDebugger_NotFound) {
  auto result = resumeDebugger(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, ResumeDebugger_Found) {
  auto target = std::make_shared<InspectorHostTarget>();
  INSPECTOR_HOST_TARGET_BY_INSTANCE_ID[0] = target;
  auto result = resumeDebugger(nullptr, nullptr);
  (void)result;
  INSPECTOR_HOST_TARGET_BY_INSTANCE_ID.erase(0);
}

TEST(RNOHAppNapiBridge, ReleaseInspectorHostTarget_Found) {
  auto target = std::make_shared<InspectorHostTarget>();
  INSPECTOR_HOST_TARGET_BY_INSTANCE_ID[0] = target;
  auto result = releaseInspectorHostTarget(nullptr, nullptr);
  (void)result;
  EXPECT_EQ(INSPECTOR_HOST_TARGET_BY_INSTANCE_ID.count(0), 0);
}

// ============================================================================
// Other tests
// ============================================================================

TEST(RNOHAppNapiBridge, SetParallelizationEnabled) {
  auto result = setParallelizationEnabled(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, SetDeviceInfo) {
  auto result = setDeviceInfo(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, LogMarker) {
  auto result = logMarker(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, InitializePerformanceMetrics) {
  auto result = initializePerformanceMetrics(nullptr, nullptr);
  (void)result;
}

TEST(RNOHAppNapiBridge, OnMemoryLevel_EmptyMap) {
  auto result = onMemoryLevel(nullptr, nullptr);
  (void)result;
}
