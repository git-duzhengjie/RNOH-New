/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <ReactCommon/RuntimeExecutor.h>

#include <cxxreact/MessageQueueThread.h>
#include <jsinspector-modern/ReactCdp.h>
#include <jsinspector-modern/tracing/RuntimeSamplingProfile.h>

// TODO HERMES_ENABLE_DEBUGGER

#include <memory>
#include "JSVMRuntime.h"

namespace facebook::react::jsinspector_modern {

/**
 * A RuntimeTargetDelegate that enables debugging a JSVM runtime over CDP.
 */
class JSVMRuntimeTargetDelegate : public RuntimeTargetDelegate {
 public:
  /**
   * Creates a JSVMRuntimeTargetDelegate for the given runtime.
   */
  explicit JSVMRuntimeTargetDelegate(
      std::shared_ptr<jsvm::JSVMRuntime> jsvmRuntime);

  ~JSVMRuntimeTargetDelegate() override;

  // RuntimeTargetDelegate methods

  std::unique_ptr<jsinspector_modern::RuntimeAgentDelegate> createAgentDelegate(
      jsinspector_modern::FrontendChannel frontendChannel,
      jsinspector_modern::SessionState& sessionState,
      std::unique_ptr<jsinspector_modern::RuntimeAgentDelegate::ExportedState>
          previouslyExportedState,
      const jsinspector_modern::ExecutionContextDescription&
          executionContextDescription,
      RuntimeExecutor runtimeExecutor) override;

  void addConsoleMessage(jsi::Runtime& runtime, ConsoleMessage message)
      override;

  bool supportsConsole() const override;

  std::unique_ptr<StackTrace> captureStackTrace(
      jsi::Runtime& runtime,
      size_t framesToSkip) override;

  void enableSamplingProfiler() override;

  void disableSamplingProfiler() override;

  tracing::RuntimeSamplingProfile collectSamplingProfile() override;

  std::optional<folly::dynamic> serializeStackTrace(
      const StackTrace& stackTrace) override;

 private:
  // We use the private implementation idiom to ensure this class has the same
  // layout regardless of whether HERMES_ENABLE_DEBUGGER is defined. The net
  // effect is that callers can include JSVMRuntimeTargetDelegate.h without
  // setting HERMES_ENABLE_DEBUGGER one way or the other.
  class Impl;

  std::unique_ptr<Impl> impl_;
};

} // namespace facebook::react::jsinspector_modern
