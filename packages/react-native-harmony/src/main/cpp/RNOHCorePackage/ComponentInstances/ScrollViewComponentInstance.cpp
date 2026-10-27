/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ScrollViewComponentInstance.h"
#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>
#include <react/renderer/components/scrollview/ScrollViewState.h>
#include <react/renderer/core/ConcreteState.h>
#include <cmath>
#include <optional>
#include "PullToRefreshViewComponentInstance.h"
#include "RNOH/arkui/UIInputEventHandler.h"
#include "ScrollViewHelpers/LeastSquareSolver.h"
#include "ViewComponentInstance.h"
#include "conversions.h"

namespace rnoh {

class ScrollViewTouchHandler : public UIInputEventHandler {
 private:
  ScrollViewComponentInstance* m_scrollViewComponentInstance;

 public:
  ScrollViewTouchHandler(ScrollViewTouchHandler const& other) = delete;
  ScrollViewTouchHandler& operator=(ScrollViewTouchHandler const& other) =
      delete;
  ScrollViewTouchHandler(ScrollViewTouchHandler&& other) = delete;
  ScrollViewTouchHandler& operator=(ScrollViewTouchHandler&& other) = delete;

  ScrollViewTouchHandler(ScrollViewComponentInstance* rootView)
      : UIInputEventHandler(rootView->getLocalRootArkUINode()),
        m_scrollViewComponentInstance(rootView) {}

