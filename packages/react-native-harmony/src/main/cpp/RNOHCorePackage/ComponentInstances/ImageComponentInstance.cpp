/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageComponentInstance.h"
#include <RNOH/RNInstanceInternal.h>
#include <react/renderer/components/image/ImageProps.h>
#include <react/renderer/components/image/ImageState.h>
#include <react/renderer/core/ConcreteState.h>
#include <string_view>
#include <arkui/native_animate.h>
#include <arkui/native_interface.h>
#ifdef IMAGE_CACHE_ENABLED
#include "RNOH/ImageCache/include/ImageCacheData.h"
#include "RNOH/ImageCache/include/ImageCacheManager.h"
#include "RNOH/ImageCache/include/ImageCacheOption.h"
#endif

#include "RNOH/ApiVersionCheck.h"
#include "RNOH/ArkTSTurboModule.h"
#include "RNOH/ImageCacheCheck.h"

namespace rnoh {

using namespace std::literals;
constexpr std::string_view ASSET_PREFIX = "asset://"sv;
constexpr std::string_view RAWFILE_PREFIX = "resource://RAWFILE/assets/";
constexpr std::string_view FILE_PREFIX = "file://";
const std::string BASE_64_PREFIX = "data:";
const std::string BASE_64_MARK = ";base64,";
const std::int32_t BASE_64_FORMAT_LENGTH = 8; // length of ";base64,"
const std::int32_t BASE_64_MIME_TYPE_LENGTH = 6; // length of "image/"
const std::string BASE_64_STANDARD_PREFIX = "data:image/png;base64,";
constexpr std::string_view RESFILE_PREFIX = "file:///data/storage/el1/bundle/";
constexpr std::string_view RESFILE_PATH = "/resources/resfile/assets/";
const std::unordered_set<std::string> validImageTypes = {
    "png",
    "jpeg",
    "jpg",
    "gif",
    "bmp",
    "webp",
    "heic",
    "heif",
};

ImageComponentInstance::ImageComponentInstance(Context context)
    : CppComponentInstance(std::move(context)),
      ImageSourceResolver::ImageSourceUpdateListener(
          m_deps->imageSourceResolver),
      m_imageNode(m_arkUINodeCtx) {
  this->getLocalRootArkUINode().setNodeDelegate(this);
  this->getLocalRootArkUINode().setInterpolation(
      ARKUI_IMAGE_INTERPOLATION_HIGH);
  this->getLocalRootArkUINode().setDraggable(false);
  this->getLocalRootArkUINode().setAccessibilityMode(
      facebook::react::ImportantForAccessibility::Auto);
  this->getLocalRootArkUINode().setResizeMode(
      facebook::react::ImageResizeMode::Stretch);
  this->getLocalRootArkUINode().setOrientationAuto();
}

bool isValidMimeType(const std::string& mimeType) {
  if (mimeType.empty()) {
    return false;
  }

  if (mimeType.substr(0, BASE_64_MIME_TYPE_LENGTH) != "image/") {
    return false;
  }
  std::string imageType = mimeType.substr(BASE_64_MIME_TYPE_LENGTH);

  return validImageTypes.find(imageType) != validImageTypes.end();
}

std::string processBase64Uri(const std::string& uri) {
  size_t base64Pos = uri.find(BASE_64_MARK);
  if (base64Pos == std::string::npos) {
    return uri;
  }
  size_t mimeStart = BASE_64_PREFIX.length();
  std::string mimeType = uri.substr(mimeStart, base64Pos - mimeStart);
  if (base64Pos <= mimeStart || !isValidMimeType(mimeType)) {
    // Only change to image/png when MIME type is illegal.
    return BASE_64_STANDARD_PREFIX +
        uri.substr(base64Pos + BASE_64_FORMAT_LENGTH);
  }

  return uri;
}

void ImageComponentInstance::setSources(
    facebook::react::ImageSources const& sources) {
  // Defined layoutMetrics are necessary for determining the best source
  // for current container dimensions
  if (m_layoutMetrics == facebook::react::EmptyLayoutMetrics) {
    return;
  }

  auto resolveResult = m_deps->imageSourceResolver->resolveImageSource(
      *this, m_layoutMetrics, sources);

  // The general image source info is kept by this object as a field.
  // The original, out-of-cache URI is used for event payloads.
  m_imageSource = std::move(resolveResult.bestResult);
  // If the resolver returned an in-cache URI as well, then it's
  // used in place of the original one to not repeat requests.
  // The ImageNode object only acts as an intermediary
  // between this object and the C-API node, and is thus
  // agnostic to whether the image is cached or not.
  auto imageLocator =
      std::move(resolveResult.bestResultUriInCache).value_or(m_imageSource.uri);
  if (auto source =
          m_deps->imageSourceResolver->getImageSourceByName(imageLocator)) {
    imageLocator = *source;
  } else if (imageLocator.starts_with(ASSET_PREFIX)) {
    std::string assetsPrefix = this->getAssetsPrefix();
    imageLocator = assetsPrefix + imageLocator.substr(ASSET_PREFIX.size());
  } else if (
      imageLocator.starts_with(BASE_64_PREFIX) &&
      imageLocator.find(BASE_64_MARK) != std::string::npos) {
    imageLocator = processBase64Uri(imageLocator);
  }

  this->getLocalRootArkUINode().setSource(imageLocator);
}

void ImageComponentInstance::onEventEmitterChanged(
    SharedConcreteEventEmitter const& eventEmitter) {
  if (m_eventEmitter == eventEmitter || eventEmitter == nullptr ||
      m_scheduledEvents.empty()) {
    return;
  }
  m_eventEmitter = eventEmitter;
  for (auto& event : m_scheduledEvents) {
    dispatchEvent(event.eventType, event.payloadFactory);
  }
  m_scheduledEvents.clear();
}

void ImageComponentInstance::dispatchOrScheduleEvent(
    EventType eventType,
    const ValueFactory& payloadFactory) {
  if (!m_eventEmitter) {
    m_scheduledEvents.push_back({eventType, payloadFactory});
    return;
  }
  dispatchEvent(eventType, payloadFactory);
}

void ImageComponentInstance::dispatchEvent(
    EventType eventType,
    const ValueFactory& payloadFactory) {
  std::string eventTypeString;
  switch (eventType) {
    case EventType::ON_LOAD_START:
      eventTypeString = "loadStart";
      break;
    case EventType::ON_LOAD:
      eventTypeString = "load";
      break;
    case EventType::ON_LOAD_END:
      eventTypeString = "loadEnd";
      break;
    case EventType::ON_PROGRESS:
      eventTypeString = "progress";
      break;
    case EventType::ON_ERROR:
      eventTypeString = "error";
      break;
    case EventType::ON_PARTIAL_LOAD:
      eventTypeString = "partialLoad";
      break;
  }
  m_eventEmitter->dispatchEvent(eventTypeString, payloadFactory);
}

void ImageComponentInstance::onPropsChanged(SharedConcreteProps const& props) {
  CppComponentInstance::onPropsChanged(props);

  auto rawProps = ImageRawProps::getFromDynamic(props->rawProps);

#ifdef IMAGE_CACHE_ENABLED
  const bool cacheEnabled = IsImageCacheEnabled();
#endif

  if (m_props->sources != props->sources) {
#ifdef IMAGE_CACHE_ENABLED
    if (!cacheEnabled)
#endif
    {
      invalidatePendingCacheRequest();
      setSources(props->sources);
      dispatchOrScheduleEvent(EventType::ON_LOAD_START);
    }
  }

#ifdef IMAGE_CACHE_ENABLED
  // Track whether blurRadius or resizeMethod changed � these require
  // image reload via ImageCache because they affect the cache key.
  bool blurRadiusChanged = false;
  bool resizeMethodChanged = false;
#endif

  if (m_props->tintColor != props->tintColor) {
    this->getLocalRootArkUINode().setTintColor(props->tintColor);
  }

  if (m_props->blurRadius != props->blurRadius) {
#ifdef IMAGE_CACHE_ENABLED
    if (cacheEnabled) {
      blurRadiusChanged = true;
    } else {
      this->getLocalRootArkUINode().setBlur(props->blurRadius);
    }
#else
    this->getLocalRootArkUINode().setBlur(props->blurRadius);
#endif
  }

  if (m_props->resizeMode != props->resizeMode) {
    this->getLocalRootArkUINode().setObjectRepeat(props->resizeMode);
    this->getLocalRootArkUINode().setResizeMode(props->resizeMode);
  }

  if (m_props->defaultSource != props->defaultSource) {
    this->getLocalRootArkUINode().setAlt(
        props->defaultSource.uri, this->getAssetsPrefix());
  }

  if (m_rawProps.loadingIndicatorSource != rawProps.loadingIndicatorSource) {
    m_rawProps.loadingIndicatorSource = rawProps.loadingIndicatorSource;
    if (m_rawProps.loadingIndicatorSource.has_value()) {
      this->getLocalRootArkUINode().setAlt(
          m_rawProps.loadingIndicatorSource.value(), this->getAssetsPrefix());
    }
  }

  if (m_props->capInsets != props->capInsets) {
    this->getLocalRootArkUINode().setCapInsets(
        props->capInsets, m_layoutMetrics.pointScaleFactor);
  }

  if (m_rawProps.fadeDuration != rawProps.fadeDuration) {
    m_rawProps.fadeDuration = rawProps.fadeDuration;
    if (m_rawProps.fadeDuration.has_value()) {
      this->getLocalRootArkUINode().setFadeDuration(
          m_rawProps.fadeDuration.value());
    } else {
      // reset if no value passed
      this->getLocalRootArkUINode().setFadeDuration(0);
    }
  }

  if (m_rawProps.resizeMethod != rawProps.resizeMethod) {
    m_rawProps.resizeMethod = rawProps.resizeMethod;
#ifdef IMAGE_CACHE_ENABLED
    if (cacheEnabled) {
      resizeMethodChanged = true;
    } else {
      if (m_rawProps.resizeMethod.has_value()) {
        this->getLocalRootArkUINode().setResizeMethod(
            m_rawProps.resizeMethod.value());
      } else {
        this->getLocalRootArkUINode().resetResizeMethod();
      }
    }
#else
    if (m_rawProps.resizeMethod.has_value()) {
      this->getLocalRootArkUINode().setResizeMethod(
          m_rawProps.resizeMethod.value());
    } else {
      this->getLocalRootArkUINode().resetResizeMethod();
    }
#endif
  }

  if (m_rawProps.focusable != rawProps.focusable) {
    m_rawProps.focusable = rawProps.focusable;
    if (m_rawProps.focusable.has_value()) {
      this->getLocalRootArkUINode().setFocusable(m_rawProps.focusable.value());
    } else {
      this->getLocalRootArkUINode().resetFocusable();
    }
  }

  if (m_rawProps.alt != rawProps.alt) {
    m_rawProps.alt = rawProps.alt;
    if (m_rawProps.alt.has_value()) {
      this->getLocalRootArkUINode().setAccessibilityText(
          m_rawProps.alt.value());
    } else {
      this->getLocalRootArkUINode().resetAccessibilityText();
    }
  }

#ifdef IMAGE_CACHE_ENABLED
  if (cacheEnabled) {
    bool needReload =
        blurRadiusChanged || resizeMethodChanged || m_pendingStateUpdate;
    if (needReload || m_props->sources != props->sources) {
      loadImageWithCache(
          {m_state->getData().getImageSource()}, props->blurRadius);
    }
  } else if (m_pendingStateUpdate) {
    invalidatePendingCacheRequest();
    if (m_props->sources == props->sources) {
      setSources({m_state->getData().getImageSource()});
    }
    this->getLocalRootArkUINode().setBlur(props->blurRadius);
  }
  m_pendingStateUpdate = false;
#endif
}

void ImageComponentInstance::onImageSourceCacheUpdate() {
#ifdef IMAGE_CACHE_ENABLED
  if (m_isUserImageCache) {
    return;
  }
#endif
  setSources({m_state->getData().getImageSource()});
}

void ImageComponentInstance::onStateChanged(SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);
#ifdef IMAGE_CACHE_ENABLED
  if (IsImageCacheEnabled()) {
    m_pendingStateUpdate = true;
    return;
  }
#endif
  invalidatePendingCacheRequest();
  setSources({state->getData().getImageSource()});
  this->getLocalRootArkUINode().setBlur(
      state->getData().getImageRequestParams().blurRadius);
}

