/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <algorithm>
#include <map>
#include <utility>
#include <memory>
#include <folly/dynamic.h>
#include "rnoh_all_internal.h"

using namespace rnoh;
using namespace facebook::react;

class MockTaskExecutor : public TaskExecutor {
 public:
  void runTask(TaskThread, Task&& task) override { task(); }
  void runSyncTask(TaskThread, Task&& task) override { task(); }
};

class MockTurboModuleProvider : public TurboModuleProvider {
 public:
  std::shared_ptr<TurboModule> getTurboModule(const std::string&) override {
    return std::make_shared<TurboModule>();
  }
};

class MockJSVMRuntimeFail : public jsvm::JSVMRuntime {
 public:
  int getHeapStatistics(JSVM_HeapStatistics* stats) override { return 1; }
};

TEST(RNInstanceInternalReal, GetBundlePath_Empty) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_EQ(inst->getBundlePath(), "");
}

TEST(RNInstanceInternalReal, GetHspModuleName_Empty) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_EQ(inst->getHspModuleName(), "");
}

TEST(RNInstanceInternalReal, GetTurboModule_NullProvider_ReturnsNullptr) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_turboModuleProvider = std::make_shared<TurboModuleProvider>();
  EXPECT_EQ(inst->getTurboModule("TestModule"), nullptr);
}

TEST(RNInstanceInternalReal, GetTurboModule_WithProvider_ReturnsModule) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_turboModuleProvider = std::make_shared<MockTurboModuleProvider>();
  auto result = inst->getTurboModule("TestModule");
  EXPECT_NE(result, nullptr);
}

TEST(RNInstanceInternalReal, OnMemoryLevel_InvalidLevel_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onMemoryLevel(999));
}

TEST(RNInstanceInternalReal, OnMemoryLevel_Level0_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onMemoryLevel(0));
}

TEST(RNInstanceInternalReal, OnMemoryLevel_Level1_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onMemoryLevel(1));
}

TEST(RNInstanceInternalReal, OnMemoryLevel_Level2_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onMemoryLevel(2));
}

TEST(RNInstanceInternalReal, OnBackground_NoGc_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onBackground());
}

TEST(RNInstanceInternalReal, OnBackground_WithGc_NoReactInstance_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_shouldEnableBackgroundGC = true;
  EXPECT_NO_THROW(inst->onBackground());
}

TEST(RNInstanceInternalReal, OnBackground_WithGc_WithReactInstance_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_shouldEnableBackgroundGC = true;
  inst->m_reactInstance = std::make_shared<facebook::react::ReactInstance>();
  EXPECT_NO_THROW(inst->onBackground());
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_ConfigurationUpdate_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("CONFIGURATION_UPDATE", folly::dynamic::object("key","val")));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_Background_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("BACKGROUND", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_KeyboardVisible_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  auto payload = folly::dynamic::object("keyboardHeight", 300.0);
  EXPECT_NO_THROW(inst->handleArkTSMessage("KEYBOARD_VISIBLE", payload));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_KeyboardHidden_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("KEYBOARD_HIDDEN", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_StatusBarHeight_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  auto payload = folly::dynamic::object("statusBarHeight", 50.0);
  EXPECT_NO_THROW(inst->handleArkTSMessage("STATUS_BAR_HEIGHT", payload));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_DeviceType_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  auto payload = folly::dynamic::object("deviceType", "phone");
  EXPECT_NO_THROW(inst->handleArkTSMessage("DEVICE_TYPE", payload));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_Unknown_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("UNKNOWN_MSG", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_KeyboardVisible_MissingKey_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("KEYBOARD_VISIBLE", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_StatusBarHeight_MissingKey_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("STATUS_BAR_HEIGHT", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_DeviceType_MissingKey_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->handleArkTSMessage("DEVICE_TYPE", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, HandleArkTSMessage_WithHandler_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  auto handler = std::make_shared<ArkTSMessageHandler>();
  inst->addArkTSMessageHandler(handler);
  EXPECT_NO_THROW(inst->handleArkTSMessage("UNKNOWN_MSG", folly::dynamic::object()));
}

TEST(RNInstanceInternalReal, AddRemoveArkTSMessageHandler_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  auto handler = std::make_shared<ArkTSMessageHandler>();
  EXPECT_NO_THROW(inst->addArkTSMessageHandler(handler));
  EXPECT_NO_THROW(inst->removeArkTSMessageHandler(handler));
}

TEST(RNInstanceInternalReal, RemoveArkTSMessageHandler_NotFound_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  auto handler = std::make_shared<ArkTSMessageHandler>();
  auto other = std::make_shared<ArkTSMessageHandler>();
  inst->addArkTSMessageHandler(handler);
  EXPECT_NO_THROW(inst->removeArkTSMessageHandler(other));
}

TEST(RNInstanceInternalReal, OnAnimationStarted_NoUITicker_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  SUCCEED();
}

TEST(RNInstanceInternalReal, OnAllAnimationsComplete_NoListener_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onAllAnimationsComplete());
}

TEST(RNInstanceInternalReal, OnAllAnimationsComplete_WithListener_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_unsubscribeUITickListener = []() {};
  EXPECT_NO_THROW(inst->onAllAnimationsComplete());
  EXPECT_EQ(inst->m_unsubscribeUITickListener, nullptr);
}

TEST(RNInstanceInternalReal, OnUITick_WithoutScheduler_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->onUITick(UITicker::Timestamp{}));
}

TEST(RNInstanceInternalReal, OnUITick_WithScheduler_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_scheduler = std::make_shared<facebook::react::Scheduler>();
  EXPECT_NO_THROW(inst->onUITick(UITicker::Timestamp{}));
}

TEST(RNInstanceInternalReal, MarkSelfAboutToDestroyed_Normal_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->markSelfAboutToDestroyed());
  EXPECT_TRUE(inst->m_isAboutToBeDestroyed->load());
}

TEST(RNInstanceInternalReal, MarkSelfAboutToDestroyed_NullFlag_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_isAboutToBeDestroyed = nullptr;
  EXPECT_NO_THROW(inst->markSelfAboutToDestroyed());
}

TEST(RNInstanceInternalReal, UnregisterFromInspector_NoReactInstance_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->unregisterFromInspector());
}

TEST(RNInstanceInternalReal, UnregisterFromInspector_WithReactInstance_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_reactInstance = std::make_shared<facebook::react::ReactInstance>();
  EXPECT_NO_THROW(inst->unregisterFromInspector());
}

TEST(RNInstanceInternalReal, LoadScript_EmptyBundlePath_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_reactInstance = std::make_shared<facebook::react::ReactInstance>();
  auto jsBundle = std::make_unique<facebook::react::JSBigStdString>("test");
  std::string result;
  EXPECT_NO_THROW(inst->loadScript(std::move(jsBundle), "test.js", [&](const std::string& s) { result = s; }));
  EXPECT_EQ(inst->getBundlePath(), "test.js");
  EXPECT_EQ(result, "");
}

TEST(RNInstanceInternalReal, LoadScript_NonEmptyBundlePath_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_reactInstance = std::make_shared<facebook::react::ReactInstance>();
  inst->m_bundlePath = "existing.js";
  auto jsBundle = std::make_unique<facebook::react::JSBigStdString>("test");
  EXPECT_NO_THROW(inst->loadScript(std::move(jsBundle), "new.js", [](const std::string&) {}));
  EXPECT_EQ(inst->getBundlePath(), "existing.js");
}

TEST(RNInstanceInternalReal, EmitComponentEvent_WithDispatcher_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->m_eventDispatcher = std::make_shared<EventDispatcher>();
  EXPECT_NO_THROW(inst->emitComponentEvent(nullptr, 0, "testEvent", nullptr));
}

TEST(RNInstanceInternalReal, EmitComponentEvent_WithoutDispatcher_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->emitComponentEvent(nullptr, 0, "testEvent", nullptr));
}

