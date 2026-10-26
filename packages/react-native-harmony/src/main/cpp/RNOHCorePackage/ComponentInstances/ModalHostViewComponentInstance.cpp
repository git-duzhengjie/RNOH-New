/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ModalHostViewComponentInstance.h"

#include <glog/logging.h>
#include <react/renderer/components/FBReactNativeSpec/Props.h>
#include "RNOH/ArkTSBridge.h"
#include "RNOH/Assert.h"
#include "RNOH/SchedulerDelegate.h"
#include "RNOH/arkui/NativeNodeApi.h"
#include "RNOH/arkui/TouchEventDispatcher.h"
#include "RNOH/arkui/UIInputEventHandler.h"

namespace rnoh {

static constexpr int32_t ANIMATION_DURATION = 300;
static constexpr const char* PAGE_BOUND_PREFIX = "__harmony::page-bound:";

class ModalHostTouchHandler : public UIInputEventHandler {
 private:
  ModalHostViewComponentInstance* m_rootView;
  TouchEventDispatcher m_touchEventDispatcher;

 public:
  ModalHostTouchHandler(ModalHostTouchHandler const& other) = delete;
  ModalHostTouchHandler& operator=(ModalHostTouchHandler const& other) = delete;
  ModalHostTouchHandler(ModalHostTouchHandler&& other) = delete;
  ModalHostTouchHandler& operator=(ModalHostTouchHandler&& other) = delete;

  ModalHostTouchHandler(ModalHostViewComponentInstance* rootView)
      : UIInputEventHandler(rootView->m_rootCustomNode), m_rootView(rootView) {}

  void onTouchEvent(ArkUI_UIInputEvent* event) override {
    m_touchEventDispatcher.dispatchTouchEvent(
        event, m_rootView->shared_from_this());
  }

  void onRootViewAboutToDisappear() override {
    m_touchEventDispatcher.cancelActiveTouches();
  }