  void onTouchEvent(ArkUI_UIInputEvent* event) override {
    auto action = OH_ArkUI_UIInputEvent_GetAction(event);

    if (action == UI_TOUCH_EVENT_ACTION_UP) {
      m_scrollViewComponentInstance->onTouchEventActionUp();
    }
  }
};

ScrollViewInternalState::ScrollViewInternalState(
    ScrollViewComponentInstance* instance)
    : m_instance(instance) {}

void IdleScrollViewInternalState::onScrollStart() {
  m_instance->onChangeInternalState(
      std::make_unique<SettlingScrollViewInternalState>(m_instance));
};

void IdleScrollViewInternalState::onDragStart() {
  m_instance->onEmitOnScrollBeginDragEvent();
  m_instance->onChangeInternalState(
      std::make_unique<DraggingScrollViewInternalState>(m_instance));
};

void IdleScrollViewInternalState::onScroll() {
  m_instance->onEmitOnScrollEvent();
};

void DraggingScrollViewInternalState::onDragStop() {
  m_instance->onDraggingScrollDragStop();
};

void ScrollViewComponentInstance::onDraggingScrollDragStop() {
  onEmitOnScrollEndDragEvent();
  auto state = std::make_unique<SettlingScrollViewInternalState>(this);
  onChangeInternalState(std::move(state));
};

void DraggingScrollViewInternalState::onScroll() {
  m_instance->onEmitOnScrollEvent();
};

void DraggingScrollViewInternalState::onScrollStart() {
  m_instance->onEmitOnScrollEndDragEvent();
  m_instance->onChangeInternalState(
      std::make_unique<SettlingScrollViewInternalState>(m_instance));
};

void DraggingScrollViewInternalState::onScrollStop() {
  m_instance->onDraggingScrollStop();
};

void ScrollViewComponentInstance::onDraggingScrollStop() {
  onEmitOnScrollEndDragEvent();
  auto state = std::make_unique<IdleScrollViewInternalState>(this);
  onChangeInternalState(std::move(state));
};

void SettlingScrollViewInternalState::onDragStart() {
  m_instance->onEmitOnScrollBeginDragEvent();
  m_instance->onChangeInternalState(
      std::make_unique<DraggingScrollViewInternalState>(m_instance));
};

void SettlingScrollViewInternalState::onDragStop() {
  // noop — This method can be triggered by a hack responsible for detecting
  // onDragStop.
  return;
};

void SettlingScrollViewInternalState::onScroll() {
  if (!m_hasOnScrollBeenCalled) {
    m_instance->onEmitMomentumScrollBegin();
    m_hasOnScrollBeenCalled = true;
  }
  m_instance->onEmitOnScrollEvent();
};

void SettlingScrollViewInternalState::onScrollStop() {
  m_instance->onEmitOnMomentumScrollEndEvent();
  m_instance->onChangeInternalState(
      std::make_unique<IdleScrollViewInternalState>(m_instance));
};

void CancelingScrollViewInternalState::onScrollStop() {
  m_instance->resetScrollInteraction();
  // When scrolling is interrupted (scrollEnabled toggled off, or parent
  // steals the gesture), onScrollFrameBegin sets state to CANCELING before
  // momentum begin/end transitions happen. Without re-emitting these two
  // end events here, JS ScrollView._isAnimating() never resets (no
  // momentumScrollEnd), onStartShouldSetResponderCapture keeps returning
  // true, and child touches (e.g. Button onPress) are swallowed.
  m_instance->onEmitOnMomentumScrollEndEvent();
  m_instance->onEmitOnScrollEndDragEvent();
  m_instance->onChangeInternalState(
      std::make_unique<IdleScrollViewInternalState>(m_instance));
};

// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// ScrollViewInternalStateDelegate

void ScrollViewComponentInstance::onChangeInternalState(
    std::unique_ptr<ScrollViewInternalState> internalState) {
  VLOG(2) << "ScrollViewComponentInstance(" << m_tag
          << ")::onChangeInternalState (" << m_internalState->getDebugName()
          << " => " << internalState->getDebugName() << ")";
  m_internalState = std::move(internalState);
}

void ScrollViewComponentInstance::onEmitMomentumScrollBegin() {
  auto scrollViewMetrics = getScrollViewMetrics();
  emitScrollEvent("momentumScrollBegin", scrollViewMetrics);
}

void ScrollViewComponentInstance::onEmitOnMomentumScrollEndEvent() {
  auto scrollViewMetrics = getScrollViewMetrics();
  emitScrollEvent("momentumScrollEnd", scrollViewMetrics);
  updateStateWithContentOffset(scrollViewMetrics.contentOffset);
}

void ScrollViewComponentInstance::onEmitOnScrollBeginDragEvent() {
  m_velocityTracker.reset();
  auto scrollViewMetrics = getScrollViewMetrics();
  emitScrollEvent("scrollBeginDrag", scrollViewMetrics);
}

void ScrollViewComponentInstance::onEmitOnScrollEvent() {
  auto scrollViewMetrics = getScrollViewMetrics();
  if (!m_isScrollNestedModeManagedByLibrary &&
      !isContentSmallerThanContainer() && m_allowScrollPropagation &&
      !isAtEnd(scrollViewMetrics.contentOffset)) {
    m_scrollNode.setNestedScroll(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
    m_allowScrollPropagation = false;
  }
  auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::steady_clock::now().time_since_epoch())
                 .count();
  m_movedBySignificantOffset =
      scrollMovedBySignificantOffset(scrollViewMetrics.contentOffset);
  if (m_allowNextScrollEvent ||
      isCloseToTargetOffset(scrollViewMetrics.contentOffset) ||
      ((m_scrollEventThrottle == 0 ||
        m_scrollEventThrottle < now - m_lastScrollDispatchTime) &&
       m_movedBySignificantOffset)) {
    m_lastScrollDispatchTime = now;
    VLOG(2) << "onScroll (contentOffset: " << scrollViewMetrics.contentOffset.x
            << ", " << scrollViewMetrics.contentOffset.y
            << "; contentSize: " << scrollViewMetrics.contentSize.width << ", "
            << scrollViewMetrics.contentSize.height
            << "; containerSize: " << scrollViewMetrics.containerSize.width
            << ", " << scrollViewMetrics.containerSize.height << ")";
    emitScrollEvent("scroll", scrollViewMetrics);
    // Similar millisecond checks are used on Android implementation
    static const int singleTimeframe = 17;
    if (now - m_lastStateUpdateTimeMs >= 3 * singleTimeframe) {
      updateStateWithContentOffset(scrollViewMetrics.contentOffset);
      m_lastStateUpdateTimeMs = now;
    }
    m_currentOffset = scrollViewMetrics.contentOffset;
    m_currentOffset.x = adjustOffsetIfRTL(m_currentOffset.x);
    updateContentClippedSubviews();
  }
  sendEventForNativeAnimations(scrollViewMetrics);
}

void ScrollViewComponentInstance::onEmitOnScrollEndDragEvent() {
  if (m_disableIntervalMomentum) {
    disableIntervalMomentum();
  }
  auto scrollViewMetrics = getScrollViewMetrics();
  emitScrollEvent("scrollEndDrag", scrollViewMetrics);
  updateStateWithContentOffset(scrollViewMetrics.contentOffset);
}

// ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  Called from PullToRefresh

void ScrollViewComponentInstance::onPullToRefreshOffsetChange(float offsetY) {
  m_onPullToRefreshOffsetY = offsetY;
  if (offsetY == 0) {
    m_internalState->onScrollStop();
  }
  m_internalState->onScroll();
}

void ScrollViewComponentInstance::onPullToRefreshStateChanged(
    RefreshNodeDelegate::RefreshStatus state) {
  using RefreshState = RefreshNodeDelegate::RefreshStatus;
  switch (state) {
    case RefreshState::REFRESH_STATUS_INACTIVE: {
      if (m_onPullToRefreshOffsetY.value_or(0) > 0) {
        m_internalState->onScrollStart();
      }
      break;
    }
    case RefreshState::REFRESH_STATUS_DRAG: {
      m_internalState->onDragStart();
      break;
    }
    case RefreshState::REFRESH_STATUS_REFRESH: {
      m_internalState->onScrollStop();
      break;
    }
    case RefreshState::REFRESH_STATUS_DONE: {
      m_internalState->onScrollStart();
      break;
    }
  }
}

// ——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//  ScrollNodeDelegate

void ScrollViewComponentInstance::onScrollStart() {
  m_internalState->onScrollStart();
  m_allowNextScrollEvent = false;
}

void ScrollViewComponentInstance::onScroll() {
  if (m_onScrollCallsAfterFrameBeginCallCounter == 1 &&
      wasInDraggingStateAtTouchUp) {
    m_internalState->onDragStop();
    wasInDraggingStateAtTouchUp = false;
  }
  m_internalState->onScroll();
  m_onScrollCallsAfterFrameBeginCallCounter++;
}

bool ScrollViewComponentInstance::shouldDisableScrollInteraction() {
  auto ancestorTouchTarget = getTouchTargetParent();
  while (ancestorTouchTarget) {
    if (ancestorTouchTarget->isJSResponder()) {
      return true;
    }
    ancestorTouchTarget = ancestorTouchTarget->getTouchTargetParent();
  }
  return false;
}

float ScrollViewComponentInstance::onScrollFrameBegin(
    float offset,
    int32_t scrollNodeState) {
  m_onScrollCallsAfterFrameBeginCallCounter = 0;
  if ((!m_props->scrollEnabled) || shouldDisableScrollInteraction()) {
    m_recentScrollFrameOffset = 0;
    m_internalState = std::make_unique<CancelingScrollViewInternalState>(this);
    m_scrollNode.setEnableScrollInteraction(false);
    return 0;
  }
  m_recentScrollFrameOffset = offset;
  auto newScrollNodeState = static_cast<ScrollNodeState>(scrollNodeState);
  if (m_internalState->asScrollNodeState() != newScrollNodeState) {
    if (newScrollNodeState == ScrollNodeState::DRAGGING) {
      wasInDraggingStateAtTouchUp = false;
      m_internalState->onDragStart();
    } else if (
        m_internalState->asScrollNodeState() == ScrollNodeState::DRAGGING) {
      m_internalState->onDragStop();
    }
  }
  // When disableIntervalMomentum is used and the finger
  // lifts off the screen, we should set the
  // offset in onScrollFrameBegin to 0. otherwise, in
  // nested-scroll scenarios, conflicting scroll events will
  // cause disableIntervalMomentum to be ignored.
  if (!m_snapToOffsets.empty() && m_disableIntervalMomentum &&
      newScrollNodeState == ScrollNodeState::SETTLING) {
    return 0;
  }
  return offset;
}

void ScrollViewComponentInstance::onScrollStop() {
  m_internalState->onScrollStop();
  m_allowNextScrollEvent = true;

  if (!m_isScrollNestedModeManagedByLibrary &&
      !isContentSmallerThanContainer() && !m_allowScrollPropagation &&
      isAtEnd(m_currentOffset)) {
    m_scrollNode.setNestedScroll(ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
    m_allowScrollPropagation = true;
  }
  updateContentClippedSubviews();
}

void ScrollViewComponentInstance::onTouchEventActionUp() {
  if (m_internalState->asScrollNodeState() != ScrollNodeState::DRAGGING) {
    return;
  }
  wasInDraggingStateAtTouchUp = true;
}

// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
// CppComponentInstance overrides

ScrollViewComponentInstance::ScrollViewComponentInstance(Context context)
    : CppComponentInstance(std::move(context)),
      m_touchHandler(std::make_unique<ScrollViewTouchHandler>(this)),
      m_scrollNode(m_arkUINodeCtx),
      m_contentContainerNode(m_arkUINodeCtx) {
  m_internalState = std::make_unique<IdleScrollViewInternalState>(this);
  m_scrollNode.insertChild(m_contentContainerNode);
  // NOTE: perhaps this needs to take rtl into account?
  m_scrollNode.setAlignment(ARKUI_ALIGNMENT_TOP_START);
  m_scrollNode.setScrollNodeDelegate(this);
  m_scrollNode.setNestedScroll(ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
}

ScrollNode& ScrollViewComponentInstance::getLocalRootArkUINode() {
  return m_scrollNode;
}

void ScrollViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);

  m_contentContainerNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void ScrollViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);

