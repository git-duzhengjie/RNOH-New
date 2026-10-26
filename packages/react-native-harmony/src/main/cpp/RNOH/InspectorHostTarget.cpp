/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "InspectorHostTarget.h"
#include <utility>
#include "RNOH/JSInspectorHostTargetDelegate.h"

namespace rnoh {

InspectorHostTarget::InspectorHostTarget(
    std::shared_ptr<ArkTSChannel> arkTSChannel,
    std::shared_ptr<NapiTaskRunner> taskRunner)
    : m_arkTSChannel(std::move(arkTSChannel)) {
  m_inspectorHostDelegate =
      std::make_unique<JSInspectorHostTargetDelegate>(m_arkTSChannel);

  m_hostTarget = facebook::react::jsinspector_modern::HostTarget::create(
      *m_inspectorHostDelegate, [runner = taskRunner](auto fn) {
        if (!runner) {
          return;
        }
        runner->runAsyncTask(std::move(fn));
      });

  m_inspectorPageId =
      facebook::react::jsinspector_modern::getInspectorInstance().addPage(
          "React Native Bridgeless (Experimental)",
          "",
          [weak = m_hostTarget->weak_from_this()](auto remote)
              -> std::unique_ptr<
                  facebook::react::jsinspector_modern::ILocalConnection> {
            auto strong = weak.lock();
            return strong->connect(std::move(remote));
          },
          {.nativePageReloads = true, .prefersFuseboxFrontend = true});
}

InspectorHostTarget::~InspectorHostTarget() {
  if (m_inspectorPageId.has_value()) {
    facebook::react::jsinspector_modern::getInspectorInstance().removePage(
        m_inspectorPageId.value());
  }
}

const std::shared_ptr<facebook::react::jsinspector_modern::HostTarget>
InspectorHostTarget::getHostTarget() const {
  return m_hostTarget;
}

void InspectorHostTarget::resumeDebugger() {
  m_hostTarget->sendCommand(
      facebook::react::jsinspector_modern::HostCommand::DebuggerResume);
}

} // namespace rnoh
