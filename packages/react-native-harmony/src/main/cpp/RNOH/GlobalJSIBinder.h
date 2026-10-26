/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <vector>
#include "RNOH/TurboModuleFactory.h"
#include "jsi/jsi.h"

namespace rnoh {
/**
 * @actor RNOH_LIBRARY
 * This class is designed for global object creation on the JavaScript side.
 * Use it for advanced use cases, when Turbo Modules are too restrictive.
 */
class GlobalJSIBinder {
 public:
  struct Context {};
  GlobalJSIBinder(Context ctx) : m_ctx(ctx) {}

  virtual void createBindings(
      facebook::jsi::Runtime&,
      std::shared_ptr<TurboModuleProvider> provider) = 0;

 private:
  Context m_ctx;
};

using GlobalJSIBinders = std::vector<std::shared_ptr<GlobalJSIBinder>>;

} // namespace rnoh