  ~ModalHostTouchHandler() {
    m_touchEventDispatcher.cancelActiveTouches();
  }
};

void ModalHostViewComponentInstance::updateDisplayRect(
    DisplayMetrics const& displayMetrics,
    SharedConcreteState const& state) {
  const auto& windowMetrics = displayMetrics.windowPhysicalPixels;
  auto screenOrientation = (displayMetrics.windowPhysicalPixels.height >=
                            displayMetrics.windowPhysicalPixels.width)
      ? ScreenOrientation::Portrait
      : ScreenOrientation::Landscape;

  if (m_screenOrientation != screenOrientation) {
    m_screenOrientation = screenOrientation;
    if (m_eventEmitter) {
      m_eventEmitter->onOrientationChange({.orientation = screenOrientation});
    }
  }
  auto stackNodeWidth =
      (windowMetrics.left + windowMetrics.width) / windowMetrics.scale;
  auto stackNodeHeight =
      (windowMetrics.top + windowMetrics.height) / windowMetrics.scale;
  m_rootStackNode.setSize(
      facebook::react::Size{stackNodeWidth, stackNodeHeight});
  m_rootCustomNode.setPosition(
      {windowMetrics.left / windowMetrics.scale,
       windowMetrics.top / windowMetrics.scale});
  facebook::react::Size screenSize = {
      .width = windowMetrics.width / windowMetrics.scale,
      .height = (windowMetrics.height - windowMetrics.decorHeight) /
          windowMetrics.scale};
  state->updateState({screenSize});
  if (m_props->animationType ==
      facebook::react::ModalHostViewAnimationType::Slide) {
    updateSlideTransition(displayMetrics);
  }
}

void ModalHostViewComponentInstance::updateSlideTransition(
    DisplayMetrics const& displayMetrics) {
  const auto& screenSize = displayMetrics.windowPhysicalPixels;
  m_rootCustomNode.setTranslateTransition(
      0,
      float((screenSize.height - screenSize.decorHeight) / screenSize.scale),
      ANIMATION_DURATION);
}

ModalHostViewComponentInstance::ModalHostViewComponentInstance(Context context)
    : CppComponentInstance(std::move(context)),
      ArkTSMessageHub::Observer(m_deps->arkTSMessageHub),
      m_touchHandler(std::make_unique<ModalHostTouchHandler>(this)),
      m_virtualNode(m_arkUINodeCtx),
      m_rootCustomNode(m_arkUINodeCtx) {
  m_virtualNode.setSize(facebook::react::Size{0, 0});
  m_dialogHandler.setDialogDelegate(this);
  m_rootStackNode.insertChild(m_rootCustomNode, 0);
}

ModalHostViewComponentInstance::~ModalHostViewComponentInstance() {
  if (m_modalVisibilityReported) {
    SchedulerDelegate::notifyModalVisibilityChanged(false);
    m_modalVisibilityReported = false;
  }
}

void ModalHostViewComponentInstance::setLayout(
    facebook::react::LayoutMetrics layoutMetrics) {
  CppComponentInstance::setLayout(layoutMetrics);
  int32_t width = static_cast<int32_t>(
      layoutMetrics.frame.size.width * layoutMetrics.pointScaleFactor + 0.5);
  int32_t height = static_cast<int32_t>(
      layoutMetrics.frame.size.height * layoutMetrics.pointScaleFactor + 0.5);
  m_rootCustomNode.updateMeasuredSize(width, height);
  m_rootCustomNode.setSize(layoutMetrics.frame.size);
}

void ModalHostViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  using AnimationType = facebook::react::ModalHostViewAnimationType;
  CppComponentInstance::onPropsChanged(props);
  updateBindingFromProps(props);
  if (props->animationType != m_props->animationType) {
    if (props->animationType == AnimationType::Slide) {
      m_rootCustomNode.resetOpacityTransition();
      auto screenSize = m_deps->displayMetricsManager->getDisplayMetrics();
      updateSlideTransition(screenSize);
    } else if (props->animationType == AnimationType::Fade) {
      m_rootCustomNode.setTranslateTransition(0, 0, 0);
      m_rootCustomNode.setOpacityTransition(ANIMATION_DURATION);
    } else {
      m_rootCustomNode.setTranslateTransition(0, 0, 0);
      m_rootCustomNode.resetOpacityTransition();
    }
  }
}

void ModalHostViewComponentInstance::onStateChanged(
    SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);
  if (!m_state) {
    // set screen rect the first time the component is initialized
    auto displayMetrics = m_deps->displayMetricsManager->getDisplayMetrics();
    updateDisplayRect(displayMetrics, state);
  }
}

void ModalHostViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_rootCustomNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void ModalHostViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_rootCustomNode.removeChild(childComponentInstance->getLocalRootArkUINode());
};

// Modal content is dispatched from an independent ArkUI dialog, so its touch
// root must not inherit pointerEvents from the page behind the dialog.
bool ModalHostViewComponentInstance::canHandleTouch() const {
  return m_props == nullptr ||
      m_props->pointerEvents == facebook::react::PointerEventsMode::Auto ||
      m_props->pointerEvents == facebook::react::PointerEventsMode::BoxOnly;
}

bool ModalHostViewComponentInstance::canChildrenHandleTouch() const {
  return m_props == nullptr ||
      m_props->pointerEvents == facebook::react::PointerEventsMode::Auto ||
      m_props->pointerEvents == facebook::react::PointerEventsMode::BoxNone;
}

void ModalHostViewComponentInstance::onArkUINodeTouchIntercept(
    const ArkUI_UIInputEvent* event) {
  // m_virtualNode stays in the page's ArkUI tree. Preserve its original,
  // ancestor-aware intercept behavior; only the dialog is a new touch root.
  auto parent = getParent().lock();
  if (parent && !parent->canChildrenHandleTouch()) {
    auto mode = IsAtLeastApi20()
        ? HitTestMode::HTM_BLOCK_DESCENDANTS
        : HitTestMode::HTM_NONE;
    OH_ArkUI_PointerEvent_SetInterceptHitTestMode(event, mode);
    return;
  }
  CppComponentInstance::onArkUINodeTouchIntercept(event);
}

