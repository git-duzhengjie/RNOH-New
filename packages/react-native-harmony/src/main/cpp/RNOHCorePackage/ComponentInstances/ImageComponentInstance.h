/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/image/ImageEventEmitter.h>
#include <react/renderer/components/image/ImageShadowNode.h>
#include <cstdint>
#include <memory>
#include "RNOH/CppComponentInstance.h"
#ifdef IMAGE_CACHE_ENABLED
#include "RNOH/ImageCache/include/ImageCacheData.h"
#include "RNOH/ImageCache/include/ImageCacheManager.h"
#include "RNOH/ImageCache/include/ImageCacheOption.h"
#endif
#include "RNOH/ImageSourceResolver.h"
#include "RNOH/arkui/ImageNode.h"

namespace rnoh {
class ImageComponentInstance
    : public CppComponentInstance<facebook::react::ImageShadowNode>,
      public ImageNodeDelegate,
      public ImageSourceResolver::ImageSourceUpdateListener {
 private:
  using ValueFactory = facebook::react::ValueFactory;
  using EventEmitter = facebook::react::EventEmitter;

#ifdef IMAGE_CACHE_ENABLED
  std::atomic<ImageCachePro::ImageCacheRequestHandle> m_activeRequestHandle{
      ImageCachePro::INVALID_REQUEST_HANDLE};
  std::atomic<ImageCachePro::ImageCacheAnimHandle> m_animHandle{
      ImageCachePro::INVALID_REQUEST_HANDLE};
#endif

  ImageNode m_imageNode;
  struct ImageRawProps {
    std::optional<std::string> resizeMethod;
    std::optional<bool> focusable;
    std::optional<std::string> alt;
    std::optional<std::string> loadingIndicatorSource;
    std::optional<int32_t> fadeDuration;
    static ImageRawProps getFromDynamic(folly::dynamic value);
  };
  ImageRawProps m_rawProps;
  // This field keeps track of the original URI, ignoring caches
  facebook::react::ImageSource m_imageSource;

  void setSources(facebook::react::ImageSources const& sources);
  void invalidatePendingCacheRequest();

#ifdef IMAGE_CACHE_ENABLED
  uint64_t m_imageLoadGeneration = 0;
  uint64_t m_loadSetp = 0;
  bool m_pendingStateUpdate = false;
  bool m_isUserImageCache = false;
  std::shared_ptr<ImageCachePro::ImageCacheOption> buildImageCacheOption(
      facebook::react::Float blurRadius);
  bool applyImageCacheData(std::shared_ptr<ImageCachePro::ImageCacheData> data);
  bool startAnimationPlay(
      const ImageCachePro::ImageCacheLoadData& data,
      std::shared_ptr<ImageCachePro::ImageCacheOption> option);
  void fallbackToUriSource(
      facebook::react::ImageSources const& sources,
      facebook::react::Float blurRadius);
  void loadImageWithCache(
      facebook::react::ImageSources const& sources,
      facebook::react::Float blurRadius);
  void cancelActiveImageCacheRequest();
  ImageCachePro::ImageCacheManager::Shared getImageCacheManager();
#endif

  std::string getBundlePath();
  std::string getHspModuleName();
  std::string getAssetsPrefix();

  enum class EventType {
    ON_LOAD_START,
    ON_LOAD,
    ON_LOAD_END,
    ON_PROGRESS,
    ON_ERROR,
    ON_PARTIAL_LOAD,
  };
  using EventType = ImageComponentInstance::EventType;
  struct ScheduledEvent {
    EventType eventType;
    const ValueFactory payloadFactory;
  };
  std::vector<ScheduledEvent> m_scheduledEvents;
  void dispatchOrScheduleEvent(
      EventType eventType,
      const ValueFactory& payloadFactory =
          EventEmitter::defaultPayloadFactory());
  void dispatchEvent(EventType eventType, const ValueFactory& payloadFactory);

 public:
  ImageComponentInstance(Context context);
  ~ImageComponentInstance() override;
  void onPropsChanged(SharedConcreteProps const& props) override;
  void onStateChanged(SharedConcreteState const& state) override;
  void onEventEmitterChanged(
      SharedConcreteEventEmitter const& eventEmitter) override;

  void onProgress(uint32_t loaded, uint32_t total) override;
  void onComplete(float width, float height) override;
  void onError(int32_t errorCode) override;

  ImageNode& getLocalRootArkUINode() override;

  // ImageSourceResolver::ImageSourceUpdateListener
  void onImageSourceCacheUpdate() override;
};
} // namespace rnoh