  m_contentContainerNode.removeChild(
      childComponentInstance->getLocalRootArkUINode());
}

void ScrollViewComponentInstance::onLayoutChanged(
    facebook::react::LayoutMetrics const& layoutMetrics) {
  // Not calling CppComponentInstance::onLayoutChanged due to bug in
  // setLayoutRect()
  if (layoutMetrics.pointScaleFactor != m_layoutMetrics.pointScaleFactor) {
    this->getLocalRootArkUINode().setTransform(
        getTransform(), layoutMetrics.pointScaleFactor);
    if (m_props) {
      auto props =
          std::static_pointer_cast<const facebook::react::ViewProps>(m_props);
      this->getLocalRootArkUINode().setShadow(
          props->shadowColor,
          props->shadowOffset,
          props->shadowOpacity,
          props->shadowRadius,
          layoutMetrics.pointScaleFactor);
    }
  }
  if (layoutMetrics.layoutDirection != m_layoutMetrics.layoutDirection) {
    ArkUI_Direction direction =
        convertLayoutDirection(layoutMetrics.layoutDirection);
    this->getLocalRootArkUINode().setDirection(direction);
  }
  // Re-resolve linear gradient stop positions when the frame size changes.
  // The base CppComponentInstance::onLayoutChanged is not called (see above),
  // so the px-stop re-resolution added there does not run for ScrollView.
  if (m_props && layoutMetrics.frame.size != m_layoutMetrics.frame.size) {
    auto props =
        std::static_pointer_cast<const facebook::react::ViewProps>(m_props);
    if (!props->backgroundImage.empty()) {
      this->getLocalRootArkUINode().setBackgroundImage(
          props->backgroundImage,
          layoutMetrics.frame.size.width,
          layoutMetrics.frame.size.height);
    }
  }
  markBoundingBoxAsDirty();

  if (m_containerSize != layoutMetrics.frame.size) {
    m_containerSize = layoutMetrics.frame.size;
    m_containerSizeChanged = true;
  }
  if (m_layoutMetrics.layoutDirection != layoutMetrics.layoutDirection) {
    m_scrollNode.setDirection(
        convertLayoutDirection(layoutMetrics.layoutDirection));
  }
}

void rnoh::ScrollViewComponentInstance::updateOffsetAfterChildChange(
    facebook::react::Point offset) {
  if (m_internalState->asScrollNodeState() != ScrollNodeState::IDLE) {
    return;
  }

  facebook::react::Point targetOffset = {offset.x, offset.y};
  if (isHorizontal(m_props)) {
    if (targetOffset.x > m_contentSize.width - m_containerSize.width) {
      targetOffset.x = m_contentSize.width - m_containerSize.width;
    }
    if (targetOffset.x < 0) {
      targetOffset.x = 0;
    }
  } else {
    if (targetOffset.y > m_contentSize.height - m_containerSize.height) {
      targetOffset.y = m_contentSize.height - m_containerSize.height;
    }
    if (targetOffset.y < 0) {
      targetOffset.y = 0;
    }
  }

  if (offset == targetOffset) {
    return;
  }

  m_scrollNode.scrollTo(
      targetOffset.x, targetOffset.y, false, m_scrollToOverflowEnabled);
  updateContentClippedSubviews();
}

void rnoh::ScrollViewComponentInstance::onStateChanged(
    SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);
  const auto& stateData = state->getData();
  auto contentSizeChanged = m_contentSize != stateData.getContentSize();
  m_contentSizeChanged = m_contentSizeChanged || contentSizeChanged;
  if (contentSizeChanged) {
    m_contentContainerNode.setSize(stateData.getContentSize());
    m_contentSize = stateData.getContentSize();

    // Re-apply edge effect when content size changes to ensure correct bounce
    // behavior when alwaysBounce* props are false (only bounce when content >
    // container)
    if (m_props && !m_rawProps.overScrollMode.has_value()) {
      m_scrollNode.setEdgeEffect(
          m_props->bounces,
          isHorizontal(m_props) ? m_props->alwaysBounceHorizontal
                                : m_props->alwaysBounceVertical);
    }
  }
}

