/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <jsinspector-modern/RuntimeTarget.h>

#include "JSVMRuntimeTargetDelegate.h"

// TODO HERMES_ENABLE_DEBUGGER
#include <jsinspector-modern/FallbackRuntimeTargetDelegate.h>

#include <utility>

namespace facebook::react::jsinspector_modern {

/**
 * A stub for JSVMRuntimeTargetDelegate when JSVM is compiled without
 * debugging support.
 */
class JSVMRuntimeTargetDelegate::Impl final
    : public FallbackRuntimeTargetDelegate {
 public:
  explicit Impl(
      JSVMRuntimeTargetDelegate&,
      std::shared_ptr<jsvm::JSVMRuntime> jsvmRuntime)
      : FallbackRuntimeTargetDelegate{jsvmRuntime->description()} {}
};

JSVMRuntimeTargetDelegate::JSVMRuntimeTargetDelegate(
    std::shared_ptr<jsvm::JSVMRuntime> jsvmRuntime)
    : impl_(std::make_unique<Impl>(*this, std::move(jsvmRuntime))) {}

JSVMRuntimeTargetDelegate::~JSVMRuntimeTargetDelegate() = default;

std::unique_ptr<RuntimeAgentDelegate>
JSVMRuntimeTargetDelegate::createAgentDelegate(
    FrontendChannel frontendChannel,
    SessionState& sessionState,
    std::unique_ptr<RuntimeAgentDelegate::ExportedState>
        previouslyExportedState,
    const ExecutionContextDescription& executionContextDescription,
    RuntimeExecutor runtimeExecutor) {
  return impl_->createAgentDelegate(
      frontendChannel,
      sessionState,
      std::move(previouslyExportedState),
      executionContextDescription,
      std::move(runtimeExecutor));
}

void JSVMRuntimeTargetDelegate::addConsoleMessage(
    jsi::Runtime& runtime,
    ConsoleMessage message) {
  impl_->addConsoleMessage(runtime, std::move(message));
}

bool JSVMRuntimeTargetDelegate::supportsConsole() const {
  return impl_->supportsConsole();
}

std::unique_ptr<StackTrace> JSVMRuntimeTargetDelegate::captureStackTrace(
    jsi::Runtime& runtime,
    size_t framesToSkip) {
  return impl_->captureStackTrace(runtime, framesToSkip);
}

void JSVMRuntimeTargetDelegate::enableSamplingProfiler() {
  impl_->enableSamplingProfiler();
}

void JSVMRuntimeTargetDelegate::disableSamplingProfiler() {
  impl_->disableSamplingProfiler();
}

tracing::RuntimeSamplingProfile
JSVMRuntimeTargetDelegate::collectSamplingProfile() {
  return impl_->collectSamplingProfile();
}

std::optional<folly::dynamic> JSVMRuntimeTargetDelegate::serializeStackTrace(
    const StackTrace& stackTrace) {
  return impl_->serializeStackTrace(stackTrace);
}

} // namespace facebook::react::jsinspector_modern