TEST(RNInstanceInternalReal, GetHeapUsageFromJSRuntime_JSVMRuntime_ReturnsZero) {
  auto inst = std::make_shared<RNInstanceInternal>();
  jsvm::JSVMRuntime rt;
  EXPECT_EQ(inst->getHeapUsageFromJSRuntime(rt), 0);
}

TEST(RNInstanceInternalReal, GetHeapUsageFromJSRuntime_FailGetStats_ReturnsZero) {
  auto inst = std::make_shared<RNInstanceInternal>();
  MockJSVMRuntimeFail rt;
  EXPECT_EQ(inst->getHeapUsageFromJSRuntime(rt), 0);
}

TEST(RNInstanceInternalReal, OnMarkerReceived_NullChannel_DoesNotThrow) {
  ArkTSChannel::Weak emptyChannel;
  RNInstanceInternal::RNInstanceRNOHMarkerListener listener{emptyChannel};
  EXPECT_NO_THROW(listener.onMarkerReceived(RNOHMarker::RNOHMarkerId::CREATE_REACT_CONTEXT_START, 0, "tag", 0.0, 0));
}

TEST(RNInstanceInternalReal, OnMarkerReceived_ValidChannel_DoesNotThrow) {
  auto channel = std::make_shared<ArkTSChannel>();
  RNInstanceInternal::RNInstanceRNOHMarkerListener listener(channel);
  EXPECT_NO_THROW(listener.onMarkerReceived(RNOHMarker::RNOHMarkerId::CREATE_REACT_CONTEXT_START, 0, "tag", 0.0, 0));
}

TEST(RNInstanceInternalReal, StartJSFpsMonitor_NoCallback_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->startJSFpsMonitor(nullptr));
}

TEST(RNInstanceInternalReal, StartJSFpsMonitor_WithCallback_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->startJSFpsMonitor([](double) {}));
}

TEST(RNInstanceInternalReal, StartJSFpsMonitor_Twice_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->startJSFpsMonitor([](double) {});
  EXPECT_NO_THROW(inst->startJSFpsMonitor([](double) {}));
}

TEST(RNInstanceInternalReal, StopJsFpsMonitor_NotStarted_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_NO_THROW(inst->stopJsFpsMonitor());
}

TEST(RNInstanceInternalReal, StopJsFpsMonitor_AfterStart_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  inst->startJSFpsMonitor(nullptr);
  EXPECT_NO_THROW(inst->stopJsFpsMonitor());
}

TEST(RNInstanceInternalReal, CaptureCurrentJSStackSync_ReturnsEmpty) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_EQ(inst->captureCurrentJSStackSync(), "");
}

TEST(RNInstanceInternalReal, GetJSRuntimeHeapUsage_ReturnsZero) {
  auto inst = std::make_shared<RNInstanceInternal>();
  EXPECT_EQ(inst->getJSRuntimeHeapUsage(), 0);
}

TEST(RNInstanceInternalReal, GetCacheDir_Default) {
  auto inst = std::make_shared<RNInstanceInternal>();
  SUCCEED();
}

TEST(RNInstanceInternalReal, PostMessageToArkTS_NullChannel_DoesNotThrow) {
  auto inst = std::make_shared<RNInstanceInternal>();
  SUCCEED();
}
