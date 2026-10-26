/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <react/renderer/components/scrollview/ScrollViewEventEmitter.h>
#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/RefreshNode.h"
#include "RNOH/arkui/ScrollNode.h"
#include "RNOH/arkui/StackNode.h"
#include "RNOH/arkui/UIInputEventHandler.h"
#include "RNOHCorePackage/TurboModules/Animated/NativeAnimatedTurboModule.h"
#include "ScrollViewHelpers/VelocityTracker.h"

namespace rnoh {
enum ScrollNodeState : int32_t { IDLE, DRAGGING, SETTLING, CANCELING };

class PullToRefreshViewComponentInstance;
class ScrollViewComponentInstance;

/**
 * Represents a state in the State design pattern. The term "Internal"
 * distinguishes it from ScrollViewState — a React Native class.
 */
class ScrollViewInternalState {
 protected:
  ScrollViewComponentInstance* m_instance;

 public:
  using Unique = std::unique_ptr<ScrollViewInternalState>;

  ScrollViewInternalState(ScrollViewComponentInstance* instance);

  virtual ~ScrollViewInternalState() {}

  virtual std::string getDebugName() {
    return "ScrollViewInternalState";
  }

  virtual ScrollNodeState asScrollNodeState() = 0;

  virtual void onScroll() {
    LOG(WARNING) << "ScrollViewInternalState::" << this->getDebugName()
                 << "::onScroll";
  }
  virtual void onScrollStart() {
    LOG(WARNING) << "ScrollViewInternalState::" << this->getDebugName()
                 << "::onScrollStart";
  }
  virtual void onScrollStop() {
    LOG(WARNING) << "ScrollViewInternalState::" << this->getDebugName()
                 << "::onScrollStop";
  }
  virtual void onDragStart() {
    LOG(WARNING) << "ScrollViewInternalState::" << this->getDebugName()
                 << "::onDragStart";
  }
  virtual void onDragStop() {
    LOG(WARNING) << "ScrollViewInternalState::" << this->getDebugName()
                 << "::onDragStop";
  }
};

class IdleScrollViewInternalState : public ScrollViewInternalState {
 public:
  using ScrollViewInternalState::ScrollViewInternalState;
  std::string getDebugName() override {
    return "IDLE";
  }

  ScrollNodeState asScrollNodeState() override {
    return ScrollNodeState::IDLE;
  }

  void onScrollStart() override;
  void onDragStart() override;
  void onScroll() override;
};

class CancelingScrollViewInternalState : public ScrollViewInternalState {
 public:
  using ScrollViewInternalState::ScrollViewInternalState;

  std::string getDebugName() override {
    return "CANCELING";
  }

  ScrollNodeState asScrollNodeState() override {
    return ScrollNodeState::CANCELING;
  }

  void onScrollStop() override;
};

class DraggingScrollViewInternalState : public ScrollViewInternalState {
 public:
  using ScrollViewInternalState::ScrollViewInternalState;

  std::string getDebugName() override {
    return "DRAGGING";
  }

  ScrollNodeState asScrollNodeState() override {
    return ScrollNodeState::DRAGGING;
  }

  void onDragStop() override;
  void onScroll() override;
  void onScrollStart() override;
  void onScrollStop() override;
};

class SettlingScrollViewInternalState : public ScrollViewInternalState {
  bool m_hasOnScrollBeenCalled = false;

 public:
  using ScrollViewInternalState::ScrollViewInternalState;

  std::string getDebugName() override {
    return "SETTLING";
  }

  ScrollNodeState asScrollNodeState() override {
    return ScrollNodeState::SETTLING;
  }

  void onDragStart() override;
  void onDragStop() override;
  void onScroll() override;
  void onScrollStop() override;
};

// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/**
 * HACK: This interface is needed for adjusting scroll position when keyboard
 * appears. It is meant to be implemented by ComponentInstances that can trigger
 * keyboard visibility e.g. TextInput.
 *
 * Adjusting the scroll position when the keyboard appears is the
 * responsibility of the platform. However, the platform doesn't adjust the
 * position correctly when a ScrollView changes its dimensions right after
 * the platform starts adjusting the position. To fix this problem, this hack
 * checks for changes in "__keyboardAvoidingViewBottomHeight" to detect
 * mutations triggered by the keyboard appearance.
 *
 * "__keyboardAvoidingViewBottomHeight" is injected by KeyboardAvoidingView.
 * ScrollView needs to be placed directly inside KeyboardAvoidingView.
 */
class KeyboardAvoider {
 public:
  using Weak = std::weak_ptr<KeyboardAvoider>;

