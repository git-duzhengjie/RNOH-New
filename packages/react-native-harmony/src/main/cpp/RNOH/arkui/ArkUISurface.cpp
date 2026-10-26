/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ArkUISurface.h"
#include <cxxreact/TraceSection.h>
#include <glog/logging.h>
#include <react/renderer/components/root/RootComponentDescriptor.h>
#include "NativeNodeApi.h"
#include "RNOH/Assert.h"
#include "RNOH/ParallelCheck.h"
#include "RNOH/Performance/RNOHMarker.h"
#include "TouchEventDispatcher.h"
#include "UIInputEventHandler.h"

namespace rnoh {

using facebook::react::Scheduler;
using facebook::react::SurfaceHandler;
using facebook::react::SurfaceId;

class SurfaceTouchEventHandler : public UIInputEventHandler,
                                 public ArkTSMessageHub::Observer {
 private:
  ComponentInstance::Shared m_rootView;
  TouchEventDispatcher m_touchEventDispatcher;
  size_t m_rnInstanceId;

 public:
  SurfaceTouchEventHandler(
      ComponentInstance::Shared rootView,
      ArkTSMessageHub::Shared arkTSMessageHub,
      size_t rnInstanceId)
      : UIInputEventHandler(rootView->getLocalRootArkUINode()),
        ArkTSMessageHub::Observer(arkTSMessageHub),
        m_rootView(std::move(rootView)),
        m_rnInstanceId(rnInstanceId) {}
  SurfaceTouchEventHandler(SurfaceTouchEventHandler const& other) = delete;
  SurfaceTouchEventHandler& operator=(SurfaceTouchEventHandler const& other) =
      delete;

  SurfaceTouchEventHandler(SurfaceTouchEventHandler&& other) noexcept = delete;
  SurfaceTouchEventHandler& operator=(
      SurfaceTouchEventHandler&& other) noexcept = delete;

  ~SurfaceTouchEventHandler() override = default;

  void onTouchEvent(ArkUI_UIInputEvent* event) override {
    auto action = OH_ArkUI_UIInputEvent_GetAction(event);
    auto actionName = "UNKNOWN";
    switch (action) {
      case UI_TOUCH_EVENT_ACTION_UP:
        actionName = "UP";
        break;
      case UI_TOUCH_EVENT_ACTION_CANCEL:
        actionName = "CANCEL";
        break;
      case UI_TOUCH_EVENT_ACTION_MOVE:
        actionName = "MOVE";
        break;
      case UI_TOUCH_EVENT_ACTION_DOWN:
        actionName = "DOWN";
        break;
    }
    facebook::react::TraceSection s(
        (std::string("#RNOH::ArkUISurface::onTouchEvent::") + actionName)
            .c_str());
    m_touchEventDispatcher.dispatchTouchEvent(event, m_rootView);
  }

  void onMessageReceived(ArkTSMessage const& message) override {
    LOG(INFO) << "onMessageReceived: " << message.name;
    if (message.name == "CANCEL_TOUCHES" &&
        message.payload["rnInstanceId"].asInt() == m_rnInstanceId) {
      m_touchEventDispatcher.cancelActiveTouches();
    }
  }
};

ArkUISurface::ArkUISurface(
    TaskExecutor::Shared taskExecutor,
    std::shared_ptr<Scheduler> scheduler,
    ComponentInstanceRegistry::Shared componentInstanceRegistry,
    ComponentInstanceFactory::Shared const& componentInstanceFactory,
    ArkTSMessageHub::Shared arkTSMessageHub,
    DisplayMetricsManager::Shared displayMetricsManager,
    SurfaceId surfaceId,
    size_t rnInstanceId,
    std::string const& appKey)
    : m_surfaceId(surfaceId),
      m_scheduler(std::move(scheduler)),
      m_componentInstanceRegistry(std::move(componentInstanceRegistry)),
      m_surfaceHandler(std::make_unique<SurfaceHandler>(appKey, surfaceId)),
      m_displayMetricsManager(std::move(displayMetricsManager)),
      m_rnInstanceId(rnInstanceId) {
  m_scheduler->registerSurface(*m_surfaceHandler);
  m_taskExecutor = taskExecutor;
  m_rootView = componentInstanceFactory->create(
      surfaceId, facebook::react::RootShadowNode::Handle(), "RootView");
  if (m_rootView == nullptr) {
    LOG(ERROR) << "Couldn't create rootInstanceComponent for surface with id: "
               << surfaceId;
    return;
  }
  m_componentInstanceRegistry->insert(m_rootView);
  RNOH_ASSERT(arkTSMessageHub != nullptr);
  m_touchEventHandler = std::make_shared<SurfaceTouchEventHandler>(
      m_rootView, std::move(arkTSMessageHub), rnInstanceId);
}

ArkUISurface::ArkUISurface(ArkUISurface&& other) noexcept
    : m_surfaceId(other.m_surfaceId),
      m_scheduler(std::move(other.m_scheduler)),
      m_taskExecutor(std::move(other.m_taskExecutor)),
      m_nodeContentHandle(std::move(other.m_nodeContentHandle)),
      m_rootView(std::move(other.m_rootView)),
      m_componentInstanceRegistry(std::move(other.m_componentInstanceRegistry)),
      m_surfaceHandler(std::move(other.m_surfaceHandler)),
      m_touchEventHandler(std::move(other.m_touchEventHandler)),
      m_doLeftAndRightSwapInRTL(other.m_doLeftAndRightSwapInRTL) {
  m_threadGuard.assertThread();
  other.m_nodeContentHandle.reset();
}

ArkUISurface& ArkUISurface::operator=(ArkUISurface&& other) noexcept {
  m_threadGuard.assertThread();
  std::swap(m_taskExecutor, other.m_taskExecutor);
  std::swap(m_surfaceId, other.m_surfaceId);
  std::swap(m_scheduler, other.m_scheduler);
  std::swap(m_nodeContentHandle, other.m_nodeContentHandle);
  std::swap(m_rootView, other.m_rootView);
  std::swap(m_componentInstanceRegistry, other.m_componentInstanceRegistry);
  std::swap(m_surfaceHandler, other.m_surfaceHandler);
  std::swap(m_touchEventHandler, other.m_touchEventHandler);
  std::swap(m_doLeftAndRightSwapInRTL, other.m_doLeftAndRightSwapInRTL);
  return *this;
}

ArkUISurface::~ArkUISurface() noexcept {
  m_threadGuard.assertThread();
  if (m_componentInstanceRegistry != nullptr && m_rootView != nullptr) {
    // NOTE: we don't detach the view from NodeContent here,
    // since the Surface must already be `.stop()`ed before it's `unregistered`
    m_componentInstanceRegistry->deleteByTag(m_rootView->getTag());
  }
}

void ArkUISurface::attachToNodeContent(NodeContentHandle nodeContentHandle) {
  m_threadGuard.assertThread();
  if (m_nodeContentHandle.has_value()) {
    throw RNOHError(
        "ArkUISurface: calling attachToNodeContent on an already attached Surface");
  }
  m_nodeContentHandle = nodeContentHandle;
  m_nodeContentHandle.value().addNode(m_rootView->getLocalRootArkUINode());
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::CONTENT_APPEARED, m_rnInstanceId);
}

