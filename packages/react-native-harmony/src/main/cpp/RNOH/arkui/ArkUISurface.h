/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <react/renderer/scheduler/SurfaceHandler.h>
#include "RNOH/ComponentInstance.h"
#include "RNOH/ComponentInstanceFactory.h"
#include "RNOH/ComponentInstanceRegistry.h"
#include "RNOH/ThreadGuard.h"
#include "RNOH/arkui/NodeContentHandle.h"
#include "RNOH/arkui/UIInputEventHandler.h"

namespace rnoh {

/**
 * @internal
 * @thread: MAIN
 * Wraps the `react::SurfaceHandle` and attaches the root component of the
 * React Native `Surface` to the ArkUI native component.
 */
class ArkUISurface : public Surface,
                     public std::enable_shared_from_this<ArkUISurface> {
 public:
  using Shared = std::shared_ptr<ArkUISurface>;

  ArkUISurface(
      TaskExecutor::Shared taskExecutor,
      std::shared_ptr<facebook::react::Scheduler> scheduler,
      ComponentInstanceRegistry::Shared componentInstanceRegistry,
      ComponentInstanceFactory::Shared const& componentInstanceFactory,
      ArkTSMessageHub::Shared arkTSMessageHub,
      DisplayMetricsManager::Shared displayMetricsManager,
      facebook::react::SurfaceId surfaceId,
      size_t rnInstanceId,
      std::string const& appKey);

  ArkUISurface(ArkUISurface const& other) = delete;
  ArkUISurface& operator=(ArkUISurface const& other) = delete;

  ArkUISurface(ArkUISurface&& other) noexcept;
  ArkUISurface& operator=(ArkUISurface&& other) noexcept;

  ~ArkUISurface() noexcept;

  void attachToNodeContent(NodeContentHandle nodeContentHandle);
  void detachFromNodeContent();

  facebook::react::Size measure(
      float minWidth,
      float minHeight,
      float maxWidth,
      float maxHeight,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      bool isRTL,
      bool doLeftAndRightSwapInRTL);

  facebook::react::Size measure(
      float minWidth,
      float minHeight,
      float maxWidth,
      float maxHeight,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      float fontSizeMultiplier,
      bool isRTL,
      bool doLeftAndRightSwapInRTL);

  void updateConstraints(
      float minWidth,
      float minHeight,
      float maxWidth,
      float maxHeight,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      bool isRTL,
      bool doLeftAndRightSwapInRTL);

  void updateConstraints(
      float minWidth,
      float minHeight,
      float maxWidth,
      float maxHeight,
      float viewportOffsetX,
      float viewportOffsetY,
      float pixelRatio,
      float fontSizeMultiplier,
      bool isRTL,
      bool doLeftAndRightSwapInRTL);
  void start(
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
          animationDriver);

  void start(
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
          animationDriver);
  void setProps(folly::dynamic const& props) override;
  void stop(std::function<void()> onStop);
  void stopSynchronously();
  void setDisplayMode(facebook::react::DisplayMode displayMode);
  void setDisplayMetrics(float pixelRatio, float fontSizeMultiplier);
  Surface::LayoutContext getLayoutContext() override;
  DisplayMetrics getDisplayMetrics() override;
  std::weak_ptr<UIInputEventHandler> getUIInputEventHandler() override;
  const facebook::react::SurfaceHandler& getSurfaceHandler() const {
    return *m_surfaceHandler;
  }

 private:
  facebook::react::SurfaceId m_surfaceId;
  size_t m_rnInstanceId;
  std::shared_ptr<facebook::react::Scheduler> m_scheduler;
  std::optional<NodeContentHandle> m_nodeContentHandle;
  ComponentInstance::Shared m_rootView = nullptr;
  ComponentInstanceRegistry::Shared m_componentInstanceRegistry;
  std::unique_ptr<facebook::react::SurfaceHandler> m_surfaceHandler;
  ThreadGuard m_threadGuard{};
  std::shared_ptr<UIInputEventHandler> m_touchEventHandler;
  TaskExecutor::Shared m_taskExecutor;
  DisplayMetricsManager::Shared m_displayMetricsManager;
  bool m_doLeftAndRightSwapInRTL{true};
};

} // namespace rnoh
