/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNInstanceCAPI.h"

#include <react/renderer/componentregistry/native/NativeComponentRegistryBinding.h>
#include <react/renderer/runtimescheduler/RuntimeSchedulerCallInvoker.h>
#include <react/runtime/nativeviewconfig/LegacyUIManagerConstantsProviderBinding.h>
#include <future>
#include <memory>
#include "NativeLogger.h"
#include "Performance/NativeTracing.h"
#include "RNInstance.h"
#include "RNOH/Assert.h"
#include "RNOH/ParallelCheck.h"
#include "RNOH/Performance/RNOHMarker.h"
#include "RNOHError.h"
#include "TaskExecutor/TaskExecutor.h"
#include "TextMeasurer.h"
#include "RNOH/UpdatePropsManager.h"

using namespace facebook;
namespace rnoh {

RNInstanceCAPI::~RNInstanceCAPI() noexcept {
  DLOG(INFO) << "~RNInstanceCAPI::start";
  if (m_unsubscribeUITickListener != nullptr) {
    m_unsubscribeUITickListener();
  }
  m_taskExecutor->runSyncTask(
      TaskThread::MAIN,
      [surfaces =
           std::make_shared<decltype(m_surfaceById)>(std::move(m_surfaceById)),
       scheduler = m_scheduler,
       mountingManager = std::move(m_mountingManager),
       componentInstanceRegistry = std::move(m_componentInstanceRegistry),
       componentInstanceFactory = std::move(m_componentInstanceFactory)] {
        DLOG(INFO) << "~RNInstanceCAPI::MAIN - stopping all surfaces";
        for (auto& [surfaceId, surface] : *surfaces) {
          auto arkUISurface = std::dynamic_pointer_cast<ArkUISurface>(surface);
          if (arkUISurface != nullptr) {
            arkUISurface->stopSynchronously();
            scheduler->unregisterSurface(arkUISurface->getSurfaceHandler());
          }
        }
        if (!surfaces->empty()) {
          LOG(WARNING)
              << "Not all React Native surfaces were destroyed before the Instance they are running in.";
        }
      });
  DLOG(INFO) << "~RNInstanceCAPI::stop";
}

size_t RNInstanceCAPI::getId() {
  return m_id;
}

void RNInstanceCAPI::synchronouslyUpdateViewOnUIThread(
    facebook::react::Tag tag,
    folly::dynamic props) {
  facebook::react::TraceSection s(
      "#RNOH::RNInstanceCAPI::synchronouslyUpdateViewOnUIThread");
  VLOG(2) << "RNInstanceCAPI::synchronouslyUpdateViewOnUIThread";
  RNOH_ASSERT(m_taskExecutor->getCurrentTaskThread() == TaskThread::MAIN);

  auto componentInstance = m_componentInstanceRegistry->findByTag(tag);
  if (componentInstance == nullptr) {
    return;
  }

  auto componentHandle = componentInstance->getComponentHandle();
  auto componentDescriptor =
      m_scheduler->findComponentDescriptorByHandle_DO_NOT_USE_THIS_IS_BROKEN(
          componentHandle);
  if (componentDescriptor == nullptr) {
    LOG(ERROR)
        << "RNInstanceCAPI::synchronouslyUpdateViewOnUIThread: could not find componentDescriptor for tag: "
        << tag;
    return;
  }

  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_START, tag);
  m_mountingManager->updateView(tag, std::move(props), *componentDescriptor);
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_END, tag);
}

void RNInstanceCAPI::attachRootView(
    NodeContentHandle nodeContentHandle,
    facebook::react::Tag surfaceId) {
  DLOG(INFO) << "RNInstanceCAPI::registerNodeContentHandle";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    LOG(ERROR) << "Surface with id: " << surfaceId << " not found";
    return;
  }
  it->second->attachToNodeContent(std::move(nodeContentHandle));
}

void RNInstanceCAPI::detachRootView(facebook::react::Tag surfaceId) {
  DLOG(INFO) << "RNInstanceCAPI::registerNodeContentHandle";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    LOG(ERROR) << "Surface with id: " << surfaceId << " not found";
    return;
  }
  it->second->detachFromNodeContent();
}

void RNInstanceCAPI::createSurface(
    facebook::react::Tag surfaceId,
    std::string const& moduleName) {
  DLOG(INFO) << "RNInstanceCAPI::createSurface";
  m_surfaceById.emplace(
      surfaceId,
      std::make_shared<ArkUISurface>(
          m_taskExecutor,
          m_scheduler,
          m_componentInstanceRegistry,
          m_componentInstanceFactory,
          m_arkTSMessageHub,
          m_arkTSBridge,
          surfaceId,
          m_id,
          moduleName));
}