void rnoh::ScrollViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  CppComponentInstance::onPropsChanged(props);

  // Handle focusable and accessibility mode for ScrollView
  if (!m_props || props->accessible != m_props->accessible) {
    m_scrollNode.setAccessibilityMode(
        props->accessible
            ? ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_ENABLED
            : ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_AUTO);
  }

  /**
   * This block is needed to detects which batch of mutations were triggered by
   * appearance of the keyboard.
   * "__keyboardAvoidingViewBottomHeight" is injected by KeyboardAvoidingView.
   * ScrollView needs to be placed directly inside KeyboardAvoidingView.
   */
  double parentKeyboardAvoidingViewBottomHeight = 0;
  if (props->rawProps.count("__keyboardAvoidingViewBottomHeight") > 0) {
    parentKeyboardAvoidingViewBottomHeight =
        props->rawProps["__keyboardAvoidingViewBottomHeight"].asDouble();
  }
  double prevParentKeyboardAvoidingViewBottomHeight = 0;
  if (m_props != nullptr &&
      m_props->rawProps.count("__keyboardAvoidingViewBottomHeight") > 0 &&
      m_props->rawProps["__keyboardAvoidingViewBottomHeight"] > 0) {
    prevParentKeyboardAvoidingViewBottomHeight =
        m_props->rawProps["__keyboardAvoidingViewBottomHeight"].asDouble();
  }
  auto keyboardAvoider = m_keyboardAvoider.lock();
  if (keyboardAvoider && parentKeyboardAvoidingViewBottomHeight > 0 &&
      parentKeyboardAvoidingViewBottomHeight !=
          prevParentKeyboardAvoidingViewBottomHeight) {
    m_shouldAdjustScrollPositionOnNextRender = true;
  }

  auto rawProps = ScrollViewRawProps::getFromDynamic(props->rawProps);

  m_persistentScrollbar = props->persistentScrollbar;
  m_scrollEventThrottle = props->scrollEventThrottle;
  m_disableIntervalMomentum = props->disableIntervalMomentum;
  m_scrollToOverflowEnabled = props->scrollToOverflowEnabled;
  auto wasHorizontal = m_scrollNode.isHorizontal();
  m_scrollNode.setHorizontal(isHorizontal(props))
      .setFriction(
          getFrictionFromDecelerationRate(rawProps.decelerationRate))
      .setScrollBarDisplayMode(getScrollBarDisplayMode(
          isHorizontal(props),
          m_persistentScrollbar,
          props->showsVerticalScrollIndicator,
          props->showsHorizontalScrollIndicator))
      .setScrollBarColor(
          props->indicatorStyle ==
                  facebook::react::ScrollViewIndicatorStyle::White
              ? 0x66FFFFFF
              : 0x66000000)
      .setEnablePaging(props->pagingEnabled);

  if (props->rawProps.count("scrollsToTop") > 0 &&
      (!m_appliedBackToTop.has_value() ||
       m_appliedBackToTop.value() != props->scrollsToTop)) {
    m_scrollNode.setBackToTop(props->scrollsToTop);
    m_appliedBackToTop = props->scrollsToTop;
  }

  // Prioritize overScrollMode over bounces and alwaysBounce* props
  if (rawProps.overScrollMode.has_value()) {
    if (m_rawProps.overScrollMode != rawProps.overScrollMode) {
      m_rawProps.overScrollMode = rawProps.overScrollMode;
      bool alwaysBounce = false;
      bool bounces = false;
      if (rawProps.overScrollMode.value() == "auto") {
        bounces = true;
      } else if (rawProps.overScrollMode.value() == "always") {
        bounces = true;
        alwaysBounce = true;
      }
      m_scrollNode.setEdgeEffect(bounces, alwaysBounce);
    }
  } else {
    if (!m_props || props->bounces != m_props->bounces ||
        (isHorizontal(props) &&
         props->alwaysBounceHorizontal != m_props->alwaysBounceHorizontal) ||
        (!isHorizontal(props) &&
         props->alwaysBounceVertical != m_props->alwaysBounceVertical)) {
      m_scrollNode.setEdgeEffect(
          props->bounces,
          isHorizontal(props) ? props->alwaysBounceHorizontal
                              : props->alwaysBounceVertical);
    }
  }
  if (m_rawProps.nestedScrollEnabled != rawProps.nestedScrollEnabled) {
    m_rawProps.nestedScrollEnabled = rawProps.nestedScrollEnabled;
  }

  if (m_rawProps.endFillColor != rawProps.endFillColor) {
    m_rawProps.endFillColor = rawProps.endFillColor;
    if (m_rawProps.endFillColor.has_value()) {
      m_scrollNode.setBackgroundColor(m_rawProps.endFillColor.value());
    }
  }

  if (m_rawProps.fadingEdgeLength != rawProps.fadingEdgeLength) {
    m_rawProps.fadingEdgeLength = rawProps.fadingEdgeLength;
    if (m_rawProps.fadingEdgeLength.has_value()) {
      m_scrollNode.setFadingEdge(m_rawProps.fadingEdgeLength.value());
    }
  }

  if (m_rawProps.flingSpeedLimit != rawProps.flingSpeedLimit) {
    m_rawProps.flingSpeedLimit = rawProps.flingSpeedLimit;
    if (m_rawProps.flingSpeedLimit.has_value()) {
      m_scrollNode.setFlingSpeedLimit(m_rawProps.flingSpeedLimit.value());
    }
  }

  if (!m_props || props->contentOffset != m_props->contentOffset ||
      props->scrollToOverflowEnabled != m_props->scrollToOverflowEnabled) {
    m_scrollNode.scrollTo(
        adjustOffsetIfRTL(props->contentOffset.x),
        props->contentOffset.y,
        false,
        m_scrollToOverflowEnabled);
    updateStateWithContentOffset(props->contentOffset);
  }

  if (props->centerContent != m_props->centerContent) {
    if (props->centerContent) {
      m_scrollNode.setCenterContent(true);
    } else {
      m_scrollNode.setCenterContent(false);
    }
  }

  setScrollSnap(
      props->snapToStart,
      props->snapToEnd,
      props->snapToOffsets,
      props->snapToInterval,
      props->snapToAlignment);

  auto borderMetrics = props->resolveBorderMetrics(m_layoutMetrics);
  m_contentContainerNode.setMargin(
      -borderMetrics.borderWidths.left,
      -borderMetrics.borderWidths.top,
      0.f,
      0.f);
}

void ScrollViewComponentInstance::onCommandReceived(
    std::string const& commandName,
    folly::dynamic const& args) {
  if (commandName == "scrollTo") {
    facebook::react::Float x = args[0].asDouble();
    facebook::react::Float y = args[1].asDouble();
    m_targetOffsetOfScrollToCommand = {x, y};
    m_scrollNode.scrollTo(x, y, args[2].asBool(), m_scrollToOverflowEnabled);
  } else if (commandName == "scrollToEnd") {
    scrollToEnd(args[0].asBool());
  }
}

void rnoh::ScrollViewComponentInstance::onNativeResponderBlockChange(
    bool isBlocked) {
  m_isNativeResponderBlocked = isBlocked;
  m_scrollNode.setEnableScrollInteraction(
      !isBlocked && m_props->scrollEnabled &&
      (!m_rawProps.nestedScrollEnabled.has_value() ||
       m_rawProps.nestedScrollEnabled.value() || !isNestedScroll()));
  m_scrollNode.markDirty();
}

