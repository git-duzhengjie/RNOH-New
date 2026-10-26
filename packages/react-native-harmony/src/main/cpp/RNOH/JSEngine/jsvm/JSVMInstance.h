/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cxxreact/MessageQueueThread.h>
#include <react/runtime/JSRuntimeFactory.h>

using namespace facebook::react;

namespace jsvm {

class CrashManager;

class JSVMInstance {
 public:
  static std::unique_ptr<JSRuntime> createJSRuntime(
      std::shared_ptr<CrashManager> crashManager,
      std::shared_ptr<facebook::react::MessageQueueThread> msgQueueThread,
      bool allocInOldGenBeforeTTI,
      folly::dynamic initOptions) noexcept;
};

} // namespace jsvm
