/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <react/renderer/graphics/Point.h>
#include "ArkUINode.h"
#include "NativeNodeApi.h"

namespace rnoh {
/**
 * @actor RNOH_LIBRARY
 *
 * @brief ScrollNodeDelegate class for handling scroll events. Provides
 * callback interfaces for scroll start, stop, and frame begin.
 */
class ScrollNodeDelegate {
 public:
  /**
   * @brief Destructor for destroying the ScrollNodeDelegate object.
   */
  virtual ~ScrollNodeDelegate() = default;

  /**
   * @brief Virtual function for implementing scrolling events.
   * Call when triggering scrolling events.
   */
  virtual void onScroll(){};

  /**
   * @brief Virtual function for starting scrolling events.
   * Call when starting scrolling events.
   */
  virtual void onScrollStart(){};

  /**
   * @brief Virtual function for stopping scrolling events.
   * Call when stopping scrolling events.
   */
  virtual void onScrollStop(){};

  /**
   * @brief Callback function for scroll frame begin events.
   * The conditions that trigger this event are:
   * 1. When the scrolling component triggers scrolling, it triggers other
   * input settings that trigger scrolling, including keyboard and mouse
   * operations.
   * 2. Not triggered when calling the controller interface.
   * 3. Out of bounds rebound does not trigger.
   *
   * @param offset The current scroll offset, Unit is VP
   * @param scrollState The current scroll state. Search on the official website
   * for NODE_SCROLL_EVENT_ON_SCROLL_FRAME_BEGIN.
   * @return float The new scroll offset.
   */
  virtual float onScrollFrameBegin(float offset, int32_t scrollState) {
    return offset;
  };

  /**
   * @brief This callback is triggered when the component is mounted and
   * displayed.
   */
  virtual void onAppear(){};
};

/**
 * @actor RNOH_LIBRARY
 *
 * @brief ScrollNode class provides more convenient to use interface to be used
 * from ComponentInstances.
 */
class ScrollNode : public ArkUINode {
 protected:
  using Point = facebook::react::Point;

  ArkUI_NodeHandle m_childArkUINodeHandle;
  ScrollNodeDelegate* m_scrollNodeDelegate;

 public:
  explicit ScrollNode(const ArkUINode::Context::Shared& context = nullptr);

  ~ScrollNode() override;

  /**
   * @brief Inserts a child node into the scroll node.
   *
   * @param child The child node to be inserted.
   */
  void insertChild(ArkUINode& child);

  /**
   * @brief Removes a child node from the scroll node.
   *
   * @param child The child node to be removed.
   */
  void removeChild(ArkUINode& child);

  /**
   * @brief Handle node events from ArkUI.
   *
   * @param eventType The type of event.
   * @param eventArgs The event arguments.
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;

  /**
   * @brief Gets the current scroll offset.
   *
   * @return Pointer to the scroll offset point.
   */
  Point getScrollOffset() const;

  /**
   * @brief Sets the scrollNodeDelegate.
   *
   * @param scrollNodeDelegate Pointer to the scrollNodeDelegate.
   */
  void setScrollNodeDelegate(ScrollNodeDelegate* scrollNodeDelegate);

  /**
   * @brief Sets whether the scroll node is horizontal.
   *
   * @param horizontal Whether the scroll node is horizontal.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setHorizontal(bool horizontal);

  /**
   * @brief Set whether to support scrolling gestures. When set to false,
   * scrolling with fingers or mouse is not possible, but it does not affect
   * the scrolling interface of the controller.
   *
   * @param enableScrollInteraction Whether scroll interaction is enabled.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setEnableScrollInteraction(bool enableScrollInteraction);

  /**
   * @brief Setting the friction coefficient takes effect when manually
   * swiping the rolling area, only affecting the inertial rolling process
   * and indirectly affecting the chain effect during the inertial rolling
   * process.
   *
   * @param friction The friction of the scroll node.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setFriction(float friction);

  /**
   * @brief Sets the fling speed limit for the scroll node.
   *
   * @param flingSpeedLimit The maximum speed of fling. Unit is VP/s.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setFlingSpeedLimit(float flingSpeedLimit);

  /**
   * @brief Set edge sliding effect.
   *
   * @param bounces Whether the scroll node bounces.
   * @param alwaysBounces Whether the scroll node always bounces.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setEdgeEffect(bool bounces, bool alwaysBounces);

  /**
   * @Deprecated: this method is no longer part of the native API. Use
   * setEdgeEffect. Deprecated when preparing the 0.77 branch for release.
   *
   * @param overScrollMode Contains three values, never、auto、always
   */
  void setScrollOverScrollMode(std::string const& overScrollMode);