ImageNode& ImageComponentInstance::getLocalRootArkUINode() {
  return m_imageNode;
}

namespace {
void fadeUpdateCb(void* userData) {
  auto* node = static_cast<ArkUINode*>(userData);
  node->setOpacity(1.0f);
}
} // namespace

void ImageComponentInstance::onComplete(float width, float height) {
#ifdef IMAGE_CACHE_ENABLED
  if (IsImageCacheEnabled() && m_loadSetp != 1) {
    return;
  }
  m_loadSetp++;
#endif
  dispatchOrScheduleEvent(
      EventType::ON_LOAD,
      [uri = m_imageSource.uri, width, height](
          facebook::jsi::Runtime& runtime) {
        auto payload = facebook::jsi::Object(runtime);
        auto source = facebook::jsi::Object(runtime);
        source.setProperty(runtime, "width", width);
        source.setProperty(runtime, "height", height);
        source.setProperty(runtime, "uri", uri.c_str());
        payload.setProperty(runtime, "source", source);
        return payload;
      });
  dispatchOrScheduleEvent(EventType::ON_LOAD_END);

  int32_t fadeDur = m_rawProps.fadeDuration.value_or(0);
  if (fadeDur > 0) {
    auto& img = this->getLocalRootArkUINode();
    static ArkUI_NativeAnimateAPI_1* animateApi = nullptr;
    if (animateApi == nullptr) {
      OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_ANIMATE, ArkUI_NativeAnimateAPI_1, animateApi);
    }
    auto uiCtx = m_arkUINodeCtx->nodeApi->getUIContext();
    if (animateApi != nullptr && uiCtx != nullptr) {
      auto* opt = OH_ArkUI_AnimateOption_Create();
      if (opt != nullptr) {
        img.setOpacity(0.0f);
        OH_ArkUI_AnimateOption_SetDuration(opt, fadeDur);
        OH_ArkUI_AnimateOption_SetCurve(opt, ARKUI_CURVE_LINEAR);
        ArkUI_ContextCallback update;
        update.userData = static_cast<void*>(&img);
        update.callback = &fadeUpdateCb;
        animateApi->animateTo(uiCtx, opt, &update, nullptr);
        OH_ArkUI_AnimateOption_Dispose(opt);
      }
    }
  }
}

