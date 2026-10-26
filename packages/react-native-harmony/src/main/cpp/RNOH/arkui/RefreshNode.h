/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include "ArkUINode.h"
#include "NativeNodeApi.h"

/**
 * @Deprecated: use rnoh::RefreshNodeDelegate.
 * Deprecated when preparing the 0.77 for release.
 * Drop down refresh node delegation interface, used to listen for refresh
 * events and status changes.
 */
class RefreshNodeDelegate {
 public:
  virtual ~RefreshNodeDelegate() = default;

  /**
   * @brief Refresh event callback, called when the component triggers a
   * refresh event, such as when the user pulls down to reach the trigger
   * threshold.
   */
  virtual void onRefresh(){};

  /**
   * @brief Refresh status enumeration.
   * Search for RefreshStatus on the official website.
   */
  enum class RefreshStatus {
    REFRESH_STATUS_INACTIVE = 0,
    REFRESH_STATUS_DRAG = 1,
    REFRESH_STATUS_OVERDRAG = 2,
    REFRESH_STATUS_REFRESH = 3,
    REFRESH_STATUS_DONE = 4,
  };

  /**
   * @brief Refresh status change callback, called when the refresh status
   * changes.
   *
   * @param state current state
   */
  virtual void onRefreshStateChanged(RefreshStatus state){};
};

/**
 * @actor RNOH_LIBRARY
 */
namespace rnoh {
class RefreshNode;

/**
 * @brief Drop down refresh node delegation interface, used to listen for
 * refresh events and status changes.
 */
class RefreshNodeDelegate {
 public:
  virtual ~RefreshNodeDelegate() = default;

  /**
   * @brief Refresh event callback, called when the component triggers a
   * refresh event, such as when the user pulls down to reach the trigger
   * threshold.
   */
  virtual void onRefresh(){};

  /**
   * @brief Refresh status enumeration.
   * Search for RefreshStatus on the official website.
   */
  enum class RefreshStatus {
    REFRESH_STATUS_INACTIVE = 0,
    REFRESH_STATUS_DRAG = 1,
    REFRESH_STATUS_OVERDRAG = 2,
    REFRESH_STATUS_REFRESH = 3,
    REFRESH_STATUS_DONE = 4,
  };

  /**
   * @brief Refresh status change callback, called when the refresh status
   * changes.
   *
   * @param state current state
   */
  virtual void onRefreshStateChanged(RefreshStatus state){};

  virtual void onRefreshNodeOffsetChange(
      RefreshNode* refreshNode,
      float offset) {}
};

/**
 * @brief Encapsulated the property settings, event registration, and child
 * node management logic of the ArkUI dropdown refresh node.
 */
class RefreshNode : public ArkUINode {
 protected:
  RefreshNodeDelegate* m_refreshNodeDelegate;
  static constexpr float REFRESH_NODE_SIZE = 29;

 public:
  explicit RefreshNode(const ArkUINode::Context::Shared& context = nullptr);

  ~RefreshNode();

  /**
   * @brief Insert child node
   *
   * @param child
   * @param index Insert position index
   */
  void insertChild(ArkUINode& child, std::size_t index);

  /**
   * @brief Remove child nodes
   *
   * @param child the child node
   */
  void removeChild(ArkUINode& child);

  /**
   * @brief Handle node events from ArkUI, including refresh events, status
   * changes, etc., and forward them to upper level callbacks for processing
   * through delegation.
   *
   * @param eventType Event type
   * @param eventArgs Event arguments
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs);

  /**
   * @brief Set whether it is in refresh mode, and the business logic can
   * manually set the start/end of refresh.ArkUI property setting
   * instructions: 1 indicates refresh in progress, 0 indicates non refresh.
   *
   * @param isRefreshing True starts refreshing, false stops refreshing
   * @return RefreshNode& Current RefreshNode Reference
   */
  RefreshNode& setNativeRefreshing(bool isRefreshing);

  /**
   * @brief Set up event delegation instances to provide callback handling
   * for refresh events and state changes.
   *
   * @param refreshNodeDelegate Event delegation instance
   * @return RefreshNode& Current RefreshNode Reference
   */
  RefreshNode& setRefreshNodeDelegate(RefreshNodeDelegate* refreshNodeDelegate);

  /**
   * @brief Set custom display content for the dropdown refresh area, and set
   * content nodes for the dropdown refresh area, such as loading animations
   * or text prompts.
   *
   * @param refreshContent Custom Content Node
   * @return RefreshNode& Current RefreshNode Reference
   */
  RefreshNode& setRefreshContent(ArkUINode& refreshContent);

  /**
   * @brief Set the follow-up coefficient for the pull-down progress to
   * control the ratio of user gesture dragging to visual response. The
   * smaller the value, the more sensitive the response. ArkUI property
   * setting instructions: valid range [0.0~1.0].
   *
   * @param pullDownRatio traction coefficient
   * @return RefreshNode& Current RefreshNode Reference
   */
  RefreshNode& setRefreshPullDownRatio(float pullDownRatio);

  /**
   * @brief Set the follow-up coefficient for the pull-down progress to
   * control the ratio of user gesture dragging to visual response. The
   * smaller the value, the more sensitive the response. ArkUI property
   * setting instructions: valid range [0.0~1.0].
   * @deprecated: use setRefreshPullDownRatio. Deprecated when preparing the
   * 0.77 branch for release.
   *
   * @param pullDownRatio traction coefficient
   * @return RefreshNode& Current RefreshNode Reference
   */
  RefreshNode& setRefreshPullDownRation(float pullDownRatio) {
    return setRefreshPullDownRatio(pullDownRatio);
  };
};
} // namespace rnoh