void ArkUISurface::detachFromNodeContent() {
  if (!m_nodeContentHandle.has_value()) {
    throw RNOHError(
        "ArkUISurface: calling detachFromNodeContent on an already detached Surface");
  }
  m_nodeContentHandle.value().removeNode(m_rootView->getLocalRootArkUINode());
  m_nodeContentHandle.reset();
}

void ArkUISurface::updateConstraints(
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    bool doLeftAndRightSwapInRTL) {
  updateConstraints(
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      1.0f,
      isRTL,
      doLeftAndRightSwapInRTL);
}

void ArkUISurface::updateConstraints(
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    float fontSizeMultiplier,
    bool isRTL,
    bool doLeftAndRightSwapInRTL) {
  m_threadGuard.assertThread();
  m_doLeftAndRightSwapInRTL = doLeftAndRightSwapInRTL;
  auto layoutConstraints = m_surfaceHandler->getLayoutConstraints();
  layoutConstraints.layoutDirection = isRTL
      ? facebook::react::LayoutDirection::RightToLeft
      : facebook::react::LayoutDirection::LeftToRight;

  layoutConstraints.maximumSize = {.width = maxWidth, .height = maxHeight};
  layoutConstraints.minimumSize = {.width = minWidth, .height = minHeight};
  auto layoutContext = m_surfaceHandler->getLayoutContext();
  layoutContext.viewportOffset = {viewportOffsetX, viewportOffsetY};
  layoutContext.pointScaleFactor = pixelRatio;
  layoutContext.fontSizeMultiplier = fontSizeMultiplier;
  layoutContext.swapLeftAndRightInRTL = isRTL && m_doLeftAndRightSwapInRTL;
  m_surfaceHandler->constraintLayout(layoutConstraints, layoutContext);
}

facebook::react::Size ArkUISurface::measure(
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    bool doLeftAndRightSwapInRTL) {
  return measure(
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      1.0f,
      isRTL,
      doLeftAndRightSwapInRTL);
}

