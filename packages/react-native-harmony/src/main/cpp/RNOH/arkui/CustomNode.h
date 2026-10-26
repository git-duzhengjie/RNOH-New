/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "ArkUINode.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"

// When the enum is available, use it; otherwise use the known integer value
#ifdef NODE_ON_CUSTOM_OVERFLOW_SCROLL
constexpr int CUSTOM_OVERFLOW_SCROLL_EVENT =
    static_cast<int>(ArkUI_NodeEventType::NODE_ON_CUSTOM_OVERFLOW_SCROLL);
#else
// Known value for NODE_ON_CUSTOM_OVERFLOW_SCROLL in API 23+
// This needs to match the actual enum value from the SDK
constexpr int CUSTOM_OVERFLOW_SCROLL_EVENT =
    34; // Placeholder - needs actual value
#endif

namespace rnoh {
/**
 * @actor RNOH_LIBRARY
 *
 * @brief Used to react on CustomNode events. Usually implemented by concrete
 * ComponentInstance classes and activated by calling
 * CustomNode::setCustomNodeDelegate method.
 */
class CustomNodeDelegate {
 public:
  virtual ~CustomNodeDelegate() = default;

  /**
   * @brief Triggered when a node is clicked.
   */
  virtual void onClick(){};

  /**
   * @brief Triggered when the mouse or gesture enters the node area.
   */
  virtual void onHoverIn(){};

  /**
   * @brief Triggered when the mouse or gesture leaves the node area
   */
  virtual void onHoverOut(){};

  virtual void onScroll(facebook::react::Tag nodeId, float offset){};
};

/**
 * @actor RNOH_LIBRARY
 *
 * @brief Custom ArkUI node component class, used to create components that
 * support layout measurement, custom event response, and child node management.
 * As a library developer, Containers with non root node use CustomNode, while
 * root node use StackNode.
 */
class CustomNode : public ArkUINode {
 protected:
  CustomNodeDelegate* m_customNodeDelegate;
  int32_t m_activeScrollChildId = -1;
  std::weak_ptr<class TaskExecutor> m_taskExecutor;

  /**
   * @brief Called when ArkUI_NodeCustomEventType is
   * ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE.
   */
  void onMeasure();

  /**
   * @brief Trigger component layout logic
   */
  void onLayout();

  /**
   * @brief Get the measured width of the current component (unit: vp)
   *
   * @return int32_t
   */
  int32_t getMeasuredWidth();

  /**
   * @brief Get the measured height of the current component (unit: vp)
   *
   * @return int32_t
   */
  int32_t getMeasuredHeight();

 public:
  explicit CustomNode(const ArkUINode::Context::Shared& context = nullptr);

  ~CustomNode() override;

  CustomNode(const CustomNode& other) = delete;
  CustomNode& operator=(const CustomNode& other) = delete;

  CustomNode(CustomNode&& other) = delete;
  CustomNode& operator=(CustomNode&& other) = delete;

  /**
   * @brief Insert a child node at the specified location.
   *
   * @param child
   * @param index Insert position index
   */
  void insertChild(ArkUINode& child, std::size_t index);

  /**
   * @brief Add child nodes.
   *
   * @param child
   */
  void addChild(ArkUINode& child);

  /**
   * @brief Remove the specified child node.
   *
   * @param child
   */
  void removeChild(ArkUINode& child);

  /**
   * @brief ArkUI event callback distribution interface, supports click and
   * hover events, and triggers onClick/onHoverIn/onHoverOut internally
   * through delegate.
   *
   * @param eventType Event type
   * @param eventArgs Event arguments
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;

  /**
   * @brief Call when actively triggering a click event.
   */
  void onClick();

  /**
   * @brief Set the Custom Node Delegate object.
   *
   * @param customNodeDelegate
   */
  void setCustomNodeDelegate(CustomNodeDelegate* customNodeDelegate);

  /**
   * @brief Set the alignment method of sub components within the container,
   * which controls the orientation and alignment behavior of sub components
   * in the container by setting the ArkUI property NODE_STACK_ALIGN_CONTENT.
   *
   * @param align For specific alignment methods, please refer to the
   * official documentation
   * @return CustomNode&
   */
  CustomNode& setAlign(int32_t align);

  /**
   * @brief Set whether the component can be focused.
   *
   * @param focusable The parameter type is 1 or 0.
   * @return CustomNode&
   */
  CustomNode& setFocusable(bool focusable);

  /**
   * @brief Sets the layout rectangle properties.
   * @param position Position relative to parent.
   * @param size Size including border and padding.
   * @param pointScaleFactor Device pixel density scale factor.
   * @return Reference to this node for method chaining.
   */
  CustomNode& setLayoutRect(
      facebook::react::Point const& position,
      facebook::react::Size const& size,
      facebook::react::Float pointScaleFactor) override;

  /**
   * @brief Manually update measurement dimensions Usually called by layout
   * or measurement processes, it is not recommended to frequently call
   * externally.
   *
   * @param width Unit: VP
   * @param height Unit: VP
   */
  void updateMeasuredSize(int32_t width, int32_t height);

  /**
   * @Deprecated: Use onMeasure(). Deprecated when preparing 0.77 branch for
   * release.
   *
   * @param eventType ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE
   */
  void onMeasure(ArkUI_NodeCustomEventType eventType);

 private:
  static void receiveCustomEvent(ArkUI_NodeCustomEvent* event);

  int32_t m_measuredWidth = 0;
  int32_t m_measuredHeight = 0;
  std::optional<TaskExecutor::DelayedTask> m_delayedMeasureTask;

  void delayMeasure();
};

} // namespace rnoh