bool rnoh::ScrollViewComponentInstance::isNestedScroll() {
  auto parent = m_parent.lock();
  while (parent) {
    if (parent->getComponentName() == this->getComponentName()) {
      return true;
    }
    parent = parent->getParent().lock();
  }
  return false;
}

facebook::react::Point rnoh::ScrollViewComponentInstance::getCurrentOffset()
    const {
  auto offset = this->getScrollOffset();
  /**
   * The line below fixes touch recognition issue that appeared,
   * after getScrollOffset started returning a negative offset during pull to
   * refresh.
   */
  offset.y += m_onPullToRefreshOffsetY.value_or(0);
  auto contentViewOffset = getContentViewOffset();

  return offset - contentViewOffset;
}

void rnoh::ScrollViewComponentInstance::updateStateWithContentOffset(
    facebook::react::Point contentOffset) {
  if (!m_state) {
    return;
  }
  if (m_state->getData().contentOffset != contentOffset) {
    m_state->updateState(
        [contentOffset](auto const& stateData)
            -> std::shared_ptr<facebook::react::ScrollViewShadowNode::
                                   ConcreteState::Data const> {
          if (stateData.contentOffset == contentOffset) {
            return nullptr;
          }
          auto newData = stateData;
          newData.contentOffset = contentOffset;
          return std::make_shared<
              facebook::react::ScrollViewShadowNode::ConcreteState::Data const>(
              newData);
        });
  }
}

facebook::react::ScrollViewEventEmitter::Metrics
ScrollViewComponentInstance::getScrollViewMetrics() {
  auto scrollViewMetrics = facebook::react::ScrollViewEventEmitter::Metrics();
  auto currentOffset = getScrollOffset();
  scrollViewMetrics.responderIgnoreScroll = true;
  scrollViewMetrics.zoomScale = 1;
  scrollViewMetrics.contentSize = m_contentSize;
  scrollViewMetrics.contentOffset = currentOffset;
  scrollViewMetrics.containerSize = m_containerSize;
  m_velocityTracker.add(currentOffset.x, currentOffset.y);
  // RN 0.88: ScrollViewMetrics 已移除 velocity 字段（改为由 ScrollEvent 携带），
  // 这里不再写入；速度追踪仍保留供其它逻辑使用。
  return scrollViewMetrics;
}

bool ScrollViewComponentInstance::isHandlingTouches() const {
  if (m_props && !m_props->scrollEnabled) {
    return false;
  }
  return m_internalState->asScrollNodeState() == ScrollNodeState::DRAGGING ||
      m_internalState->asScrollNodeState() == ScrollNodeState::SETTLING;
}

// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void ScrollViewComponentInstance::updateContentClippedSubviews() {
  if (!m_children.empty() && m_children[0] != nullptr) {
    auto contentContainer =
        std::dynamic_pointer_cast<ViewComponentInstance>(m_children[0]);
    if (contentContainer != nullptr) {
      contentContainer->updateClippedSubviews();
    }
  }
}

facebook::react::Float
ScrollViewComponentInstance::getFrictionFromDecelerationRate(
    facebook::react::Float decelerationRate) {
  constexpr facebook::react::Float ARKUI_NORMAL = 0.75f;
  constexpr facebook::react::Float ARKUI_FAST = 2.0f;
  constexpr facebook::react::Float HARMONY_FAST = 0.992f;
  constexpr facebook::react::Float kRateEpsilon = 0.00001f;
  constexpr facebook::react::Float kMinFriction = 0.1f;

  // Guard invalid input.
  if (!std::isfinite(decelerationRate) || decelerationRate < 0.0f ||
      decelerationRate >= 1.0f) {
    return ARKUI_NORMAL;
  }

  // Fast path for preset rates.
  if (std::abs(
          decelerationRate -
          ScrollViewRawProps::HARMONY_NORMAL_DECELERATION_RATE) <
      kRateEpsilon) {
    return ARKUI_NORMAL;
  }
  if (std::abs(decelerationRate - HARMONY_FAST) < kRateEpsilon) {
    return ARKUI_FAST;
  }

  // Apply mapping with lower bound.
  auto friction = 250.0f * (1.0f - decelerationRate);
  return friction < kMinFriction ? kMinFriction : friction;
}

void ScrollViewComponentInstance::scrollToEnd(bool animated) {
  bool horizontal = false;
  if (m_props) {
    horizontal = m_props->alwaysBounceHorizontal ||
        m_contentSize.width > m_containerSize.width;
  }
  auto x = horizontal ? m_contentSize.width : 0.0;
  auto y = horizontal ? 0.0 : m_contentSize.height;
  m_scrollNode.scrollTo(x, y, animated);
}

ArkUI_ScrollBarDisplayMode ScrollViewComponentInstance::getScrollBarDisplayMode(
    bool horizontal,
    bool persistentScrollBar,
    bool showsVerticalScrollIndicator,
    bool showsHorizontalScrollIndicator) {
  if (horizontal && !showsHorizontalScrollIndicator ||
      !horizontal && !showsVerticalScrollIndicator) {
    return ArkUI_ScrollBarDisplayMode::ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF;
  }
  return persistentScrollBar
      ? ArkUI_ScrollBarDisplayMode::ARKUI_SCROLL_BAR_DISPLAY_MODE_ON
      : ArkUI_ScrollBarDisplayMode::ARKUI_SCROLL_BAR_DISPLAY_MODE_AUTO;
}

