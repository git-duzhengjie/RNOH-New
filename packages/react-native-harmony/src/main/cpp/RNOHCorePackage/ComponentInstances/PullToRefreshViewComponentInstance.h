/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <react/renderer/components/FBReactNativeSpec/EventEmitters.h>
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/ColumnNode.h"
#include "RNOH/arkui/LoadingProgressNode.h"
#include "RNOH/arkui/RefreshNode.h"
#include "RNOH/arkui/StackNode.h"
#include "RNOH/arkui/TextNode.h"
#include "react/renderer/components/FBReactNativeSpec/ShadowNodes.h"

namespace rnoh {

class ScrollViewComponentInstance;

class PullToRefreshViewComponentInstance
    : public CppComponentInstance<facebook::react::PullToRefreshViewShadowNode>,
      public RefreshNodeDelegate {
 private:
  static constexpr float REFRESH_INDICATOR_SIZE = 29;

  RefreshNode m_refreshNode;
  bool m_enableOnRefreshNativeEvent{true};
  bool m_isNativeResponderBlocked{false};
  bool m_isPullToRefreshEnabled{true};
  bool m_isRefreshGestureSuppressed{false};
  /**
   * In a certain situation, a received offset is incorrect, which messes up
   * the logic in ScrollViewComponentInstance responsible for recognizing some
   * event when offset == 0.
   * This property is used to ignore that one spurious offset.
   */
  bool m_willIgnoreNextOffsetChange__hack__{false};

  ColumnNode m_refreshIndicatorContainerNode;
  StackNode m_refreshIndicatorBackgroundNode;
  LoadingProgressNode m_refreshIndicatorSpinnerNode;
  TextNode m_refreshIndicatorTitleNode;

  void createRefreshIndicatorTitle(
      const std::string& refreshIndicatorTitleText,
      facebook::react::SharedColor const& refreshIndicatorTitleColor,
      bool hasExplicitColor);
  void createOrUpdateRefreshIndicatorTitle(
      const std::string& refreshIndicatorTitleText,
      facebook::react::SharedColor const& refreshIndicatorTitleColor,
      bool hasExplicitColor);
  void updateRefreshIndicatorTitleNode(
      const std::string& title,
      facebook::react::SharedColor const& color,
      bool hasExplicitColor);
  void updateRefreshPullDownRatio();
  std::shared_ptr<ScrollViewComponentInstance> getRelatedScrollView();

 protected:
  void onNativeResponderBlockChange(bool isBlocked) override;
  void onFinalizeUpdates() override;

 public:
  PullToRefreshViewComponentInstance(Context context);

  RefreshNode& getLocalRootArkUINode() override;

  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;
  void onPropsChanged(SharedConcreteProps const& props) override;
  void onRefresh() override;
  void onRefreshStateChanged(RefreshStatus state) override;
  void onRefreshNodeOffsetChange(RefreshNode* refreshNode, float offset)
      override;
  facebook::react::Point getCurrentOffset() const override;

  bool isRefreshControlComponentInstance() const override {
    return true;
  }
};
} // namespace rnoh