void ImageComponentInstance::onError(int32_t errorCode) {
  if (m_eventEmitter == nullptr) {
    return;
  }

  std::string errMsg = "error message: the image load failed, ";
  if (errorCode == 401) {
    errMsg =
        "error message: the image could not be obtained because the image path is invalid, ";
  } else if (errorCode == 103101) {
    errMsg = "error message: the image format is not supported, ";
  }

  errMsg += std::string("error code: ") + std::to_string(errorCode);
  dispatchOrScheduleEvent(
      EventType::ON_ERROR, [errMsg](facebook::jsi::Runtime& runtime) {
        auto payload = facebook::jsi::Object(runtime);
        payload.setProperty(runtime, "error", errMsg);
        auto source = facebook::jsi::Object(runtime);
        source.setProperty(runtime, "error", errMsg);
        payload.setProperty(runtime, "source", source);
        return payload;
      });

  dispatchOrScheduleEvent(EventType::ON_LOAD_END);
}

void ImageComponentInstance::onProgress(uint32_t loaded, uint32_t total) {
  dispatchOrScheduleEvent(
      EventType::ON_PROGRESS, [=](facebook::jsi::Runtime& runtime) {
        auto payload = facebook::jsi::Object(runtime);
        payload.setProperty(runtime, "loaded", (int32_t)loaded);
        payload.setProperty(runtime, "total", (int32_t)total);
        return payload;
      });
}

