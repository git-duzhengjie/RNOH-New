/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jsinspector-modern/HostTarget.h>
#include <jsireact/JSIExecutor.h>
#include "ArkTSChannel.h"

namespace rnoh {

class JSInspectorHostTargetDelegate
    : public facebook::react::jsinspector_modern::HostTargetDelegate {
 public:
  JSInspectorHostTargetDelegate(ArkTSChannel::Weak arkTSChannel);
  JSInspectorHostTargetDelegate(const JSInspectorHostTargetDelegate&) = delete;
  JSInspectorHostTargetDelegate(JSInspectorHostTargetDelegate&&) = delete;
  JSInspectorHostTargetDelegate& operator=(
      const JSInspectorHostTargetDelegate&) = delete;
  JSInspectorHostTargetDelegate& operator=(JSInspectorHostTargetDelegate&&) =
      delete;
  ~JSInspectorHostTargetDelegate() noexcept override {}
  facebook::react::jsinspector_modern::HostTargetMetadata getMetadata()
      override;
  void onReload(const PageReloadRequest& request) override;
  void onSetPausedInDebuggerMessage(
      const OverlaySetPausedInDebuggerMessageRequest& request) override;

 private:
  ArkTSChannel::Weak m_arkTSChannel;
};

} // namespace rnoh