void ScrollViewComponentInstance::setScrollSnap(
    bool snapToStart,
    bool snapToEnd,
    const std::vector<facebook::react::Float>& snapToOffsets,
    facebook::react::Float snapToInterval,
    facebook::react::ScrollViewSnapToAlignment snapToAlignment) {
  if (!snapToOffsets.empty()) {
    m_snapToOffsets = snapToOffsets;
    m_snapToOffsets.erase(
        std::remove_if(
            m_snapToOffsets.begin(),
            m_snapToOffsets.end(),
            [](facebook::react::Float x) { return std::isnan(x); }),
        m_snapToOffsets.end());
    std::sort(m_snapToOffsets.begin(), m_snapToOffsets.end());

    // ArkUI treats a single snap pagination value as an interval. Append
    // +inf so a single snap point is always passed as an explicit snap list.
    if (m_snapToOffsets.size() == 1) {
      m_snapToOffsets.push_back(
          std::numeric_limits<facebook::react::Float>::infinity());
    }
    m_scrollNode.setScrollSnap(
        ArkUI_ScrollSnapAlign::ARKUI_SCROLL_SNAP_ALIGN_START,
        snapToStart,
        snapToEnd,
        m_snapToOffsets);
  } else if (snapToInterval > 0) {
    const std::vector<facebook::react::Float> snapPoints = {snapToInterval};
    m_scrollNode.setScrollSnap(
        getArkUI_ScrollSnapAlign(snapToAlignment),
        snapToStart,
        snapToEnd,
        snapPoints);
  } else {
    m_scrollNode.resetScrollSnap();
  }
}
bool ScrollViewComponentInstance::scrollMovedBySignificantOffset(
    facebook::react::Point newOffset) {
  return std::abs(newOffset.x - m_currentOffset.x) >= 0.01 ||
      std::abs(newOffset.y - m_currentOffset.y) >= 0.01;
}

void ScrollViewComponentInstance::onFinalizeUpdates() {
  CppComponentInstance::onFinalizeUpdates();

  // when parent isn't refresh node, set the position
  auto parent = this->getParent().lock();
  if (parent && !parent->isRefreshControlComponentInstance()) {
    this->getLocalRootArkUINode().setLayoutRect(m_layoutMetrics);
  }
  if (m_props->maintainVisibleContentPosition.has_value()) {
    adjustVisibleContentPosition(
        m_props->maintainVisibleContentPosition.value());
    m_firstVisibleView = getFirstVisibleView(
        m_props->maintainVisibleContentPosition.value().minIndexForVisible);
  }

  if (m_shouldAdjustScrollPositionOnNextRender) {
    auto maybeKeyboardAvoider = m_keyboardAvoider.lock();
    if (maybeKeyboardAvoider != nullptr) {
      auto keyboardAvoider =
          std::dynamic_pointer_cast<KeyboardAvoider>(maybeKeyboardAvoider);
      if (keyboardAvoider != nullptr) {
        auto scrollOffset =
            keyboardAvoider->getBottomEdgeOffsetRelativeToScrollView(
                std::dynamic_pointer_cast<ScrollViewComponentInstance>(
                    this->shared_from_this()));
        auto newScrollOffset = scrollOffset - m_layoutMetrics.frame.size.height;
        if (isHorizontal(m_props)) {
          if (newScrollOffset > m_scrollNode.getScrollOffset().x) {
            m_scrollNode.scrollTo(
                newScrollOffset, m_scrollNode.getScrollOffset().y, true);
          }
        } else {
          if (newScrollOffset > m_scrollNode.getScrollOffset().y) {
            LOG(INFO)
                << "Adjusting scroll position to prevent keyboard avoider being hidden by the keyboard";
            m_scrollNode.scrollTo(
                m_scrollNode.getScrollOffset().x, newScrollOffset, true);
          }
        }
        m_keyboardAvoider.reset();
      }
    }
    m_shouldAdjustScrollPositionOnNextRender = false;
  }

  if ((m_contentSizeChanged || m_containerSizeChanged) &&
      m_internalState->asScrollNodeState() == ScrollNodeState::IDLE) {
    updateOffsetAfterChildChange(m_scrollNode.getScrollOffset());
    m_contentSizeChanged = false;
    m_containerSizeChanged = false;
  }
  resetScrollInteraction();
}

void ScrollViewComponentInstance::resetScrollInteraction() {
  m_scrollNode.setEnableScrollInteraction(
      !m_isNativeResponderBlocked && m_props->scrollEnabled &&
      (!m_rawProps.nestedScrollEnabled.has_value() ||
       m_rawProps.nestedScrollEnabled.value() || !isNestedScroll()));
}

folly::dynamic ScrollViewComponentInstance::getScrollEventPayload(
    facebook::react::ScrollViewEventEmitter::Metrics const& scrollViewMetrics) {
  using folly::dynamic;

  dynamic contentSize =
      dynamic::object("width", scrollViewMetrics.contentSize.width)(
          "height", scrollViewMetrics.contentSize.height);
  dynamic contentOffset =
      dynamic::object("x", scrollViewMetrics.contentOffset.x)(
          "y", scrollViewMetrics.contentOffset.y);
  dynamic contentInset =
      dynamic::object("left", scrollViewMetrics.contentInset.left)(
          "top", scrollViewMetrics.contentInset.top)(
          "right", scrollViewMetrics.contentInset.right)(
          "bottom", scrollViewMetrics.contentInset.bottom);
  dynamic containerSize =
      dynamic::object("width", scrollViewMetrics.containerSize.width)(
          "height", scrollViewMetrics.containerSize.height);
  auto currentOffset = getScrollOffset();
  m_velocityTracker.add(currentOffset.x, currentOffset.y);
  auto velocity = m_velocityTracker.getVelocity();
  dynamic velocityObj =
      dynamic::object("x", velocity.first)("y", velocity.second);
  dynamic payload = dynamic::object("contentSize", contentSize)(
      "contentOffset", contentOffset)("contentInset", contentInset)(
      "containerSize", containerSize)("velocity", velocityObj)(
      "zoomScale", scrollViewMetrics.zoomScale)(
      "responderIgnoreScroll", scrollViewMetrics.responderIgnoreScroll)(
      "layoutMeasurement", containerSize);
  return payload;
}

void rnoh::ScrollViewComponentInstance::sendEventForNativeAnimations(
    facebook::react::ScrollViewEventEmitter::Metrics const& scrollViewMetrics) {
  auto nativeAnimatedTurboModule = m_nativeAnimatedTurboModule.lock();
  if (nativeAnimatedTurboModule == nullptr) {
    auto instance = m_deps->rnInstance.lock();
    if (instance == nullptr) {
      return;
    }
    nativeAnimatedTurboModule =
        instance->getTurboModule<NativeAnimatedTurboModule>(
            "NativeAnimatedTurboModule");
    m_nativeAnimatedTurboModule = nativeAnimatedTurboModule;
  }
  if (nativeAnimatedTurboModule != nullptr) {
    nativeAnimatedTurboModule->handleComponentEvent(
        m_tag, "onScroll", getScrollEventPayload(scrollViewMetrics));
  }
}