std::string ImageComponentInstance::getBundlePath() {
  auto rnInstance = m_deps->rnInstance.lock();
  RNOH_ASSERT(rnInstance != nullptr);
  return rnInstance->getBundlePath();
}

std::string ImageComponentInstance::getHspModuleName() {
  auto rnInstance = m_deps->rnInstance.lock();
  RNOH_ASSERT(rnInstance != nullptr);
  return rnInstance->getHspModuleName();
}

std::string ImageComponentInstance::getAssetsPrefix() {
  auto bundlePath = getBundlePath();
  auto position = bundlePath.rfind('/');

  if (position == std::string::npos || bundlePath.find('/', 0) != 0) {
    if (this->getHspModuleName().empty()) {
      return std::string(RAWFILE_PREFIX);
    }
    return std::string(RESFILE_PREFIX) + this->getHspModuleName() +
        std::string(RESFILE_PATH);
  }

  return std::string(FILE_PREFIX) + bundlePath.substr(0, position + 1);
}

ImageComponentInstance::ImageRawProps
ImageComponentInstance::ImageRawProps::getFromDynamic(folly::dynamic value) {
  // Set default resizeMethod as 'auto' for oversized image handling.
  auto resizeMethod = (value.count("resizeMethod") > 0)
      ? std::optional(value["resizeMethod"].asString())
      : std::optional<std::string>("auto");
  auto focusable = (value.count("focusable") > 0)
      ? std::optional(value["focusable"].asBool())
      : std::nullopt;
  auto alt = (value.count("alt") > 0) ? std::optional(value["alt"].asString())
                                      : std::nullopt;
  auto loadingIndicatorSource = (value.count("loadingIndicatorSource") > 0)
      ? std::optional(value["loadingIndicatorSource"].at("uri").getString())
      : std::nullopt;
  auto fadeDuration = (value.count("fadeDuration") > 0)
      ? std::optional(value["fadeDuration"].asInt())
      : std::nullopt;

  return {resizeMethod, focusable, alt, loadingIndicatorSource, fadeDuration};
}
#ifdef IMAGE_CACHE_ENABLED
std::shared_ptr<ImageCachePro::ImageCacheOption>
ImageComponentInstance::buildImageCacheOption(
    facebook::react::Float blurRadius) {
  return ImageCachePro::ImageCacheOption::Create(
      m_imageSource.uri,
      getAssetsPrefix(),
      m_rawProps.resizeMethod.value_or(""),
      static_cast<uint32_t>(
          m_layoutMetrics.frame.size.width * m_layoutMetrics.pointScaleFactor +
          0.5),
      static_cast<uint32_t>(
          m_layoutMetrics.frame.size.height * m_layoutMetrics.pointScaleFactor +
          0.5),
      blurRadius);
}