void ModalHostViewComponentInstance::onFinalizeUpdates() {
  // only show modal after the screen size has been set and processed by RN
  auto isScreenSizeSet = m_state && m_state->getData().screenSize.height != 0 &&
      m_state->getData().screenSize.width != 0;
  auto shouldShowDialog =
      m_props->visible && !m_dialogHandler.isShow() && isScreenSizeSet;
  if (shouldShowDialog) {
    showDialog();
  }
  auto shouldCloseDialog = !m_props->visible && m_dialogHandler.isShow();
  if (shouldCloseDialog) {
    closeDialog();
  }
  CppComponentInstance::onFinalizeUpdates();
}

void ModalHostViewComponentInstance::showDialog() {
  m_dialogHandler.setContent(m_rootStackNode);
  m_dialogHandler.show();
}

void ModalHostViewComponentInstance::closeDialog() {
  m_touchHandler->onRootViewAboutToDisappear();
  if (m_eventEmitter != nullptr) {
    m_eventEmitter->onDismiss({});
  }
}

void ModalHostViewComponentInstance::onShow() {
  if (!m_modalVisibilityReported) {
    SchedulerDelegate::notifyModalVisibilityChanged(true);
    m_modalVisibilityReported = true;
  }
  if (m_eventEmitter != nullptr) {
    m_eventEmitter->onShow({});
  }
}

void ModalHostViewComponentInstance::onRequestClose() {
  if (m_eventEmitter != nullptr) {
    m_eventEmitter->onRequestClose({});
  }
}

CustomNode& ModalHostViewComponentInstance::getLocalRootArkUINode() {
  return m_virtualNode;
}

void ModalHostViewComponentInstance::onMessageReceived(
    ArkTSMessage const& message) {
  if (message.name == "WINDOW_SIZE_CHANGE") {
    if (m_state) {
      auto displayMetrics = m_deps->displayMetricsManager->getDisplayMetrics();
      updateDisplayRect(displayMetrics, m_state);
    }
  }
}

void ModalHostViewComponentInstance::updateBindingFromProps(
    SharedConcreteProps const& props) {
  int32_t newTargetId = -1;

  if (props && !props->nativeId.empty()) {
    const std::string prefix = PAGE_BOUND_PREFIX;
    const auto& nativeId = props->nativeId;
    if (nativeId.rfind(prefix, 0) == 0) {
      const auto idString = nativeId.substr(prefix.size());
      if (!idString.empty()) {
        try {
          size_t pos = 0;
          int32_t parsedId = std::stoi(idString, &pos);
          if (pos == idString.size() && parsedId >= 0) {
            newTargetId = parsedId;
          } else if (pos != idString.size()) {
            LOG(WARNING) << "Unexpected trailing characters in native page binding id: "
                         << idString << ", parsed: " << parsedId;
          } else {
            LOG(WARNING) << "Negative page binding id is not valid: " << parsedId
                         << ", from nativeID: " << nativeId;
          }
        } catch (const std::exception& e) {
          LOG(WARNING) << "Failed to parse native page binding id from nativeID: "
                       << nativeId << ", reason: " << e.what();
        }
      } else {
        LOG(WARNING) << "Empty page binding id in nativeID: " << nativeId;
      }
    }
  }

  if (newTargetId == m_dialogHandler.getPageBindingTarget()) {
    return;
  }

  if (newTargetId >= 0) {
    m_dialogHandler.setPageBindingTarget(newTargetId);
  } else {
    m_dialogHandler.clearPageBindingTarget();
  }
}

} // namespace rnoh