  /**
   * @brief Set scrollbar status. Do not display, display on demand, display
   * continuously
   *
   * @param scrollBarDisplayMode Scroll bar display mode.Search the official
   * website for NODE_SCROLL_BAR_DISPLAY_MODE.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setScrollBarDisplayMode(
      ArkUI_ScrollBarDisplayMode scrollBarDisplayMode);

  /**
   * @brief Set the color of the scrollbar.
   *
   * @param scrollBarColor Scroll bar color as an unsigned 32-bit integer.
   * The color is in the 0xargb type.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setScrollBarColor(uint32_t scrollBarColor);

  /**
   * @brief Sets the scroll snapping behavior of the Scroll component
   *
   * @param scrollSnapAlign Alignment for scroll snap.
   * @param snapToStart Boolean indicating if snapping to the start is enabled.
   * @param snapToEnd Boolean indicating if snapping to the end is enabled.
   * @param snapPoints Vector of snap points.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setScrollSnap(
      ArkUI_ScrollSnapAlign scrollSnapAlign,
      bool snapToStart,
      bool snapToEnd,
      const std::vector<facebook::react::Float>& snapPoints);

  /**
   * @brief Set whether to support sliding page flipping.
   *
   * @param enablePaging Boolean indicating if paging is enabled.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setEnablePaging(bool enablePaging);

  /**
   * @brief Set the alignment of sub components within the container.
   *
   * @param centerContent Boolean indicating if content should be centered.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setCenterContent(bool centerContent);

  /**
   * @brief Enable or disable the back-to-top feature for the scroll node.
   *
   * Maps to NODE_SCROLL_BACK_TO_TOP (available since API15). On lower API
   * levels it is silently skipped, keeping the system default behavior.
   *
   * @param backToTop Whether to enable back-to-top.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setBackToTop(bool backToTop);

  /**
   * @brief Set the fading edge effect length for the scroll node.
   *
   * @param length The length of the fading edge effect. Unit is VP.
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& setFadingEdge(float length);

  /**
   * @brief Checks if the scroll node is horizontal.
   *
   * @return Boolean indicating if the scroll node is horizontal.
   */
  bool isHorizontal() const;

  /**
   * @brief Reset the limit scrolling mode of the Scroll component
   *
   * @return ScrollNode& Reference to the ScrollNode object.
   */
  ScrollNode& resetScrollSnap();

  /**
   * @brief Scroll to the specified x, y offset. The third parameter is
   * whether to enable smooth scrolling animation.
   *
   * @param x The x-coordinate of the position to scroll to. Unit is VP
   * @param y The y-coordinate of the position to scroll to. Unit is VP
   * @param animated Boolean indicating if the scroll should be animated.
   * @param scrollToOverflowEnabled Optional value, sets whether scrolling
   * can cross the boundary.
   */
  void scrollTo(
      float x,
      float y,
      bool animated,
      bool scrollToOverflowEnabled = false);

  /**
   * @brief Scroll nested scrolling options
   *
   * @param scrollNestedMode Nested scroll mode. Search the official website for
   * NODE_SCROLL_NESTED_SCROLL.
   */
  void setNestedScroll(ArkUI_ScrollNestedMode scrollNestedMode);
  void setNestedScroll(
      ArkUI_ScrollNestedMode scrollForward,
      ArkUI_ScrollNestedMode scrollBackward);

  /**
   * @brief Set whether to support scrolling gestures. When set to false,
   * scrolling with fingers or mouse is not possible, but it does not affect
   * the scrolling interface of the controller
   * @Deprecated: this method is no longer part of the native API. Deprecated
   * when preparing the 0.77 branch for release.
   *
   * @param nestedScrollEnabled Boolean indicating if nested scrolling is
   * enabled.
   */
  void setNestedScrollEnabled(bool nestedScrollEnabled);

  /**
   * @brief Sets the BackgroundColor.
   * @Deprecated: this method is no longer part of the native API. Use
   * ArkUINode::setBackgroundColor. Deprecated when preparing the 0.77 branch
   * for release.
   *
   * @param color Background color attribute, 0xargb format
   */
  void setEndFillColor(uint32_t color);
};

} // namespace rnoh