bool ImageComponentInstance::applyImageCacheData(
    std::shared_ptr<ImageCachePro::ImageCacheData> data) {
  if (data == nullptr) {
    return false;
  }

  getLocalRootArkUINode().setImageCacheData(data);
  if (m_loadSetp == 0) {
    onProgress(100, 100);
  }
  m_loadSetp++;
  return true;
}

bool ImageComponentInstance::startAnimationPlay(
    const ImageCachePro::ImageCacheLoadData& data,
    std::shared_ptr<ImageCachePro::ImageCacheOption> option) {
  if (data.GetFrameContext() == nullptr || option == nullptr) {
    return false;
  }

  auto imageCacheManager = getImageCacheManager();
  if (imageCacheManager == nullptr) {
    return false;
  }

  auto weakSelf = weak_from_this();
  const uint64_t generation = m_imageLoadGeneration;
  auto onFrameReady = [weakSelf, generation](
                          const ImageCachePro::ImageCacheLoadData& data,
                          ImageCachePro::ImageCacheLoadResult result) {
    auto self =
        std::dynamic_pointer_cast<ImageComponentInstance>(weakSelf.lock());
    if (self == nullptr || self->m_imageLoadGeneration != generation) {
      return;
    }
    if (!result.IsSuccess() || !data.IsImageData()) {
      return;
    }
    self->applyImageCacheData(data.GetImageData());
  };

  // Animation end callback: log result and clean up handle
  auto onAnimationEnd = [weakSelf](ImageCachePro::AnimationResult animResult) {
    auto self =
        std::dynamic_pointer_cast<ImageComponentInstance>(weakSelf.lock());
    if (self == nullptr) {
      return;
    }
    self->m_animHandle.store(
        ImageCachePro::INVALID_REQUEST_HANDLE, std::memory_order_release);
  };
  auto animHandle = imageCacheManager->PlayAnimation(
      data, option, nullptr, onFrameReady, onAnimationEnd);
  if (animHandle == ImageCachePro::INVALID_REQUEST_HANDLE) {
    DLOG(WARNING) << "ImageCache PlayAnimation failed for uri: "
                  << m_imageSource.uri;
    return false;
  }

  m_animHandle.store(animHandle, std::memory_order_release);
  return true;
}

