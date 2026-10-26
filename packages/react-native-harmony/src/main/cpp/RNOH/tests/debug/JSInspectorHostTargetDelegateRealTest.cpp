/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests the REAL JSInspectorHostTargetDelegate production code.
// The real .cpp is added as a CMake source (not #include'd), so coverage
// goes to the production file. Deep deps (napi, uv, boost) are stubbed via
// tests/stub/. Platform calls (OH_NativeBundle_*, OH_GetMarketName) use
// existing stubs that return fixed test values.
//
// Tests getMetadata() fully (no ArkTSChannel dependency).
// Tests onReload/onSetPausedInDebuggerMessage dead-channel path (early return).
// Alive-channel behavior is covered by JSInspectorHostTargetDelegateTest.cpp
// (reimplemented logic — last resort per gtest-guide P8).

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <string>

#include "JSInspectorHostTargetDelegate.h"

using namespace rnoh;
using namespace facebook::react::jsinspector_modern;

class JSInspectorHostTargetDelegateRealTest : public ::testing::Test {
 protected:
  std::unique_ptr<JSInspectorHostTargetDelegate> delegate =
      std::make_unique<JSInspectorHostTargetDelegate>(ArkTSChannel::Weak{});
};

// === getMetadata (fully testable — no ArkTSChannel dependency) ===

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    GetMetadata_ReturnsHarmonyPlatform) {
  auto meta = delegate->getMetadata();
  EXPECT_EQ(meta.platform, "Harmony");
  EXPECT_EQ(meta.integrationName, "HarmonyOS Bridgeless");
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    GetMetadata_ReturnsAppInfoFromStub) {
  auto meta = delegate->getMetadata();
  EXPECT_EQ(meta.appIdentifier, "com.test.app");
  EXPECT_EQ(meta.appDisplayName, "com.test.app");
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    GetMetadata_ReturnsDeviceNameFromStub) {
  auto meta = delegate->getMetadata();
  EXPECT_EQ(meta.deviceName, "TestDevice");
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    GetMetadata_ReturnsReactNativeVersion) {
  auto meta = delegate->getMetadata();
  EXPECT_EQ(meta.reactNativeVersion, "0.84.1");
}

TEST_F(JSInspectorHostTargetDelegateRealTest, GetMetadata_AllFieldsNonEmpty) {
  auto meta = delegate->getMetadata();
  EXPECT_FALSE(meta.integrationName.empty());
  EXPECT_FALSE(meta.platform.empty());
  EXPECT_FALSE(meta.appIdentifier.empty());
  EXPECT_FALSE(meta.deviceName.empty());
  EXPECT_FALSE(meta.reactNativeVersion.empty());
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    GetMetadata_CalledMultipleTimes_ReturnsConsistent) {
  auto meta1 = delegate->getMetadata();
  auto meta2 = delegate->getMetadata();
  EXPECT_EQ(meta1.platform, meta2.platform);
  EXPECT_EQ(meta1.integrationName, meta2.integrationName);
  EXPECT_EQ(meta1.reactNativeVersion, meta2.reactNativeVersion);
}

// === onReload — dead channel (early return, no crash) ===

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnReload_DeadChannel_ReturnsWithoutCrash) {
  PageReloadRequest req;
  EXPECT_NO_THROW(delegate->onReload(req));
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnReload_DeadChannel_DelegateStillUsable) {
  PageReloadRequest req;
  delegate->onReload(req);
  auto meta = delegate->getMetadata();
  EXPECT_EQ(meta.platform, "Harmony");
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnReload_WithMessage_DeadChannel_ReturnsSafely) {
  PageReloadRequest req;
  req.message = "reload reason";
  EXPECT_NO_THROW(delegate->onReload(req));
}

// === onSetPausedInDebuggerMessage — dead channel (early return, no crash) ===

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnSetPaused_MessagePresentDeadChannel_ReturnsSafely) {
  OverlaySetPausedInDebuggerMessageRequest req;
  req.message = "test message";
  EXPECT_NO_THROW(delegate->onSetPausedInDebuggerMessage(req));
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnSetPaused_MessageAbsentDeadChannel_DelegateIntact) {
  OverlaySetPausedInDebuggerMessageRequest req;
  req.message = std::nullopt;
  delegate->onSetPausedInDebuggerMessage(req);
  EXPECT_EQ(delegate->getMetadata().platform, "Harmony");
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnSetPaused_EmptyStringDeadChannel_ReturnsSafely) {
  OverlaySetPausedInDebuggerMessageRequest req;
  req.message = std::string("");
  EXPECT_NO_THROW(delegate->onSetPausedInDebuggerMessage(req));
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    OnSetPaused_LongMessageDeadChannel_ReturnsSafely) {
  OverlaySetPausedInDebuggerMessageRequest req;
  req.message = std::string(5000, 'x');
  EXPECT_NO_THROW(delegate->onSetPausedInDebuggerMessage(req));
}

TEST_F(
    JSInspectorHostTargetDelegateRealTest,
    MultipleCalls_SequentialDeadChannel_AllReturnSafely) {
  PageReloadRequest reloadReq;
  OverlaySetPausedInDebuggerMessageRequest pauseReq;
  pauseReq.message = "pause1";
  EXPECT_NO_THROW(delegate->onReload(reloadReq));
  EXPECT_NO_THROW(delegate->onSetPausedInDebuggerMessage(pauseReq));
  EXPECT_NO_THROW(delegate->onReload(reloadReq));
  EXPECT_EQ(delegate->getMetadata().platform, "Harmony");
}