bool ScrollViewComponentInstance::isContentSmallerThanContainer() {
  return isHorizontal(m_props) ? m_contentSize.width <= m_containerSize.width
                               : m_contentSize.height <= m_containerSize.height;
}

bool ScrollViewComponentInstance::isAtEnd(
    facebook::react::Point currentOffset) {
  if (isHorizontal(m_props)) {
    return currentOffset.x <= 0.001 ||
        m_contentSize.width - m_containerSize.width - currentOffset.x < 0.001;
  } else {
    return currentOffset.y <= 0.001 ||
        m_contentSize.height - m_containerSize.height - currentOffset.y < 0.001;
  }
}

bool ScrollViewComponentInstance::isCloseToTargetOffset(
    facebook::react::Point currentOffset) {
  if (m_targetOffsetOfScrollToCommand.has_value()) {
    auto flag = std::abs(m_targetOffsetOfScrollToCommand->x - currentOffset.x) <
            0.001 &&
        std::abs(m_targetOffsetOfScrollToCommand->y - currentOffset.y) < 0.001;
    if (flag) {
      m_targetOffsetOfScrollToCommand = std::nullopt;
    }
    return flag;
  }
  return false;
}

bool ScrollViewComponentInstance::isHorizontal(
    SharedConcreteProps const& props) {
  return props->horizontal;
}

void ScrollViewComponentInstance::disableIntervalMomentum() {
  if (m_props->pagingEnabled) {
    return;
  }
  auto nextSnapTarget = getNextSnapTarget();
  if (nextSnapTarget.has_value()) {
    if (isHorizontal(m_props)) {
      m_scrollNode.scrollTo(
          nextSnapTarget.value(),
          static_cast<float>(m_currentOffset.y),
          true,
          m_scrollToOverflowEnabled);
    } else {
      m_scrollNode.scrollTo(
          static_cast<float>(m_currentOffset.x),
          nextSnapTarget.value(),
          true,
          m_scrollToOverflowEnabled);
    }
  }
}

std::optional<float> ScrollViewComponentInstance::getNextSnapTarget() {
  std::optional<float> nextSnapTarget = std::nullopt;
  auto currentOffset =
      isHorizontal(m_props) ? m_currentOffset.x : m_currentOffset.y;
  bool horizontal = isHorizontal(m_props);

  if (!m_snapToOffsets.empty()) {
    if (m_recentScrollFrameOffset > 0) {
      auto upper = std::upper_bound(
          m_snapToOffsets.begin(), m_snapToOffsets.end(), currentOffset);
      if (upper != m_snapToOffsets.end() &&
          *upper < std::numeric_limits<facebook::react::Float>::infinity()) {
        nextSnapTarget = static_cast<float>(*upper);
      } else if (m_props->snapToEnd) {
        nextSnapTarget =
            horizontal ? m_contentSize.width : m_contentSize.height;
      }
    } else {
      auto lower = std::lower_bound(
          m_snapToOffsets.begin(), m_snapToOffsets.end(), currentOffset);
      auto lowerIsEnd = lower == m_snapToOffsets.end();
      auto lowerIsInfinity =
          !lowerIsEnd &&
          *lower == std::numeric_limits<facebook::react::Float>::infinity();
      if ((lowerIsEnd || lowerIsInfinity) && !m_props->snapToEnd) {
        // Past the last explicit snap point and snapToEnd is false: don't snap.
      } else if (lower == m_snapToOffsets.begin()) {
        nextSnapTarget = 0;
      } else {
        nextSnapTarget = static_cast<float>(*(std::prev(lower, 1)));
      }
    }
  } else if (m_props->snapToInterval > 0) {
    auto interval = m_props->snapToInterval;
    auto intervalIndex = (m_recentScrollFrameOffset > 0)
        ? std::ceil(currentOffset / interval)
        : std::floor(currentOffset / interval);
    nextSnapTarget = static_cast<float>(intervalIndex * interval);
  }
  return nextSnapTarget;
}

ScrollViewComponentInstance::ScrollViewRawProps
ScrollViewComponentInstance::ScrollViewRawProps::getFromDynamic(
    folly::dynamic value) {
  auto overScrollMode = (value.count("overScrollMode") > 0)
      ? std::optional(value["overScrollMode"].asString())
      : std::nullopt;
  auto nestedEnabled = (value.count("nestedScrollEnabled") > 0)
      ? std::optional(value["nestedScrollEnabled"].asBool())
      : std::nullopt;
  auto endFillColor = (value.count("endFillColor") > 0)
      ? std::optional(value["endFillColor"].asInt())
      : std::nullopt;

  auto fadingEdgeLength = (value.count("fadingEdgeLength") > 0)
      ? std::optional(value["fadingEdgeLength"].asDouble())
      : std::nullopt;
  auto flingSpeedLimit = (value.count("flingSpeedLimit") > 0)
      ? std::optional<float>(value["flingSpeedLimit"].asDouble())
      : std::nullopt;
  auto decelerationRate =
      (value.count("decelerationRate") > 0 &&
       !value["decelerationRate"].isNull())
      ? static_cast<facebook::react::Float>(
            value["decelerationRate"].asDouble())
      : HARMONY_NORMAL_DECELERATION_RATE;

  return {
      overScrollMode,
      nestedEnabled,
      endFillColor,
      fadingEdgeLength,
      flingSpeedLimit,
      decelerationRate};
}

facebook::react::Point ScrollViewComponentInstance::getContentViewOffset()
    const {
  facebook::react::Point contentViewOffset = {0, 0};
  if (m_props->centerContent) {
    if (m_contentSize.width < m_containerSize.width) {
      contentViewOffset.x = (m_containerSize.width - m_contentSize.width) / 2;
    }
    if (m_contentSize.height < m_containerSize.height) {
      contentViewOffset.y = (m_containerSize.height - m_contentSize.height) / 2;
    }
  }
  return contentViewOffset;
}

