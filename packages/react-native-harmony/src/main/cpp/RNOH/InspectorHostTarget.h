/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <optional>
#include "ArkTSChannel.h"
#include "RNOH/JSInspectorHostTargetDelegate.h"

namespace rnoh {

/**
 * @internal
 */
class InspectorHostTarget {
 public:
  InspectorHostTarget(
      std::shared_ptr<ArkTSChannel> arkTSChannel,
      std::shared_ptr<NapiTaskRunner> taskRunner);
  ~InspectorHostTarget();

  const std::shared_ptr<facebook::react::jsinspector_modern::HostTarget>
  getHostTarget() const;
  void resumeDebugger();

 private:
  std::shared_ptr<ArkTSChannel> m_arkTSChannel;
  std::optional<size_t> m_inspectorPageId;
  std::unique_ptr<JSInspectorHostTargetDelegate> m_inspectorHostDelegate;
  std::shared_ptr<facebook::react::jsinspector_modern::HostTarget> m_hostTarget;
};

} // namespace rnoh