  virtual facebook::react::Float getBottomEdgeOffsetRelativeToScrollView(
      std::shared_ptr<ScrollViewComponentInstance> scrollView) = 0;
};

class ScrollViewComponentInstance
    : public CppComponentInstance<facebook::react::ScrollViewShadowNode>,
      public ScrollNodeDelegate {
 private:
  struct ChildTagWithOffset {
    facebook::react::Tag tag;
    facebook::react::Float offset;
  };
  ScrollViewInternalState::Unique m_internalState;
  ScrollNode m_scrollNode;
  StackNode m_contentContainerNode;
  facebook::react::Size m_contentSize;
  facebook::react::Size m_containerSize;
  facebook::react::Point m_currentOffset = {0, 0};
  bool m_persistentScrollbar = false;
  long long m_lastScrollDispatchTime = 0;
  long long m_lastStateUpdateTimeMs = 0;
  bool m_allowNextScrollEvent = false;
  facebook::react::Float m_scrollEventThrottle = 0;
  bool m_isNativeResponderBlocked = false;
  std::weak_ptr<NativeAnimatedTurboModule> m_nativeAnimatedTurboModule{};
  bool m_allowScrollPropagation = true;
  bool m_disableIntervalMomentum = false;
  bool m_scrollToOverflowEnabled = false;
  bool m_movedBySignificantOffset = false;
  float m_recentScrollFrameOffset = 0;
  bool m_shouldAdjustScrollPositionOnNextRender = false;
  bool m_isScrollNestedModeManagedByLibrary = false;
  std::vector<facebook::react::Float> m_snapToOffsets = {};
  std::optional<ChildTagWithOffset> m_firstVisibleView = std::nullopt;
  std::unique_ptr<UIInputEventHandler> m_touchHandler;
  /**
   * ScrollNode doesn't emit events during pull to refresh action which messes
   * up sticky headers.
   */
  std::optional<float> m_onPullToRefreshOffsetY = std::nullopt;
  std::optional<bool> m_appliedBackToTop = std::nullopt;

  /**
   * `onScrollFrameBegin` is not always called but is needed to detect when
   * the user stops dragging. The properties
   * `m_onScrollCallsAfterFrameBeginCallCounter` and
   * `wasInDraggingStateAtTouchUp` are part of a workaround. The fix assumes
   * `onScrollFrameBegin` follows `onScroll` during active scrolling if not, the
   * user has either stopped dragging or is overscrolling.
   */
  int m_onScrollCallsAfterFrameBeginCallCounter = 0;
  bool wasInDraggingStateAtTouchUp = false;
  bool m_contentSizeChanged = false;
  bool m_containerSizeChanged = false;

  // ———————————————————————————————————————————————————————————————————————————————————————————————————————————————————
  struct ScrollViewRawProps {
    std::optional<std::string> overScrollMode;
    std::optional<bool> nestedScrollEnabled;
    std::optional<uint32_t> endFillColor;
    std::optional<float> fadingEdgeLength;
    std::optional<float> flingSpeedLimit;
    static constexpr facebook::react::Float
        HARMONY_NORMAL_DECELERATION_RATE = 0.997f;
    facebook::react::Float decelerationRate{
        HARMONY_NORMAL_DECELERATION_RATE};
    static ScrollViewRawProps getFromDynamic(folly::dynamic value);
  };
  ScrollViewRawProps m_rawProps;
  // (implicit) ScrollViewInternalStateDelegate

  void onChangeInternalState(
      std::unique_ptr<ScrollViewInternalState> internalState);
  void onEmitMomentumScrollBegin();
  void onEmitOnMomentumScrollEndEvent();
  void onEmitOnScrollBeginDragEvent();
  void onEmitOnScrollEndDragEvent();
  void onEmitOnScrollEvent();
  void onDraggingScrollStop();
  void onDraggingScrollDragStop();
  // ———————————————————————————————————————————————————————————————————————————————————————————————————————————————————

  facebook::react::Float getFrictionFromDecelerationRate(
      facebook::react::Float decelerationRate);
  void scrollToEnd(bool animated);
  ArkUI_ScrollBarDisplayMode getScrollBarDisplayMode(
      bool horizontal,
      bool persistentScrollBar,
      bool showsVerticalScrollIndicator,
      bool showsHorizontalScrollIndicator);
  void setScrollSnap(
      bool snapToStart,
      bool snapToEnd,
      const std::vector<facebook::react::Float>& snapToOffsets,
      facebook::react::Float snapToInterval,
      facebook::react::ScrollViewSnapToAlignment snapToAlignment);
  bool scrollMovedBySignificantOffset(facebook::react::Point newOffset);
  folly::dynamic getScrollEventPayload(
      facebook::react::ScrollViewEventEmitter::Metrics const&
          scrollViewMetrics);

  void sendEventForNativeAnimations(
      facebook::react::ScrollViewEventEmitter::Metrics const&
          scrollViewMetrics);
  bool isHorizontal(SharedConcreteProps const& props);
  void disableIntervalMomentum();
  std::optional<float> getNextSnapTarget();
  void adjustVisibleContentPosition(
      facebook::react::ScrollViewMaintainVisibleContentPosition const&
          scrollViewMaintainVisibleContentPosition);
  std::optional<ChildTagWithOffset> getFirstVisibleView(
      int32_t minIndexForVisible);

  void updateContentClippedSubviews();

 public:
  /**
   * PullToRefreshViewComponentInstance is tightly coupled with ScrollView.
   */
  friend PullToRefreshViewComponentInstance;
  friend ScrollViewInternalState;
  friend IdleScrollViewInternalState;
  friend DraggingScrollViewInternalState;
  friend SettlingScrollViewInternalState;
  friend CancelingScrollViewInternalState;

  ScrollViewComponentInstance(Context context);

  ScrollNode& getLocalRootArkUINode() override;

  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;

  void onStateChanged(SharedConcreteState const& state) override;
  void onPropsChanged(SharedConcreteProps const& props) override;
  void onLayoutChanged(
      facebook::react::LayoutMetrics const& layoutMetrics) override;

  void onCommandReceived(
      std::string const& commandName,
      folly::dynamic const& args) override;

  void onPullToRefreshOffsetChange(float offsetY);
  void onPullToRefreshStateChanged(
      RefreshNodeDelegate::RefreshStatus refreshState);
  // ScrollNodeDelegate implementation
  void onScroll() override;
  void onScrollStart() override;
  void onScrollStop() override;
  float onScrollFrameBegin(float offset, int32_t scrollState) override;
  void onAppear() override;

  void onFinalizeUpdates() override;

  // TouchTarget implementation
  facebook::react::Point getCurrentOffset() const override;

  facebook::react::ScrollViewEventEmitter::Metrics getScrollViewMetrics();

  bool isHandlingTouches() const override;

  bool setKeyboardAvoider(
      ComponentInstance::Weak keyboardAvoidingComponentInstance);

  void onTouchEventActionUp();

  void setNestedScrollMode(
      ArkUI_ScrollNestedMode scrollForward,
      ArkUI_ScrollNestedMode scrollBackward);

 protected:
  void onNativeResponderBlockChange(bool isBlocked) override;

 private:
  void updateStateWithContentOffset(facebook::react::Point contentOffset);
  void updateOffsetAfterChildChange(facebook::react::Point offset);
  bool isContentSmallerThanContainer();
  bool isAtEnd(facebook::react::Point currentOffset);
  /**
   * When the `onEndReachedThreshold` of VirtualizedList is set to 0,
   * due to the rebound effect of the Scroll component,
   * after calling `scrollToEnd`, the last remaining position deviates from the
   * bottom, causing `onEndReached` to not be triggered. So when
   * `isCloseToTargetOffset` returns true, you need to actively emit the
   * `onScroll` event.
   */
  bool isCloseToTargetOffset(facebook::react::Point currentOffset);
  bool isNestedScroll();
  void onContentSizeChanged();
  facebook::react::Float adjustOffsetIfRTL(facebook::react::Float x) const;
  facebook::react::Point getScrollOffset() const;
  facebook::react::Point getContentViewOffset() const;
  bool shouldDisableScrollInteraction();
  void resetScrollInteraction();
  void emitScrollEvent(
      const std::string& eventName,
      const facebook::react::ScrollViewEventEmitter::Metrics& metrics);
  ComponentInstance::Weak m_keyboardAvoider;
  std::optional<facebook::react::Point> m_targetOffsetOfScrollToCommand =
      std::nullopt;
  rnoh::VelocityTracker m_velocityTracker;
};
} // namespace rnoh
