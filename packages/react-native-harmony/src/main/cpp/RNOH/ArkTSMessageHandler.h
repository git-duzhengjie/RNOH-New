/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <folly/dynamic.h>
#include <string>

namespace rnoh {
class RNInstance;

/**
 * @actor RNOH_LIBRARY
 * @thread: MAIN
 * Handles messages sent from the ArkTS side within the context of an
 * RNInstance. If a message needs to be handled by a specific component
 * or turbo module, use ArkTSMessageHub::Observer instead. The class
 * is expected to be instantiated in Package::createArkTSMessageHandlers.
 */
class ArkTSMessageHandler {
 public:
  /**
   * @brief Type alias for a shared pointer to ArkTSMessageHandler.
   * @details This type alias is used to simplify typing.
   */
  using Shared = std::shared_ptr<ArkTSMessageHandler>;

  /**
   * @brief Context structure that holds information about the ArkTS message.
   */
  struct Context {
    std::string messageName;
    folly::dynamic messagePayload;
    std::weak_ptr<RNInstance> rnInstance;
  };

  /**
   * @brief Method to handle ArkTS messages.
   * @param ctx The context of the message.
   */
  virtual void handleArkTSMessage(const Context& ctx) = 0;
};

} // namespace rnoh