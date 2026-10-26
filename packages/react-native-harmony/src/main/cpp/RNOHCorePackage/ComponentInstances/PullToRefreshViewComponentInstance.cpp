/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "PullToRefreshViewComponentInstance.h"
#include <react/renderer/components/FBReactNativeSpec/Props.h>
#include "RNOH/RNInstanceInternal.h"
#include "ScrollViewComponentInstance.h"

using namespace rnoh;

PullToRefreshViewComponentInstance::PullToRefreshViewComponentInstance(
    Context context)
    : CppComponentInstance(std::move(context)),
      m_refreshNode(m_arkUINodeCtx),
      m_refreshIndicatorContainerNode(m_arkUINodeCtx),
      m_refreshIndicatorBackgroundNode(m_arkUINodeCtx),
      m_refreshIndicatorSpinnerNode(m_arkUINodeCtx),
      m_refreshIndicatorTitleNode(m_arkUINodeCtx) {
  m_refreshIndicatorContainerNode.setZIndex(1.0);
  m_refreshIndicatorContainerNode.setVisibility(ARKUI_VISIBILITY_HIDDEN);

  m_refreshIndicatorContainerNode.insertChild(
      m_refreshIndicatorBackgroundNode, 0);
  m_refreshIndicatorBackgroundNode.insertChild(
      m_refreshIndicatorSpinnerNode, 0);
  m_refreshIndicatorBackgroundNode
      .setBorderRadius({500, 500, 500, 500, 500, 500, 500, 500})
      .setBackgroundColor(*facebook::react::clearColor())
      .setSize({REFRESH_INDICATOR_SIZE, REFRESH_INDICATOR_SIZE});

  m_refreshNode.setRefreshNodeDelegate(this);
  m_refreshNode.setRefreshContent(m_refreshIndicatorContainerNode);
}

void PullToRefreshViewComponentInstance::createRefreshIndicatorTitle(
    const std::string& refreshIndicatorTitleText,
    facebook::react::SharedColor const& refreshIndicatorTitleColor,
    bool hasExplicitColor) {
  this->updateRefreshIndicatorTitleNode(
      refreshIndicatorTitleText, refreshIndicatorTitleColor, hasExplicitColor);

  m_refreshIndicatorContainerNode.insertChild(m_refreshIndicatorTitleNode, 1);
}

void PullToRefreshViewComponentInstance::updateRefreshIndicatorTitleNode(
    const std::string& title,
    facebook::react::SharedColor const& color,
    bool hasExplicitColor) {
  if (hasExplicitColor && !facebook::react::isColorMeaningful(color)) {
    m_refreshIndicatorTitleNode.setTextContent(title).setVisibility(
        ARKUI_VISIBILITY_HIDDEN);
    return;
  }
  uint32_t refreshIndicatorTitleColor =
      hasExplicitColor ? *color : *facebook::react::blackColor();
  m_refreshIndicatorTitleNode.setFontColor(refreshIndicatorTitleColor)
      .setTextContent(title);
  m_refreshIndicatorTitleNode.setVisibility(ARKUI_VISIBILITY_VISIBLE);
}

void PullToRefreshViewComponentInstance::updateRefreshPullDownRatio() {
  m_refreshNode.setRefreshPullDownRatio(
      m_isNativeResponderBlocked || !m_isPullToRefreshEnabled ||
              m_isRefreshGestureSuppressed
          ? 0.0
          : 1.0);
}

void PullToRefreshViewComponentInstance::createOrUpdateRefreshIndicatorTitle(
    const std::string& refreshIndicatorTitleText,
    facebook::react::SharedColor const& refreshIndicatorTitleColor,
    bool hasExplicitColor) {
  ArkUI_NodeHandle currRefreshIndicatorTitleNode =
      m_refreshIndicatorContainerNode.getChildAt(1);

  if (!currRefreshIndicatorTitleNode) {
    this->createRefreshIndicatorTitle(
        refreshIndicatorTitleText,
        refreshIndicatorTitleColor,
        hasExplicitColor);
    return;
  }

  this->updateRefreshIndicatorTitleNode(
      refreshIndicatorTitleText, refreshIndicatorTitleColor, hasExplicitColor);
}

void PullToRefreshViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_refreshNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

RefreshNode& PullToRefreshViewComponentInstance::getLocalRootArkUINode() {
  return m_refreshNode;
}

void PullToRefreshViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_refreshNode.removeChild(childComponentInstance->getLocalRootArkUINode());
}

void PullToRefreshViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  CppComponentInstance::onPropsChanged(props);
  const bool hadTintColor =
      m_props != nullptr && m_props->rawProps.count("tintColor") > 0;
  const bool hasTintColor = props->rawProps.count("tintColor") > 0;
  const bool hadTintColorValue =
      hadTintColor && !m_props->rawProps["tintColor"].isNull();
  const bool hasTintColorValue =
      hasTintColor && !props->rawProps["tintColor"].isNull();

  const bool hadTitleColorValue = m_props != nullptr &&
      m_props->rawProps.count("titleColor") > 0 &&
      !m_props->rawProps["titleColor"].isNull();
  const bool hasTitleColorValue = props->rawProps.count("titleColor") > 0 &&
      !props->rawProps["titleColor"].isNull();

  const auto getEnabled = [](SharedConcreteProps const& currentProps) {
    if (currentProps->rawProps.count("enabled") == 0 ||
        !currentProps->rawProps["enabled"].isBool()) {
      return true;
    }
    return currentProps->rawProps["enabled"].asBool();
  };
  const bool wasEnabled = getEnabled(m_props);
  const bool isEnabled = getEnabled(props);
  const bool refreshingChanged = m_props->refreshing != props->refreshing;
  const bool disabledNow = wasEnabled && !isEnabled;

  if (m_props->title != props->title ||
      m_props->titleColor != props->titleColor ||
      hadTitleColorValue != hasTitleColorValue) {
    this->createOrUpdateRefreshIndicatorTitle(
        props->title, props->titleColor, hasTitleColorValue);
  };

  if (props->rawProps.count("progressBackgroundColor") > 0) {
    const auto& progressBgColor = props->rawProps["progressBackgroundColor"];
    if (progressBgColor != nullptr &&
        facebook::react::isColorMeaningful(progressBgColor.asInt())) {
      m_refreshIndicatorBackgroundNode.setBackgroundColor(
          progressBgColor.asInt());
    } else {
      m_refreshIndicatorBackgroundNode.setBackgroundColor(
          facebook::react::clearColor());
    }
  };

  if (props->rawProps.count("size") > 0) {
    const auto& size = props->rawProps["size"];
    if (size != nullptr && size.asString() == "large") {
      m_refreshIndicatorBackgroundNode.setSize(
          {REFRESH_INDICATOR_SIZE * 1.5, REFRESH_INDICATOR_SIZE * 1.5});
    } else {
      m_refreshIndicatorBackgroundNode.setSize(
          {REFRESH_INDICATOR_SIZE, REFRESH_INDICATOR_SIZE});
    }
  }

  m_refreshIndicatorContainerNode.setOffset(0, props->progressViewOffset);
  if (props->refreshing) {
    m_enableOnRefreshNativeEvent = false;
  }
  if (refreshingChanged && !props->refreshing) {
    m_isRefreshGestureSuppressed = false;
    updateRefreshPullDownRatio();
  }
  if (refreshingChanged) {
    m_refreshNode.setNativeRefreshing(props->refreshing);
  } else if (disabledNow && props->refreshing) {
    m_isRefreshGestureSuppressed = true;
    m_refreshNode.setNativeRefreshing(false);
  }

  if (hadTintColor != hasTintColor || hadTintColorValue != hasTintColorValue ||
      (hasTintColorValue && m_props->tintColor != props->tintColor)) {
    if (hasTintColorValue) {
      m_refreshIndicatorSpinnerNode.setColor(props->tintColor);
    } else {
      m_refreshIndicatorSpinnerNode.resetColor();
    }
  }

  if (wasEnabled != isEnabled) {
    m_isPullToRefreshEnabled = isEnabled;
    m_refreshNode.setEnabled(m_isPullToRefreshEnabled);
    updateRefreshPullDownRatio();
  }
}

