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
 * A delegate interface for handling column node events, such as click events.
 */
class ColumnNodeDelegate {
 public:
  virtual ~ColumnNodeDelegate() = default;

  /**
   * @brief Called when the column node is clicked.
   * This method can be overridden by the implementing class to define custom
   * behavior when the column node is clicked.
   *
   * @note The default implementation does nothing.
   */
  virtual void onClick(){};
};

/**
 * @actor RNOH_LIBRARY
 * Represents a column node in the UI hierarchy, inheriting from ArkUINode.
 * This class can manage child nodes and respond to events like click actions.
 */
class ColumnNode : public ArkUINode {
 protected:
  ColumnNodeDelegate* m_columnNodeDelegate;

 public:
  /**
   * @brief Constructor that initializes the column node.
   * This sets up the node and any necessary internal state for managing child
   * nodes.
   *
   * @param context The Context of ArkUINode.
   */
  explicit ColumnNode(const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief Destructor that cleans up resources when the column node is
   * destroyed.
   */
  ~ColumnNode() override;

  /**
   * @brief Retrieves the child node at the specified index.
   *
   * @param index The index of the child node to retrieve.
   * @return ArkUI_NodeHandle A handle to the child node at the specified index.
   */
  ArkUI_NodeHandle getChildAt(std::size_t index);

  /**
   * @brief Inserts a child node at the specified index in the column.
   * If the index is invalid (e.g., negative or out of bounds), the child node
   * will be added to the end.
   *
   * @param child A reference to the child node to be added.
   * @param index The position at which to insert the child. Defaults to the end
   * if invalid.
   */
  void insertChild(ArkUINode& child, std::size_t index);

  /**
   * @brief Removes the specified child node from the column.
   * This removes the child node from the column's internal structure.
   *
   * @param child The child node to remove.
   */
  void removeChild(ArkUINode& child);

  /**
   * @brief Handles events triggered on the ColumnNode.
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
   * @brief Handles the click event for the column node.
   * This method will trigger the `onClick` method of the delegate, if one is
   * set. The delegate can implement custom behavior when the node is clicked.
   */
  void onClick();

  /**
   * @brief Sets the delegate to handle column node events.
   *
   * @param columnNodeDelegate A pointer to the delegate that will handle
   * column-specific events.
   */
  void setColumnNodeDelegate(ColumnNodeDelegate* columnNodeDelegate);
};

} // namespace rnoh