void ImageComponentInstance::fallbackToUriSource(
    facebook::react::ImageSources const& sources,
    facebook::react::Float blurRadius) {
  m_isUserImageCache = false;
  cancelActiveImageCacheRequest();

  m_loadSetp++;
  setSources(sources);
  this->getLocalRootArkUINode().setBlur(blurRadius);
  if (m_rawProps.resizeMethod.has_value()) {
    this->getLocalRootArkUINode().setResizeMethod(
        m_rawProps.resizeMethod.value());
  } else {
    this->getLocalRootArkUINode().resetResizeMethod();
  }
}

void ImageComponentInstance::loadImageWithCache(
    facebook::react::ImageSources const& sources,
    facebook::react::Float blurRadius) {
  // Defined layoutMetrics are necessary for determining the best source
  // for current container dimensions
  if (m_layoutMetrics == facebook::react::EmptyLayoutMetrics) {
    return;
  }
  dispatchOrScheduleEvent(EventType::ON_LOAD_START);
  if (m_deps == nullptr || m_deps->imageSourceResolver == nullptr ||
      sources.empty()) {
    fallbackToUriSource(sources, blurRadius);
    return;
  }
  auto rnInstance = m_deps->rnInstance.lock();
  auto imageCacheManager = getImageCacheManager();
  if (rnInstance == nullptr || imageCacheManager == nullptr) {
    fallbackToUriSource(sources, blurRadius);
    return;
  }
  cancelActiveImageCacheRequest();
  m_isUserImageCache = true;
  m_loadSetp = 0;

  m_imageSource = sources[0];
  auto option = buildImageCacheOption(blurRadius);
  if (option == nullptr) {
    fallbackToUriSource(sources, blurRadius);
    return;
  }

  auto hasDownloadProvider = imageCacheManager->HasDownloadProvider();
  if (!hasDownloadProvider) {
    auto imageLoader =
        rnInstance->getTurboModule<ArkTSTurboModule>("ImageLoader");
    if (imageLoader != nullptr) {
      auto downloadProvider =
          ImageCachePro::CreateImageLoaderDownloader(imageLoader);
      imageCacheManager->SetDownloadProvider(downloadProvider);
      hasDownloadProvider = imageCacheManager->HasDownloadProvider();
    }
  }
  const bool isRemoteImage = imageCacheManager->IsRemoteUri(m_imageSource.uri);
  if (auto cachedData = imageCacheManager->GetCachedImageSync(option)) {
    if (applyImageCacheData(std::move(cachedData))) {
      return;
    }
    DLOG(INFO) << "ImageCache HIT (sync path) but applyImageCacheData returned false, "
                  "falling through to async path. uri=" << m_imageSource.uri;
  } else {
    DLOG(INFO) << "ImageCache MISS (sync path): uri=" << m_imageSource.uri
               << ", isRemote=" << isRemoteImage
               << ", hasDownloadProvider=" << hasDownloadProvider;
  }

  const uint64_t generation = ++m_imageLoadGeneration;
  auto weakSelf = weak_from_this();

  // HTTP(S) images: fallback to ArkUI when DownloadProvider is not injected;
  // otherwise use ImageCache for async download, decode, and cache.
  if (isRemoteImage && !hasDownloadProvider) {
    fallbackToUriSource(sources, blurRadius);
    return;
  }
  // Cache miss (first time downloading this URL): dispatch 0% to mark the
  // start of the download/decode phase. Once the data is ready and written
  // into the memory cache, the async callback will dispatch 100% via
  // applyImageCacheData. On subsequent loads for the same URL the sync
  // cache-hit path (GetCachedImageSync) will short-circuit and dispatch
  // 100% directly, skipping this 0% event.
  onProgress(0, 100);
  auto handle = imageCacheManager->GetCacheImage(
      option,
      [weakSelf, generation, sources, blurRadius, option](
          ImageCachePro::ImageCacheLoadData data,
          ImageCachePro::ImageCacheLoadResult result) {
        auto self =
            std::dynamic_pointer_cast<ImageComponentInstance>(weakSelf.lock());
        if (self == nullptr || self->m_imageLoadGeneration != generation) {
          return;
        }
        self->m_activeRequestHandle.store(
            ImageCachePro::INVALID_REQUEST_HANDLE, std::memory_order_release);
        if (result.IsSuccess() && data.IsImageData() &&
            data.GetImageData() != nullptr) {
          if (self->applyImageCacheData(std::move(data.GetImageData()))) {
            return;
          }
        }

        if (result.IsSuccess() && data.IsFrameDecodeData()) {
          if (self->startAnimationPlay(data, option->Clone())) {
            return;
          }
        }
        self->fallbackToUriSource(sources, blurRadius);
      });
  m_activeRequestHandle.store(handle, std::memory_order_release);
}

