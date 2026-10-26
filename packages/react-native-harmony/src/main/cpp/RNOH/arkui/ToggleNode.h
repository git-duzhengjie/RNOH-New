/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "ArkUINode.h"

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * ToggleNodeDelegate class is an abstract base class for handling value
 * changes in a ToggleNode.
 * This class provides a virtual method `onValueChange` that must be
 * implemented by derived classes to handle value changes.
 * It is typically inherited by classes like SwitchComponentInstance
 * to manage switch-specific value changes.
 */
class ToggleNodeDelegate {
 public:
  /**
   * @brief Virtual destructor for ToggleNodeDelegate.
   * Ensures that derived classes' destructors are called correctly.
   */
  virtual ~ToggleNodeDelegate() = default;

  /**
   * @brief Called when the value of the ToggleNode changes.
   * @param value The new value of the ToggleNode.
   */
  virtual void onValueChange(int32_t& value) = 0;
};

/**
 * @actor RNOH_LIBRARY
 * ToggleNode class represents a toggle component node, inheriting from
 * ArkUINode.
 * This class is used to manage the state and behavior of a toggle component,
 * interacting with its delegate to handle value changes.
 * Usually used in concrete ComponentInstance classes.
 */
class ToggleNode : public ArkUINode {
 protected:
  /**
   * @brief Handle to the child ArkUI node.
   * @deprecated handle is currently not used in the class and is initialized to
   * `nullptr`.
   */
  ArkUI_NodeHandle m_childArkUINodeHandle;

  /**
   * @brief This delegate is typically initialized when the
   * component is constructed.
   */
  ToggleNodeDelegate* m_toggleNodeDelegate;

 public:
  /**
   * @brief Constructs a new ToggleNode object.
   * Initializes the ToggleNode by creating a new ArkUI node of type
   * `ARKUI_NODE_TOGGLE`.
   * Registers the `NODE_TOGGLE_ON_CHANGE` event.
   *
   * @param context The Context of ArkUINode.
   */
  explicit ToggleNode(const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief Destructor for ToggleNode.
   * Unregisters the `NODE_TOGGLE_ON_CHANGE` event.
   */
  ~ToggleNode();

  /**
   * @brief Handles node events.
   * @param eventType The type of the event.
   * @param eventArgs The arguments associated with the event.
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs);

  /**
   * @brief Sets the ToggleNode delegate.
   * @param delegate The new delegate to be set.
   */
  void setToggleNodeDelegate(ToggleNodeDelegate* delegate);

  /**
   * @brief Sets the selected color of the ToggleNode.
   * @param color The new selected color to be set, specified as a SharedColor.
   * @return A reference to the ToggleNode object for method chaining.
   */
  ToggleNode& setSelectedColor(facebook::react::SharedColor const& color);

  /**
   * @brief Sets the unselected color of the ToggleNode.
   * @param color The new unselected color to be set, specified as a
   * SharedColor.
   * @return A reference to the ToggleNode object for method chaining.
   */
  ToggleNode& setUnselectedColor(facebook::react::SharedColor const& color);

  /**
   * @brief Sets the thumb color of the ToggleNode.
   * @param color The new thumb color to be set, specified as a SharedColor.
   * @return A reference to the ToggleNode object for method chaining.
   */
  ToggleNode& setThumbColor(facebook::react::SharedColor const& color);

  /**
   * @brief Sets whether the ToggleNode is focusable.
   * @param focusable The new focusable state to be set.
   * @return A reference to the ToggleNode object for method chaining.
   */
  ToggleNode& setFocusable(bool focusable);

  /**
   * @brief Gets the current value of the ToggleNode.
   * @return The current value of the ToggleNode.
   */
  bool getValue();

  /**
   * @brief Sets the value of the ToggleNode.
   * @param value The new value to be set.
   * @return A reference to the ToggleNode object for method chaining.
   */
  ToggleNode& setValue(bool value);
};

} // namespace rnoh