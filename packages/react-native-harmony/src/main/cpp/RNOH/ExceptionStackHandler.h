/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace facebook::jsi {
class Runtime;
}

namespace rnoh {

class ExceptionStackHandler final {
 public:
  struct Context {
    size_t rnInstanceId;
    std::string bundlePath;
    std::string hspModuleName;
  };

  using ContextProvider = std::function<std::optional<Context>()>;

  static void configureCppCrashAppLogMergeOnce();
  static void installCppCrashStackBindings(
      facebook::jsi::Runtime& rt,
      ContextProvider contextProvider);
};

} // namespace rnoh