ImageCachePro::ImageCacheManager::Shared
ImageComponentInstance::getImageCacheManager() {
  auto rnInstance = m_deps->rnInstance.lock();
  if (rnInstance == nullptr) {
    return nullptr;
  }
  auto rnInstanceInternal = static_cast<RNInstanceInternal*>(rnInstance.get());
  return rnInstanceInternal->getImageCacheManager();
}

void ImageComponentInstance::cancelActiveImageCacheRequest() {
  if (auto imageCacheManager = getImageCacheManager()) {
    auto requestHandle = m_activeRequestHandle.load(std::memory_order_acquire);
    if (requestHandle != ImageCachePro::INVALID_REQUEST_HANDLE) {
      imageCacheManager->CancelRequest(requestHandle);
      m_activeRequestHandle.store(
          ImageCachePro::INVALID_REQUEST_HANDLE, std::memory_order_release);
    }
    auto animHandle = m_animHandle.load(std::memory_order_acquire);
    if (animHandle != ImageCachePro::INVALID_REQUEST_HANDLE) {
      imageCacheManager->ControlAnimation(
          animHandle, ImageCachePro::AnimationAction::Cancel);
      m_animHandle.store(
          ImageCachePro::INVALID_REQUEST_HANDLE, std::memory_order_release);
    }
  } else {
    m_activeRequestHandle.store(
        ImageCachePro::INVALID_REQUEST_HANDLE, std::memory_order_release);
    m_animHandle.store(
        ImageCachePro::INVALID_REQUEST_HANDLE, std::memory_order_release);
  }
}
#endif

void ImageComponentInstance::invalidatePendingCacheRequest() {
#ifdef IMAGE_CACHE_ENABLED
  cancelActiveImageCacheRequest();
  ++m_imageLoadGeneration;
#endif
}

ImageComponentInstance::~ImageComponentInstance() {
  this->getLocalRootArkUINode().setNodeDelegate(nullptr);
#ifdef IMAGE_CACHE_ENABLED
  cancelActiveImageCacheRequest();
#endif
}
} // namespace rnoh