void PullToRefreshViewComponentInstance::onRefresh() {
  m_refreshIndicatorContainerNode.setVisibility(ARKUI_VISIBILITY_VISIBLE);
  getLocalRootArkUINode().setNativeRefreshing(true);
  if (m_enableOnRefreshNativeEvent) {
    m_refreshNode.setNativeRefreshing(m_props->refreshing);
    m_eventEmitter->onRefresh({});
  }
  m_enableOnRefreshNativeEvent = true;
  auto instance =
      std::static_pointer_cast<RNInstanceInternal>(m_deps->rnInstance.lock());
  if (!instance) {
    return;
  }
  instance->getTaskExecutor()->runTask(
      TaskThread::JS,
      [wptr = this->weak_from_this(), wInstance = instance->weak_from_this()] {
        auto ptr = std::static_pointer_cast<PullToRefreshViewComponentInstance>(
            wptr.lock());
        auto instance =
            std::static_pointer_cast<RNInstanceInternal>(wInstance.lock());
        if (!ptr || !instance || !ptr->m_props || ptr->m_props->refreshing) {
          return;
        }
        instance->getTaskExecutor()->runTask(
            TaskThread::MAIN, [wptr]() mutable {
              auto ptr =
                  std::static_pointer_cast<PullToRefreshViewComponentInstance>(
                      wptr.lock());
              if (ptr && ptr->m_props && !ptr->m_props->refreshing) {
                ptr->getLocalRootArkUINode().setNativeRefreshing(false);
              }
            });
      });
}

void PullToRefreshViewComponentInstance::onRefreshStateChanged(
    RefreshStatus state) {
  m_willIgnoreNextOffsetChange__hack__ = false;
  switch (state) {
    case RefreshStatus::REFRESH_STATUS_DRAG: {
      m_enableOnRefreshNativeEvent = true;
      m_refreshIndicatorContainerNode.setVisibility(ARKUI_VISIBILITY_VISIBLE);
      break;
    }
    case RefreshStatus::REFRESH_STATUS_INACTIVE: {
      m_refreshIndicatorContainerNode.setVisibility(ARKUI_VISIBILITY_HIDDEN);
      break;
    }
    case RefreshStatus::REFRESH_STATUS_DONE:
      m_refreshIndicatorContainerNode.setVisibility(ARKUI_VISIBILITY_HIDDEN);
      break;
    case RefreshStatus::REFRESH_STATUS_REFRESH:
      m_willIgnoreNextOffsetChange__hack__ = true;
      break;
    default:
      break;
  }
  if (auto relatedScrollView = this->getRelatedScrollView()) {
    relatedScrollView->onPullToRefreshStateChanged(state);
  }
}

std::shared_ptr<ScrollViewComponentInstance>
PullToRefreshViewComponentInstance::getRelatedScrollView() {
  if (this->getChildren().empty()) {
    return nullptr;
  }
  const auto& firstChild = this->getChildren()[0];
  if (firstChild == nullptr) {
    return nullptr;
  }
  auto relatedScrollView =
      std::dynamic_pointer_cast<ScrollViewComponentInstance>(firstChild);
  if (relatedScrollView == nullptr) {
    return nullptr;
  }
  return relatedScrollView;
}

void PullToRefreshViewComponentInstance::onRefreshNodeOffsetChange(
    RefreshNode* refreshNode,
    float offset) {
  if (m_willIgnoreNextOffsetChange__hack__) {
    m_willIgnoreNextOffsetChange__hack__ = false;
    return;
  }
  if (auto relatedScrollView = this->getRelatedScrollView()) {
    relatedScrollView->onPullToRefreshOffsetChange(offset);
  }
};

facebook::react::Point PullToRefreshViewComponentInstance::getCurrentOffset()
    const {
  if (!this->getChildren().empty() && this->getChildren()[0] != nullptr) {
    const auto& scrollComponent = this->getChildren()[0];
    auto scrollPosition =
        scrollComponent->getLocalRootArkUINode().getLayoutPosition();
    auto pointScaleFactor = m_layoutMetrics.pointScaleFactor;

    return {
        -scrollPosition.x / pointScaleFactor,
        -scrollPosition.y / pointScaleFactor};
  }

  return {0, 0};
}
void PullToRefreshViewComponentInstance::onNativeResponderBlockChange(
    bool isBlocked) {
  m_isNativeResponderBlocked = isBlocked;
  updateRefreshPullDownRatio();
}

void PullToRefreshViewComponentInstance::onFinalizeUpdates() {
  CppComponentInstance::onFinalizeUpdates();
  if (!this->getChildren().empty() && this->getChildren()[0] != nullptr) {
    auto& child = this->getChildren()[0];
    auto childLayoutMetrics = child->getLayoutMetrics();
    child->getLocalRootArkUINode().setSize(childLayoutMetrics.frame.size);
  }
}
