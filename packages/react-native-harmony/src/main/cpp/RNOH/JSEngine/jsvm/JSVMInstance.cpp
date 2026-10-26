/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "JSVMInstance.h"
#include "JSVMRuntimeTargetDelegate.h"

#include <jsi/jsi.h>
#include <jsi/jsilib.h>
#include <jsinspector-modern/InspectorFlags.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>
#include "JSVMRuntime.h"

// HERMES_ENABLE_DEBUGGER should be supported in the future

using namespace facebook::jsi;
using namespace facebook::react;

namespace jsvm {

class JSVMJSRuntime : public JSRuntime {
 public:
  JSVMJSRuntime(std::unique_ptr<JSVMRuntime> runtime)
      : runtime_(std::move(runtime)) {}

  facebook::jsi::Runtime& getRuntime() noexcept override {
    return *runtime_;
  }

  jsinspector_modern::RuntimeTargetDelegate& getRuntimeTargetDelegate()
      override {
    if (!targetDelegate_) {
      targetDelegate_.emplace(runtime_);
    }
    return *targetDelegate_;
  }

  void unstable_initializeOnJsThread() override {}

 private:
  std::shared_ptr<JSVMRuntime> runtime_;
  std::optional<jsinspector_modern::JSVMRuntimeTargetDelegate> targetDelegate_;
};

// param[0] crashManager should be supported in the future
std::unique_ptr<JSRuntime> JSVMInstance::createJSRuntime(
    std::shared_ptr<CrashManager> crashManager,
    std::shared_ptr<facebook::react::MessageQueueThread> msgQueueThread,
    bool allocInOldGenBeforeTTI,
    folly::dynamic initOptions) noexcept {
  assert(msgQueueThread != nullptr);

  std::unique_ptr<JSVMRuntime> jsvmRuntime;
  msgQueueThread->runOnQueueSync([&]() {
    jsvmRuntime = std::make_unique<JSVMRuntime>(msgQueueThread, initOptions);
  });

  return std::make_unique<JSVMJSRuntime>(std::move(jsvmRuntime));
}

} // namespace jsvm
