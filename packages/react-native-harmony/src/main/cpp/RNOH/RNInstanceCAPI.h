/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "FontRegistry.h"
#include "RNOH/ArkTSBridge.h"
#include "RNOH/ArkTSMessageHandler.h"
#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/ComponentInstanceFactory.h"
#include "RNOH/ComponentInstancePreallocationRequestQueue.h"
#include "RNOH/ComponentInstanceRegistry.h"
#include "RNOH/RNInstanceInternal.h"
#include "RNOH/arkui/ArkUISurface.h"
#include "RNOH/arkui/NodeContentHandle.h"
#include "UIManagerModule.h"

namespace rnoh {
using MutationsListener = std::function<void(
    MutationsToNapiConverter const&,
    facebook::react::ShadowViewMutationList const& mutations)>;

class RNInstanceCAPI final : public RNInstanceInternal {
 public:
  RNInstanceCAPI(
      size_t id,
      std::shared_ptr<const facebook::react::ContextContainer> contextContainer,
      TurboModuleFactory&& turboModuleFactory,
      TaskExecutor::Shared taskExecutor,
      std::shared_ptr<facebook::react::ComponentDescriptorProviderRegistry>
          componentDescriptorProviderRegistry,
      MutationsToNapiConverter::Shared mutationsToNapiConverter,
      EventEmitRequestHandlers eventEmitRequestHandlers,
      GlobalJSIBinders globalJSIBinders,
      UITicker::Shared uiTicker,
      ShadowViewRegistry::Shared shadowViewRegistry,
      MountingManager::Shared mountingManager,
      std::vector<ArkTSMessageHandler::Shared> arkTSMessageHandlers,
      ArkTSChannel::Shared arkTSChannel,
      ArkTSMessageHub::Shared arkTSMessageHub,
      ComponentInstanceRegistry::Shared componentInstanceRegistry,
      ComponentInstanceFactory::Shared componentInstanceFactory,
      ComponentInstancePreallocationRequestQueue::Shared
          componentInstancePreallocationRequestQueue,
      SharedNativeResourceManager nativeResourceManager,
      bool shouldEnableDebugger,
      ArkTSBridge::Shared arkTSBridge,
      FontRegistry::Shared FontRegistry,
      RNOHMarker::RNOHMarkerListener::Shared markerListener,
      ComponentJSIBinderByString componentJSIBinderByName,
      std::shared_ptr<facebook::react::JSRuntimeFactory> jsEngineProvider,
      std::shared_ptr<InspectorHostTarget> inspectorHostTarget,
      std::string hspModuleName,
      std::string cacheDir)
      : RNInstanceInternal(
            id,
            contextContainer,
            std::move(turboModuleFactory),
            std::move(taskExecutor),
            std::move(componentDescriptorProviderRegistry),
            std::move(mutationsToNapiConverter),
            std::move(eventEmitRequestHandlers),
            std::move(globalJSIBinders),
            std::move(uiTicker),
            std::move(shadowViewRegistry),
            std::move(arkTSChannel),
            std::move(mountingManager),
            std::move(arkTSMessageHandlers),
            std::move(componentInstancePreallocationRequestQueue),
            std::move(nativeResourceManager),
            shouldEnableDebugger,
            std::move(arkTSBridge),
            std::move(FontRegistry),
            std::move(jsEngineProvider),
            std::move(inspectorHostTarget),
            arkTSMessageHub,
            hspModuleName),
        m_componentInstanceRegistry(std::move(componentInstanceRegistry)),
        m_componentInstanceFactory(std::move(componentInstanceFactory)),
        m_componentJSIBinderByName(std::move(componentJSIBinderByName)),
        m_markerListener(std::move(markerListener)),
        m_cacheDir(cacheDir) {}

  ~RNInstanceCAPI() noexcept override;

  size_t getId() override;

  void createSurface(
      facebook::react::Tag surfaceId,
      std::string const& moduleName) override;
  void updateSurfaceConstraints(
      facebook::react::Tag surfaceId,
      float minWidth,
      float minHeight,
      float maxWidth,
      float maxHeight,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      bool isRTL,
      bool doLeftAndRightSwapInRTL = true) override;

  void updateSurfaceConstraints(
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
      bool doLeftAndRightSwapInRTL = true) override;

  facebook::react::Size measureSurface(
      facebook::react::Tag surfaceId,
      float minWidth,
      float minHeight,
      float maxWidth,
      float maxHeight,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      bool isRTL,
      bool doLeftAndRightSwapInRTL = true) override;

  facebook::react::Size measureSurface(
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
      bool doLeftAndRightSwapInRTL = true) override;

  void startSurface(
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
      bool doLeftAndRightSwapInRTL = true) override;

  void startSurface(
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
      bool doLeftAndRightSwapInRTL = true) override;
  void setSurfaceProps(facebook::react::Tag surfaceId, folly::dynamic&& props)
      override;
  void stopSurface(facebook::react::Tag surfaceId, std::function<void()> onStop)
      override;
  void destroySurface(facebook::react::Tag surfaceId) override;
  void setSurfaceDisplayMode(
      facebook::react::Tag surfaceId,
      facebook::react::DisplayMode displayMode) override;

  void onConfigurationChange(folly::dynamic const& payload) override;

  void synchronouslyUpdateViewOnUIThread(
      facebook::react::Tag tag,
      folly::dynamic props) override;

  void attachRootView(
      NodeContentHandle nodeContentHandle,
      facebook::react::Tag surfaceId);

  void detachRootView(facebook::react::Tag surfaceId);

  ComponentInstance::Shared findComponentInstanceByTag(
      facebook::react::Tag tag);

  std::optional<facebook::react::Tag> findComponentInstanceTagById(
      const std::string& id);

  std::optional<std::string> getNativeNodeIdByTag(
      facebook::react::Tag tag) const;

  std::string getCacheDir() override {
    return m_cacheDir;
  }

  std::optional<Surface::Weak> getSurfaceByRootTag(
      facebook::react::Tag rootTag) override;

  size_t getComponentInstancesCount() const {
    if (m_componentInstanceRegistry) {
      return m_componentInstanceRegistry->getComponentsCount();
    }
    return 0;
  }

  void setArkUINodeContext(const ArkUINode::Context::Shared& arkUINodeContext) {
    if (m_componentInstanceFactory) {
      m_componentInstanceFactory->setArkUINodeContext(arkUINodeContext);
    } else {
      DLOG(ERROR)
          << "RNInstanceCAPI::setArkUINodeContext: m_componentInstanceFactory is null";
    }
  }

 protected:
  std::shared_ptr<TurboModuleProvider> createTurboModuleProvider() override;

  void installJSBindings(facebook::jsi::Runtime& rt) override;

  std::unordered_map<facebook::react::SurfaceId, ArkUISurface::Shared>
      m_surfaceById;
  ComponentInstanceRegistry::Shared m_componentInstanceRegistry;
  ComponentInstanceFactory::Shared m_componentInstanceFactory;
  ComponentJSIBinderByString m_componentJSIBinderByName;
  RNOHMarker::RNOHMarkerListener::Shared m_markerListener;
  std::string m_cacheDir;
};

} // namespace rnoh
