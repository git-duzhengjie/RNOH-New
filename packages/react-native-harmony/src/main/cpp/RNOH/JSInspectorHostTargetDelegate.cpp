/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "JSInspectorHostTargetDelegate.h"
#include <bundle/native_interface_bundle.h>
#include <cxxreact/ReactNativeVersion.h>
#include <deviceinfo.h>

namespace rnoh {

using namespace facebook::react;

JSInspectorHostTargetDelegate::JSInspectorHostTargetDelegate(
    ArkTSChannel::Weak arkTSChannel)
    : m_arkTSChannel{arkTSChannel} {}

jsinspector_modern::HostTargetMetadata
JSInspectorHostTargetDelegate::getMetadata() {
  OH_NativeBundle_ApplicationInfo appInfo =
      OH_NativeBundle_GetCurrentApplicationInfo();
  auto appId = OH_NativeBundle_GetAppId();
  auto device = OH_GetMarketName();

  std::string rnVersion = std::to_string(ReactNativeVersion.Major) + "." +
      std::to_string(ReactNativeVersion.Minor) + "." +
      std::to_string(ReactNativeVersion.Patch);
  return {
      .integrationName = "HarmonyOS Bridgeless",
      .appDisplayName = appInfo.bundleName,
      .appIdentifier = appId,
      .platform = "Harmony",
      .deviceName = device,
      .reactNativeVersion = rnVersion};
}

void JSInspectorHostTargetDelegate::onReload(const PageReloadRequest& request) {
  auto arkTSChannel = m_arkTSChannel.lock();
  if (!arkTSChannel) {
    return;
  }
  arkTSChannel->postMessage("RNOH::ON_RELOAD", {});
}

void JSInspectorHostTargetDelegate::onSetPausedInDebuggerMessage(
    const OverlaySetPausedInDebuggerMessageRequest& request) {
  auto arkTSChannel = m_arkTSChannel.lock();
  if (!arkTSChannel) {
    return;
  }

  if (request.message.has_value()) {
    folly::dynamic payload = folly::dynamic::object();
    payload["message"] = *request.message;
    arkTSChannel->postMessage("RNOH::OPEN_DEBUGGER_PAUSED_DIALOG", payload);
    return;
  }
  arkTSChannel->postMessage("RNOH::CLOSE_DEBUGGER_PAUSED_DIALOG", {});
}

} // namespace rnoh