void ScrollViewComponentInstance::adjustVisibleContentPosition(
    facebook::react::ScrollViewMaintainVisibleContentPosition const&
        scrollViewMaintainVisibleContentPosition) {
  if (!m_firstVisibleView.has_value() || m_children.empty() ||
      m_children[0] == nullptr) {
    return;
  }

  const auto& firstVisibleView = m_firstVisibleView.value();
  ComponentInstance::Shared firstVisibleChild = nullptr;
  for (const auto& child : m_children[0]->getChildren()) {
    const auto& childComponentInstance =
        std::static_pointer_cast<ComponentInstance>(child);
    if (childComponentInstance->getTag() == firstVisibleView.tag) {
      firstVisibleChild = childComponentInstance;
      break;
    }
  }
  if (firstVisibleChild == nullptr) {
    return;
  }
  auto newPosition = firstVisibleChild->getLayoutMetrics().frame.origin;

  if (isHorizontal(m_props)) {
    auto deltaX = newPosition.x - firstVisibleView.offset;
    if (deltaX != 0 && m_contentSizeChanged) {
      auto scrollX = m_currentOffset.x;
      m_scrollNode.scrollTo(scrollX + deltaX, m_currentOffset.y, false);

      if (scrollViewMaintainVisibleContentPosition.autoscrollToTopThreshold
              .has_value() &&
          scrollX <= scrollViewMaintainVisibleContentPosition
                         .autoscrollToTopThreshold.value()) {
        m_scrollNode.scrollTo(0, m_currentOffset.y, true);
      }
    }
  } else {
    auto deltaY = newPosition.y - firstVisibleView.offset;
    if (deltaY != 0 && m_contentSizeChanged) {
      auto scrollY = m_currentOffset.y;
      m_scrollNode.scrollTo(m_currentOffset.x, scrollY + deltaY, false);

      if (scrollViewMaintainVisibleContentPosition.autoscrollToTopThreshold
              .has_value() &&
          scrollY <= scrollViewMaintainVisibleContentPosition
                         .autoscrollToTopThreshold.value()) {
        m_scrollNode.scrollTo(m_currentOffset.x, 0, true);
      }
    }
  }
}

std::optional<ScrollViewComponentInstance::ChildTagWithOffset>
ScrollViewComponentInstance::getFirstVisibleView(int32_t minIndexForVisible) {
  if (!m_props || m_children.empty() || m_children[0] == nullptr) {
    return std::nullopt;
  }

  auto currentScrollPosition =
      isHorizontal(m_props) ? m_currentOffset.x : m_currentOffset.y;
  auto const& scrollViewChildren = m_children[0]->getChildren();
  if (scrollViewChildren.empty()) {
    return std::nullopt;
  }

  minIndexForVisible = std::max(minIndexForVisible, 0);
  for (auto it = scrollViewChildren.begin() + minIndexForVisible;
       it < scrollViewChildren.end();
       it++) {
    auto childComponentInstance =
        std::static_pointer_cast<ComponentInstance>(*it);
    auto childLayoutMetrics = childComponentInstance->getLayoutMetrics();
    auto position = isHorizontal(m_props) ? childLayoutMetrics.frame.origin.x
                                          : childLayoutMetrics.frame.origin.y;

    bool isViewVisible = isHorizontal(m_props)
        ? position + childLayoutMetrics.frame.size.width > currentScrollPosition
        : position + childLayoutMetrics.frame.size.height >
            currentScrollPosition;
    if (isViewVisible) {
      return std::optional<ScrollViewComponentInstance::ChildTagWithOffset>(
          {childComponentInstance->getTag(), position});
    }
  }

  auto lastChild =
      std::static_pointer_cast<ComponentInstance>(scrollViewChildren.back());
  auto position = isHorizontal(m_props)
      ? lastChild->getLayoutMetrics().frame.origin.x
      : lastChild->getLayoutMetrics().frame.origin.y;
  return std::optional<ScrollViewComponentInstance::ChildTagWithOffset>(
      {lastChild->getTag(), position});
}

void ScrollViewComponentInstance::onAppear() {
  if (!m_state || !m_props) {
    return;
  }

  const auto& stateData = m_state->getData();
  bool isContentOffsetZero =
      stateData.contentOffset == facebook::react::Point{0, 0};

  if (isContentOffsetZero) {
    m_scrollNode.scrollTo(
        m_props->contentOffset.x, m_props->contentOffset.y, false);
    updateStateWithContentOffset(m_props->contentOffset);
  }

  if (!isContentOffsetZero) {
    m_scrollNode.scrollTo(
        stateData.contentOffset.x, stateData.contentOffset.y, false);
  }
}

bool ScrollViewComponentInstance::setKeyboardAvoider(
    ComponentInstance::Weak keyboardAvoidingComponentInstance) {
  m_keyboardAvoider = keyboardAvoidingComponentInstance;
  return true;
}

// NOTE: ArkUI ScrollNode's offset is calculated relative to top-right if
// RTL is enabled, while RN expects it to always be counted from top-left
facebook::react::Float ScrollViewComponentInstance::adjustOffsetIfRTL(
    facebook::react::Float x) const {
  auto isRTL = m_layoutMetrics.layoutDirection ==
      facebook::react::LayoutDirection::RightToLeft;
  if (isRTL) {
    x = m_contentSize.width - m_containerSize.width - x;
  }
  return x;
}

facebook::react::Point ScrollViewComponentInstance::getScrollOffset() const {
  auto scrollOffset = m_scrollNode.getScrollOffset();
  scrollOffset.x = adjustOffsetIfRTL(scrollOffset.x);
  if (m_onPullToRefreshOffsetY.has_value()) {
    scrollOffset.y -= m_onPullToRefreshOffsetY.value();
  }
  return scrollOffset;
}

void ScrollViewComponentInstance::emitScrollEvent(
    const std::string& eventName,
    const facebook::react::ScrollViewEventEmitter::Metrics& metrics) {
  if (!m_eventEmitter) {
    return;
  }
  auto customPayload = getScrollEventPayload(metrics);
  if (eventName == "scroll") {
    m_eventEmitter->dispatchUniqueEvent(
        "scroll",
        std::make_shared<facebook::react::ScrollEvent>(getScrollViewMetrics()));
  } else {
    m_eventEmitter->dispatchEvent(
        eventName, [customPayload](facebook::jsi::Runtime& runtime) {
          return facebook::jsi::valueFromDynamic(runtime, customPayload);
        });
  }
}

void ScrollViewComponentInstance::setNestedScrollMode(
    ArkUI_ScrollNestedMode scrollForward,
    ArkUI_ScrollNestedMode scrollBackward) {
  m_isScrollNestedModeManagedByLibrary = true;
  m_scrollNode.setNestedScroll(scrollForward, scrollBackward);
}
} // namespace rnoh