facebook::react::Size ArkUISurface::measure(
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    float fontSizeMultiplier,
    bool isRTL,
    bool doLeftAndRightSwapInRTL) {
  m_threadGuard.assertThread();
  m_doLeftAndRightSwapInRTL = doLeftAndRightSwapInRTL;
  auto layoutConstraints = m_surfaceHandler->getLayoutConstraints();
  layoutConstraints.layoutDirection = isRTL
      ? facebook::react::LayoutDirection::RightToLeft
      : facebook::react::LayoutDirection::LeftToRight;

  layoutConstraints.maximumSize = {.width = maxWidth, .height = maxHeight};
  layoutConstraints.minimumSize = {.width = minWidth, .height = minHeight};
  auto layoutContext = m_surfaceHandler->getLayoutContext();
  layoutContext.viewportOffset = {viewportOffsetX, viewportOffsetY};
  layoutContext.pointScaleFactor = pixelRatio;
  layoutContext.fontSizeMultiplier = fontSizeMultiplier;
  layoutContext.swapLeftAndRightInRTL = isRTL && m_doLeftAndRightSwapInRTL;
  return m_surfaceHandler->measure(layoutConstraints, layoutContext);
}

void ArkUISurface::start(
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    bool doLeftAndRightSwapInRTL,
    folly::dynamic const& initialProps,
    std::shared_ptr<facebook::react::LayoutAnimationDriver> const&
        animationDriver) {
  start(
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      1.0f,
      isRTL,
      doLeftAndRightSwapInRTL,
      initialProps,
      animationDriver);
}

void ArkUISurface::start(
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    float fontSizeMultiplier,
    bool isRTL,
    bool doLeftAndRightSwapInRTL,
    folly::dynamic const& initialProps,
    std::shared_ptr<facebook::react::LayoutAnimationDriver> const&
        animationDriver) {
  m_threadGuard.assertThread();
  folly::dynamic m_initialProps = initialProps;
  m_initialProps["batchToNative"] = IsParallelizationEnabled();
  this->setProps(m_initialProps);
  this->updateConstraints(
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      fontSizeMultiplier,
      isRTL,
      doLeftAndRightSwapInRTL);
  m_surfaceHandler->start();
  auto mountingCoordinator = m_surfaceHandler->getMountingCoordinator();
  mountingCoordinator->setMountingOverrideDelegate(animationDriver);
}

void ArkUISurface::setProps(folly::dynamic const& props) {
  m_threadGuard.assertThread();
  m_surfaceHandler->setProps(props);
}

void ArkUISurface::stop(std::function<void()> onStop) {
  m_threadGuard.assertThread();
  m_taskExecutor->runTask(
      TaskThread::JS,
      [weakSelf = weak_from_this(),
       taskExecutor = m_taskExecutor,
       onStop = std::move(onStop)]() mutable {
        // mark as mutable, so that params are not treated as const, otherwise
        // subsequent std::move(onStop) will call copy ctor
        auto self = weakSelf.lock();
        if (self != nullptr &&
            self->m_surfaceHandler->getStatus() ==
                SurfaceHandler::Status::Running) {
          self->m_surfaceHandler->stop();
        }
        taskExecutor->runTask(
            TaskThread::MAIN,
            [onStop = std::move(onStop),
             // moving self here releases ArkUISurface on the main thread
             self = std::move(self)

        ] { onStop(); });
      });
}

void ArkUISurface::stopSynchronously() {
  m_threadGuard.assertThread();
  if (m_surfaceHandler->getStatus() == SurfaceHandler::Status::Running) {
    m_surfaceHandler->stop();
  }
}

void ArkUISurface::setDisplayMode(facebook::react::DisplayMode displayMode) {
  m_threadGuard.assertThread();
  m_surfaceHandler->setDisplayMode(displayMode);
}

void ArkUISurface::setDisplayMetrics(
    float pixelRatio,
    float fontSizeMultiplier) {
  m_threadGuard.assertThread();
  auto layoutConstraints = m_surfaceHandler->getLayoutConstraints();
  auto layoutContext = m_surfaceHandler->getLayoutContext();
  layoutContext.pointScaleFactor = pixelRatio;
  layoutContext.fontSizeMultiplier = fontSizeMultiplier;
  layoutContext.swapLeftAndRightInRTL =
      layoutConstraints.layoutDirection ==
          facebook::react::LayoutDirection::RightToLeft &&
      m_doLeftAndRightSwapInRTL;
  m_surfaceHandler->constraintLayout(layoutConstraints, layoutContext);
}

Surface::LayoutContext ArkUISurface::getLayoutContext() {
  m_threadGuard.assertThread();
  return Surface::LayoutContext::from(m_surfaceHandler->getLayoutContext());
}

DisplayMetrics ArkUISurface::getDisplayMetrics() {
  DisplayMetrics result;

  m_taskExecutor->runSyncTask(TaskThread::MAIN, [this, &result] {
    result = m_displayMetricsManager->getDisplayMetrics();
  });
  return result;
}

std::weak_ptr<UIInputEventHandler> ArkUISurface::getUIInputEventHandler() {
  return m_touchEventHandler;
};

} // namespace rnoh
