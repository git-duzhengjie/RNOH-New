/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <ReactCommon/TurboModule.h>
#include <glog/logging.h>
#include <memory>
#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/RNInstance.h"

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * Extend this class if your TurboModule is intended to be a C++ only
 * TurboModule.
 */
class TurboModule : public facebook::react::TurboModule {
 public:
  /**
   * @brief Context struct which holds context data required by the
   * TurboModule.
   */
  struct Context {
    /**
     * @brief This is used to invoke JavaScript functions from C++.
     */
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker;

    /**
     * @deprecated Use `safeInstance` instead.
     */
    std::weak_ptr<RNInstance> instance;

    /**
     * @brief Weak pointer to the RNInstance.
     * This is used to set view properties.
     */
    RNInstance::SafeWeak safeInstance;

    /**
     * @brief Shared pointer to the ArkTSMessageHub.
     * This is used to receive messages from the ArkTS side.
     */
    std::shared_ptr<ArkTSMessageHub> arkTSMessageHub;
  };

  /**
   * @brief Type alias for shared pointer of TurboModule.
   */
  using Shared = std::shared_ptr<TurboModule>;

  /**
   * @brief Constructs a new TurboModule object.
   * @param ctx Context data for the TurboModule, including JSInvoker,
   * RNInstance, and ArkTSMessageHub.
   * @param name The name of the TurboModule.
   */
  TurboModule(Context ctx, std::string name)
      : facebook::react::TurboModule(name, ctx.jsInvoker) {}

  /**
   * @brief Destructor for TurboModule.
   */
  virtual ~TurboModule();

  /**
   * @brief Sets a property on the TurboModule.
   * Used to set a property with a given name on the js side,
   * it will call this method.
   * @param rt The JSI runtime.
   * @param name The name of the property.
   * @param value The value to set for the property.
   */
  void set(
      facebook::jsi::Runtime& rt,
      const facebook::jsi::PropNameID& name,
      const facebook::jsi::Value& value) override {
    LOG(INFO) << "Turbo Module: " << name_ << "::"
              << "set("
              << "_, \"" << name.utf8(rt) << "\", \""
              << value.toString(rt).utf8(rt) << "\")";
  }
};

} // namespace rnoh