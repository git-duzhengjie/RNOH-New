/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "ArkTSMessageHandler.h"
#include "Subject.h"

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * Struct representing a message in ArkTS system, holding a name and a dynamic
 * payload.
 */
struct ArkTSMessage {
  std::string name;
  folly::dynamic payload;
};

/**
 * @actor RNOH_LIBRARY
 * ArkTSMessageHub class is responsible for handling and distributing
 * ArkTSMessages. It inherits from ArkTSMessageHandler and Subject<ArkTSMessage>
 * to manage and notify observers.
 */
class ArkTSMessageHub final : public ArkTSMessageHandler,
                              public Subject<ArkTSMessage> {
 public:
  /**
   * @brief Type alias for a shared pointer to ArkTSMessageHub.
   * @details This type alias is used to simplify typing.
   */
  using Shared = std::shared_ptr<ArkTSMessageHub>;

  /**
   * @brief Type alias for a weak pointer to ArkTSMessageHub.
   * @details This type alias is used to simplify typing.
   */
  using Weak = std::weak_ptr<ArkTSMessageHub>;

  /**
   * @actor RNOH_LIBRARY
   * @thread: MAIN
   * Inherit this class and override `onMessageReceived` if a
   * CppComponentInstance or TurboModule should handle a message ArkTSMessage.
   * If neither CppComponentInstance nor TurboModule is the appropriate
   * recipient, use ArkTSMessageHandler instead.
   */
  class Observer : private Subject::Observer {
   public:
    /**
     * Constructor: Initializes the Observer with a shared ArkTSMessageHub
     * subject.
     * @param subject Shared pointer to the ArkTSMessageHub.
     */
    Observer(ArkTSMessageHub::Shared const& subject)
        : Subject::Observer(std::static_pointer_cast<Subject>(subject)) {}

    /**
     * This function is called when an ArkTSMessage is received and should be
     * overridden to define how the message is handled.
     * @param payload The received ArkTSMessage.
     */
    virtual void onMessageReceived(ArkTSMessage const& payload) = 0;

   protected:
    /**
     * @internal
     */
    void onUpdate(ArkTSMessage const& payload) final {
      onMessageReceived(payload);
    }
  };

  /**
   * @internal
   */
  void handleArkTSMessage(const Context& ctx) override {
    notifyObservers({ctx.messageName, ctx.messagePayload});
  }
};

} // namespace rnoh