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
 * This interface defines methods that can be overriden to react on StackNode
 * events
 */
class StackNodeDelegate {
 public:
  /**
   * @brief Virtual destructor ensures proper cleanup in derived classes.
   */
  virtual ~StackNodeDelegate() = default;

  /**
   * @brief Handles the click event on the StackNode.
   * This is a virtual method and should be overridden to implement custom
   * behavior.
   */
  virtual void onClick(){};

  /**
   * @brief Handles the hover-in event when the pointer enters the StackNode.
   * This is a virtual method and can be overridden to implement hover-in
   * behavior.
   */
  virtual void onHoverIn(){};

  /**
   * @brief Handles the hover-out event when the pointer leaves the StackNode.
   * This is a virtual method and can be overridden to implement hover-out
   * behavior.
   */
  virtual void onHoverOut(){};
};

/**
 * @actor RNOH_LIBRARY
 * A class that represents a StackNode, a specific UI component in a stack
 * structure. This class allows stacking of child components, and it responds
 * to events.
 */
class StackNode : public ArkUINode {
 protected:
  StackNodeDelegate* m_stackNodeDelegate;

 public:
  /**
   * @brief Constructs a new StackNode object.
   * Initializes the StackNode, setting up any necessary resources.
   *
   * @param context The Context of ArkUINode.
   */
  explicit StackNode(const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief Destroys the StackNode object.
   * Cleans up resources when the StackNode is destroyed.
   */
  ~StackNode() override;

  /**
   * @brief Inserts a child node at the specified index in the stack.
   * If the index is invalid (e.g., negative or out of bounds), the child node
   * will be added to the end.
   *
   * @param child A reference to the child node to be added.
   * @param index The position at which to insert the child. Defaults to the end
   * if invalid.
   */
  void insertChild(ArkUINode& child, std::size_t index);

  /**
   * @brief Adds a child node to the stack, placing it at the end of the current
   * children.
   *
   * @param child A reference to the child node to be added.
   */
  void addChild(ArkUINode& child);

  /**
   * @brief Removes a child node from the stack.
   * This removes the child node from its parent node.
   *
   * @param child A reference to the child node to be removed.
   */
  void removeChild(ArkUINode& child);

  /**
   * @brief Handles events triggered on the StackNode.
   *
   * This method processes various event types and delegates the appropriate
   * actions based on the event type. It ensures that the correct delegate
   * methods are invoked in response to different events such as clicks, focus,
   * etc.
   *
   * @param eventType The type of event triggered on the node (e.g.,
   * `NODE_ON_CLICK`, `NODE_ON_FOCUS`, etc.).
   *
   * @param eventArgs A reference to the event arguments that provide additional
   * details about the event (e.g., for a click event, `eventArgs[3].i32`
   * represents the input device type).
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;

  /**
   * @brief Triggers the click event for the StackNode.
   * If a delegate is set, this method calls the delegate's onClick() method.
   */
  void onClick();

  /**
   * @brief Sets the delegate to handle stack node events (click, hover, etc
   * .).  The delegate is responsible for handling the specific behaviors for
   * those events.
   *
   * @param stackNodeDelegate A pointer to the delegate that handles events for
   * this StackNode.
   */
  void setStackNodeDelegate(StackNodeDelegate* stackNodeDelegate);

  /**
   * @brief Sets the alignment of all child components within the StackNode.
   *
   * This method allows you to define how child components are aligned within
   * the `StackNode` container.
   *
   * @param align The alignment enumeration value to apply to the child
   * components (e.g.,`ARKUI_ALIGNMENT_TOP_START`, `ARKUI_ALIGNMENT_CENTER`).
   *
   * @return StackNode& A reference to the current object, allowing method
   * chaining.
   */
  StackNode& setAlign(int32_t align);
};

} // namespace rnoh