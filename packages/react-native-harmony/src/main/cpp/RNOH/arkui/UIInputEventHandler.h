/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <arkui/native_node.h>
#include <arkui/ui_input_event.h>
#include "RNOH/TouchTarget.h"
#include "RNOH/arkui/ArkUINode.h"

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 *
 * @brief Allows the given ArkUINode to handle touch events.
 * This is intended to be used by ArkUINodes that are "Roots" of the React
 * Native component trees.
 * Examples include the "RootView" and "ModalHostingView" components.
 */
class UIInputEventHandler {
 public:
  /**
   * @brief Constructs a UIInputEventHandler for a given ArkUINode.
   *
   * This constructor registers the handler to receive NODE_TOUCH_EVENTs from
   * the native ArkUI event system. It attaches a static C-style event
   * receiver and binds this instance as the user data.
   * NOTE: The node must outlive the UIInputEventHandler.
   *
   * @param node The ArkUINode instance that will emit touch input events.
   */
  UIInputEventHandler(ArkUINode& node);

  /**
   * @brief Destroy the UIInputEventHandler object
   * Cleans up by unregistering the handler from receiving NODE_TOUCH_EVENT
   * and removing the event receiver from the native node.
   *
   */
  virtual ~UIInputEventHandler() noexcept;

  /**
   * @brief Invoked when NODE_TOUCH_EVENT is received. Overrid this method
   * to handle input events such as taps, presses, or gestures.
   *
   * @param event A pointer to the native ArkUI_UIInputEvent structure
   * representing the touch interaction.
   */
  virtual void onTouchEvent(ArkUI_UIInputEvent* event) = 0;

  /**
   * @brief The callback before the RootView is destroyed.
   * Developers may need to manually cancel the pressed touches before the
   * RootView is destroyed. Currently, this is how it's handled in Modal.
   */
  virtual void onRootViewAboutToDisappear() {}

 private:
  ArkUINode& m_node;
};
} // namespace rnoh