void RNInstanceCAPI::updateSurfaceConstraints(
    facebook::react::Tag surfaceId,
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    bool doLeftAndRightSwapInRTL) {
  RNInstanceCAPI::updateSurfaceConstraints(
      surfaceId,
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

void RNInstanceCAPI::updateSurfaceConstraints(
    facebook::react::Tag surfaceId,
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
  DLOG(INFO) << "RNInstanceCAPI::updateSurfaceConstraints";
  facebook::react::TraceSection s(
      "#RNOH::RNInstanceCAPI::updateSurfaceConstraints");
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second->updateConstraints(
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
}

facebook::react::Size RNInstanceCAPI::measureSurface(
    facebook::react::Tag surfaceId,
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    bool doLeftAndRightSwapInRTL) {
  return RNInstanceCAPI::measureSurface(
      surfaceId,
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

facebook::react::Size RNInstanceCAPI::measureSurface(
    facebook::react::Tag surfaceId,
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
  DLOG(INFO) << "RNInstanceCAPI::measureSurface";
  facebook::react::TraceSection s("#RNOH::RNInstanceCAPI::measureSurface");
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    throw FatalRNOHError("Invalid surfaceId");
  }
  return it->second->measure(
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
}

void RNInstanceCAPI::startSurface(
    facebook::react::Tag surfaceId,
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    bool isRTL,
    folly::dynamic&& initialProps,
    bool doLeftAndRightSwapInRTL) {
  RNInstanceCAPI::startSurface(
      surfaceId,
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      1.0f,
      isRTL,
      std::move(initialProps),
      doLeftAndRightSwapInRTL);
}
void RNInstanceCAPI::startSurface(
    facebook::react::Tag surfaceId,
    float minWidth,
    float minHeight,
    float maxWidth,
    float maxHeight,
    float viewportOffsetX,
    float viewportOffsetY,
    float pixelRatio,
    float fontSizeMultiplier,
    bool isRTL,
    folly::dynamic&& initialProps,
    bool doLeftAndRightSwapInRTL) {
  DLOG(INFO) << "RNInstanceCAPI::startSurface";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second->start(
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      viewportOffsetX,
      viewportOffsetY,
      pixelRatio,
      fontSizeMultiplier,
      isRTL,
      doLeftAndRightSwapInRTL,
      std::move(initialProps),
      m_animationDriver);
}

void RNInstanceCAPI::setSurfaceProps(
    facebook::react::Tag surfaceId,
    folly::dynamic&& props) {
  DLOG(INFO) << "RNInstanceCAPI::setSurfaceProps";
  facebook::react::TraceSection s("#RNOH::RNInstanceCAPI::setSurfaceProps");
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second->setProps(std::move(props));
}

void RNInstanceCAPI::stopSurface(
    facebook::react::Tag surfaceId,
    std::function<void()> onStop) {
  DLOG(INFO) << "RNInstanceCAPI::stopSurface";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second->stop(std::move(onStop));
}

void RNInstanceCAPI::destroySurface(facebook::react::Tag surfaceId) {
  DLOG(INFO) << "RNInstanceCAPI::destroySurface";
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  auto surface = it->second;
  auto arkUISurface = std::dynamic_pointer_cast<ArkUISurface>(surface);
  if (arkUISurface != nullptr) {
    m_scheduler->unregisterSurface(arkUISurface->getSurfaceHandler());
  }

  m_surfaceById.erase(it);
}

void RNInstanceCAPI::setSurfaceDisplayMode(
    facebook::react::Tag surfaceId,
    facebook::react::DisplayMode displayMode) {
  DLOG(INFO) << "RNInstanceCAPI::setSurfaceDisplayMode";
  facebook::react::TraceSection s(
      "#RNOH::RNInstanceCAPI::setSurfaceDisplayMode");
  auto it = m_surfaceById.find(surfaceId);
  if (it == m_surfaceById.end()) {
    return;
  }
  it->second->setDisplayMode(displayMode);
}

void RNInstanceCAPI::onConfigurationChange(folly::dynamic const& payload) {
  if (payload.isNull()) {
    return;
  }

  if (!payload.count("windowPhysicalPixels") ||
      !payload.count("screenPhysicalPixels")) {
    return;
  }

  auto windowPhysicalPixels =
      PhysicalPixels::fromDynamic(payload["windowPhysicalPixels"]);
  if (IsParallelizationEnabled()) {
    static double s_lastWindowWidth = -1.0;
    static double s_lastWindowHeight = -1.0;
    if (windowPhysicalPixels.width && windowPhysicalPixels.height) {
      double width = windowPhysicalPixels.width;
      double height = windowPhysicalPixels.height;
      if (width > 0.0 && height > 0.0) {
        const bool hasLast =
            s_lastWindowWidth > 0.0 && s_lastWindowHeight > 0.0;
        const bool sizeChanged = hasLast &&
            (width != s_lastWindowWidth || height != s_lastWindowHeight);
        s_lastWindowWidth = width;
        s_lastWindowHeight = height;
        if (sizeChanged) {
          SchedulerDelegate::markConfigurationChange();
        }
      }
    }
  }

  // Use windowPhysicalPixels for scale/fontScale to stay consistent with
  // RNInstanceInternal::initialize() (which seeds TextMeasurer from the window
  // metrics). Mixing window (init) and screen (config-change) sources made
  // TextMeasurer and Surface disagree in split-screen / multi-window where the
  // two pixel sets differ.
  for (const auto& [_, surface] : m_surfaceById) {
    surface->setDisplayMetrics(
        windowPhysicalPixels.scale, windowPhysicalPixels.fontScale);
  }

  auto textMeasurer =
      m_contextContainer->at<std::shared_ptr<rnoh::TextMeasurer>>(
          "textLayoutManagerDelegate");
  if (!textMeasurer) {
    return;
  }
  textMeasurer->setParams(
      windowPhysicalPixels.scale, windowPhysicalPixels.fontScale);
  m_fontRegistry->updateThemeFont();
  const double scaleRatioDpiX =
      windowPhysicalPixels.scale / windowPhysicalPixels.xDpi;
  const double scaleRatioDpiY =
      windowPhysicalPixels.scale / windowPhysicalPixels.yDpi;
  m_arkTSBridge->setScaleRatioDpi(scaleRatioDpiX, scaleRatioDpiY);
}

ComponentInstance::Shared RNInstanceCAPI::findComponentInstanceByTag(
    facebook::react::Tag tag) {
  return m_componentInstanceRegistry->findByTag(tag);
}

std::optional<facebook::react::Tag>
RNInstanceCAPI::findComponentInstanceTagById(const std::string& id) {
  return m_componentInstanceRegistry->findTagById(id);
}

std::optional<std::string> RNInstanceCAPI::getNativeNodeIdByTag(
    facebook::react::Tag tag) const {
  auto componentInstance = m_componentInstanceRegistry->findByTag(tag);
  if (componentInstance == nullptr) {
    return std::nullopt;
  }
  return componentInstance->getLocalRootArkUINode().getId();
}

std::shared_ptr<TurboModuleProvider>
RNInstanceCAPI::createTurboModuleProvider() {
  DLOG(INFO) << "RNInstanceCAPI::createTurboModuleProvider";
  auto turboModuleProvider = std::make_shared<TurboModuleProvider>(
      std::make_shared<facebook::react::RuntimeSchedulerCallInvoker>(
          m_runtimeScheduler),
      std::move(m_turboModuleFactory),
      m_eventDispatcher,
      std::move(m_jsQueue),
      m_weakSelf);
  turboModuleProvider->installJSBindings(
      m_reactInstance->getUnbufferedRuntimeExecutor());
  return turboModuleProvider;
}

std::optional<Surface::Weak> RNInstanceCAPI::getSurfaceByRootTag(
    facebook::react::Tag rootTag) {
  auto it = m_surfaceById.find(rootTag);
  if (it == m_surfaceById.end()) {
    return std::nullopt;
  }
  return it->second;
};

void RNInstanceCAPI::installJSBindings(facebook::jsi::Runtime& rt) {
  // install `console.log` (etc.) implementation
  react::bindNativeLogger(rt, nativeLogger);
  // install tracing functions
  rnoh::setupTracing(rt);
  facebook::react::bindHasComponentProvider(
      rt, [this](std::string const& name) {
        return m_componentJSIBinderByName.contains(name);
      });

  facebook::react::LegacyUIManagerConstantsProviderBinding::install(
      rt,
      "getConstantsForViewManager",
      [this](auto& rt, auto const& name) -> jsi::Value {
        auto componentJSIBinder = m_componentJSIBinderByName.find(name);
        if (componentJSIBinder != m_componentJSIBinderByName.end()) {
          return componentJSIBinder->second->createBindings(rt);
        }
        LOG(ERROR) << "Couldn't find ComponentJSIBinder for: " << name;
        return nullptr;
      });

  facebook::react::LegacyUIManagerConstantsProviderBinding::install(
      rt, "getConstants", [](auto& rt) {
        return facebook::jsi::Value(facebook::jsi::Object(rt));
      });

  // RNOH 0.86 无 reanimated 原生模块（UpdatePropsManager 缺失），
  // 安装 UpdatePropsManager 到 global，供 reanimated 的 updateProps 调用。
  // 需要 MountingManager。
  UpdatePropsManager::install(
      rt,
      m_mountingManager.get());
}

} // namespace rnoh
