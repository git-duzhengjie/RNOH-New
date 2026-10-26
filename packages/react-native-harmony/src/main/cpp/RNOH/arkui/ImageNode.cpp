/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageNode.h"

#ifdef IMAGE_CACHE_ENABLED
#include <arkui/drawable_descriptor.h>
#include "RNOH/ImageCache/include/ImageCacheData.h"
#endif

#include <native_drawing/drawing_color_filter.h>
#include <climits>
#include <string_view>
#include "DynamicArkUILoader.h"
#include "NativeNodeApi.h"
#include "RNOH/ApiVersionCheck.h"

static constexpr ArkUI_NodeEventType IMAGE_NODE_EVENT_TYPES[] = {
    NODE_IMAGE_ON_COMPLETE,
    NODE_IMAGE_ON_ERROR,
    NODE_IMAGE_ON_DOWNLOAD_PROGRESS};

using namespace std::literals;
constexpr std::string_view ASSET_PREFIX = "asset://"sv;
const std::string RAWFILE_PREFIX = "resource://RAWFILE/assets/";

namespace rnoh {

ImageNode::ImageNode(const ArkUINode::Context::Shared& context)
    : ArkUINode(context, ArkUI_NodeType::ARKUI_NODE_IMAGE),
      m_childArkUINodeHandle(nullptr),
      m_imageNodeDelegate(nullptr) {
  for (auto eventType : IMAGE_NODE_EVENT_TYPES) {
    registerNodeEvent(eventType);
  }
}

ImageNode::~ImageNode() {
#ifdef IMAGE_CACHE_ENABLED
  disposeDrawableSource();
#endif
  for (auto eventType : IMAGE_NODE_EVENT_TYPES) {
    unregisterNodeEvent(eventType);
  }
}

#ifdef IMAGE_CACHE_ENABLED
void ImageNode::disposeDrawableSource() {
  // CRITICAL: Release DrawableDescriptor BEFORE clearing m_cachedImageData.
  // DrawableDescriptor may hold references to PixelMaps in ImageCacheData.
  // 1. First dispose the DrawableDescriptor (ArkUI's wrapper)
  // 2. Then release our ImageCacheData reference (actual PixelMap owner)
  // This order ensures ArkUI doesn't access freed PixelMaps.
  if (m_drawableDescriptor != nullptr) {
    OH_ArkUI_DrawableDescriptor_Dispose(m_drawableDescriptor);
    m_drawableDescriptor = nullptr;
  }
  // Now safe to release ImageCacheData - DrawableDescriptor no longer
  // references it
  m_cachedImageData = nullptr;
}
#endif

void ImageNode::setNodeDelegate(ImageNodeDelegate* imageNodeDelegate) {
  m_imageNodeDelegate = imageNodeDelegate;
}

void ImageNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  ArkUINode::onNodeEvent(eventType, eventArgs);
#ifdef IMAGE_CACHE_ENABLED
  if (m_imageCacheWating.load()) {
    return;
  }
#endif
  if (eventType == ArkUI_NodeEventType::NODE_IMAGE_ON_COMPLETE) {
    if (m_imageNodeDelegate != nullptr && eventArgs[0].i32 == 1) {
      m_imageNodeDelegate->onComplete(eventArgs[1].f32, eventArgs[2].f32);
    }
  }

  if (eventType == ArkUI_NodeEventType::NODE_IMAGE_ON_ERROR) {
#ifdef IMAGE_CACHE_ENABLED
    if (m_cachedImageData != nullptr &&
        m_cachedImageData->GetFrameCount() > 0 && eventArgs[0].i32 == 401) {
      DLOG(ERROR) << "ImageNode received errorCode 62980149";
      return;
    }
#endif
    if (m_imageNodeDelegate != nullptr) {
      m_imageNodeDelegate->onError(eventArgs[0].i32);
    }
  }

  if (eventType == ArkUI_NodeEventType::NODE_IMAGE_ON_DOWNLOAD_PROGRESS) {
    if (m_imageNodeDelegate != nullptr) {
      m_imageNodeDelegate->onProgress(eventArgs[0].u32, eventArgs[1].u32);
    }
  }
}
ImageNode& ImageNode::setSources(std::string const& uri, std::string prefix) {
#ifdef IMAGE_CACHE_ENABLED
  disposeDrawableSource();
  m_imageCacheWating.store(false, std::memory_order_release);
#endif
  m_uri = uri;
  ArkUI_AttributeItem item;
  std::string absolutePath = prefix == "" ? RAWFILE_PREFIX : prefix;
  if (uri.rfind(ASSET_PREFIX, 0) == 0) {
    absolutePath += uri.substr(ASSET_PREFIX.size());
    item = {.string = absolutePath.c_str()};
  } else {
    item = {.string = uri.c_str()};
  }
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_SRC, &item);
  return *this;
}

ImageNode& ImageNode::setSource(std::string const& imageSource) {
#ifdef IMAGE_CACHE_ENABLED
  disposeDrawableSource();
  m_imageCacheWating.store(false, std::memory_order_release);
#endif
  ArkUI_AttributeItem item = {.string = imageSource.c_str()};
  m_uri = imageSource;
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_SRC, &item);
  return *this;
}

#ifdef IMAGE_CACHE_ENABLED
ImageNode& ImageNode::setImageCacheData(
    std::shared_ptr<ImageCachePro::ImageCacheData> data) {
  if (data == nullptr) {
    return *this;
  }

  disposeDrawableSource();

  if (data->GetFrameCount() > 1) {
    setAnimatedPixelMaps(
        data->GetPixelmapList(),
        data->GetFrameCount(),
        data->GetDelayTimeList());
  } else {
    setPixelMap(data->GetPixelmap());
  }
  if (m_drawableDescriptor != nullptr) {
    m_imageCacheWating.store(false, std::memory_order_release);
    m_cachedImageData = std::move(data);
  }
  return *this;
}

ImageNode& ImageNode::setPixelMap(OH_PixelmapNative* pixelmap) {
  if (pixelmap == nullptr) {
    return *this;
  }
  disposeDrawableSource();
  auto* drawable = OH_ArkUI_DrawableDescriptor_CreateFromPixelMap(pixelmap);
  if (drawable == nullptr) {
    return *this;
  }
  m_drawableDescriptor = drawable;
  ArkUI_AttributeItem item = {.object = drawable};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_SRC, &item);
  return *this;
}

ImageNode& ImageNode::setAnimatedPixelMaps(
    OH_PixelmapNative** pixelmapList,
    uint32_t frameCount,
    const int32_t* delayTimeList) {
  if (pixelmapList == nullptr || frameCount <= 1) {
    return *this;
  }
  for (uint32_t i = 0; i < frameCount; i++) {
    if (pixelmapList[i] == nullptr) {
      LOG(WARNING) << "setAnimatedPixelMaps: pixelmapList[" << i
                   << "] is null, skipping animated image";
      return *this;
    }
  }
  disposeDrawableSource();
  auto* drawable = OH_ArkUI_DrawableDescriptor_CreateFromAnimatedPixelMap(
      pixelmapList, static_cast<int32_t>(frameCount));
  if (drawable == nullptr) {
    return *this;
  }

  setDrawableAnimationTiming(drawable, delayTimeList, frameCount);

  // -1 means infinite loop
  OH_ArkUI_DrawableDescriptor_SetAnimationIteration(drawable, -1);

  m_drawableDescriptor = drawable;
  ArkUI_AttributeItem item = {.object = drawable};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_SRC, &item);
  return *this;
}

void ImageNode::setDrawableAnimationTiming(
    ArkUI_DrawableDescriptor* drawable,
    const int32_t* delayTimeList,
    uint32_t frameCount) {
  if (delayTimeList == nullptr) {
    // No delay time list provided, set a default duration (e.g., 100ms per
    // frame)
    uint64_t duration = static_cast<uint64_t>(frameCount) * 100;
    int32_t defaultDurationMs =
        duration > INT32_MAX ? INT32_MAX : static_cast<int32_t>(duration);
    OH_ArkUI_DrawableDescriptor_SetAnimationDuration(
        drawable, defaultDurationMs);
    return;
  }

  // Set animation timing based on API version
  // API 22+ supports per-frame durations for accurate GIF timing
  // Below API 22, only total duration is supported (frames evenly distributed)
  // Try to use dynamic loaded API for SetAnimationFrameDurations (API 22+)
  if (DynamicArkUILoader::initAPI22()) {
    auto setAnimationFrameDurations =
        DynamicArkUILoader::getSetAnimationFrameDurationsFun();
    if (setAnimationFrameDurations != nullptr) {
      std::vector<int> sanitized(frameCount);
      for (uint32_t i = 0; i < frameCount; i++) {
        sanitized[i] = delayTimeList[i] < 0 ? 0 : delayTimeList[i];
      }
      // Use per-frame durations for accurate animation timing (API 22+)
      setAnimationFrameDurations(
          drawable, sanitized.data(), static_cast<int32_t>(frameCount));
      return;
    }
  }
  // Fallback: calculate total duration for older API versions
  int64_t totalDurationMs = 0;
  for (uint32_t i = 0; i < frameCount; i++) {
    int d = delayTimeList[i] < 0 ? 0 : delayTimeList[i];
    totalDurationMs += d;
    if (totalDurationMs > INT32_MAX) {
      totalDurationMs = INT32_MAX;
      break;
    }
  }
  if (totalDurationMs > 0) {
    OH_ArkUI_DrawableDescriptor_SetAnimationDuration(
        drawable, static_cast<int32_t>(totalDurationMs));
  }
}
#endif

ImageNode& ImageNode::setResizeMode(
    facebook::react::ImageResizeMode const& mode) {
  // Based on:
  // https://github.com/facebook/react-native/blob/main/packages/react-native/ReactAndroid/src/main/java/com/facebook/react/views/image/ImageResizeMode.kt
  int32_t val = ARKUI_OBJECT_FIT_COVER;
  switch (mode) {
    case facebook::react::ImageResizeMode::Cover:
      val = ARKUI_OBJECT_FIT_COVER;
      break;
    case facebook::react::ImageResizeMode::Contain:
      val = ARKUI_OBJECT_FIT_CONTAIN;
      break;
    case facebook::react::ImageResizeMode::Center:
    case facebook::react::ImageResizeMode::Repeat:
      val = ARKUI_OBJECT_FIT_SCALE_DOWN;
      break;
    case facebook::react::ImageResizeMode::Stretch:
      val = ARKUI_OBJECT_FIT_FILL;
      break;
  }

  ArkUI_NumberValue value[] = {{.i32 = val}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_OBJECT_FIT, &item);
  return *this;
}

ImageNode& ImageNode::setTintColor(
    facebook::react::SharedColor const& sharedColor) {
  if (!sharedColor) { // restore default value
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_IMAGE_COLOR_FILTER);
    return *this;
  }

  auto* colorFilter =
      OH_Drawing_ColorFilterCreateBlendMode(*sharedColor, BLEND_MODE_SRC_IN);
  if (colorFilter == nullptr) {
    DLOG(WARNING) << "setTintColor: failed to create color filter";
    return *this;
  }
  ArkUI_AttributeItem item = {.size = 0, .object = colorFilter};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_COLOR_FILTER, &item);
  OH_Drawing_ColorFilterDestroy(colorFilter);
  return *this;
}

ImageNode& ImageNode::setBlur(facebook::react::Float blur) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(blur)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_BLUR, &item);
  return *this;
}

ImageNode& ImageNode::setObjectRepeat(
    facebook::react::ImageResizeMode const& resizeMode) {
  int32_t val = ARKUI_IMAGE_REPEAT_NONE;
  if (resizeMode == facebook::react::ImageResizeMode::Repeat) {
    val = ARKUI_IMAGE_REPEAT_XY;
  }

  ArkUI_NumberValue value[] = {{.i32 = val}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_OBJECT_REPEAT, &item);
  return *this;
}

ImageNode& ImageNode::setInterpolation(int32_t interpolation) {
  ArkUI_NumberValue value[] = {{.i32 = interpolation}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_INTERPOLATION, &item);
  return *this;
}

ImageNode& ImageNode::setDraggable(bool draggable) {
  ArkUI_NumberValue value[] = {{.i32 = static_cast<int32_t>(draggable)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_DRAGGABLE, &item);
  return *this;
}

ImageNode& ImageNode::setCapInsets(
    facebook::react::EdgeInsets const& capInsets,
    float dpi) {
  float left = 0;
  float right = 0;
  float top = 0;
  float bottom = 0;
  if (capInsets != facebook::react::EdgeInsets::ZERO) {
    left = capInsets.left / (dpi * 2);
    right = (capInsets.right < 1 ? 1 : capInsets.right) /
        (dpi * 2); // arkui need right >= 1 if wants capInsets works
    top = capInsets.top / (dpi * 2);
    bottom = (capInsets.bottom < 1 ? 1 : capInsets.bottom) /
        (dpi * 2); // arkui need bottom >= 1 if wants capInsets works
  }

  ArkUI_NumberValue value[] = {
      {.f32 = left}, {.f32 = top}, {.f32 = right}, {.f32 = bottom}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_RESIZABLE, &item);
  return *this;
}

ImageNode& ImageNode::setFadeDuration(int32_t duration) {
  // Do not set NODE_OPACITY_TRANSITION here. For an Image initially mounted under an
  // opacity:0 ancestor, when that ancestor's opacity later switches to 1, ArkUI fires
  // ON_DISAPPEAR on the Image, and this transition attribute drives a 1->0 fade-out
  // that makes the image invisible. Fade-in is instead driven on image load
  // (ImageComponentInstance::onComplete) via animateTo.
  (void)duration;
  return *this;
}

ImageNode& ImageNode::setOrientationAuto() {
  if (!IsAtLeastApi21()) {
    return *this;
  }

  // NODE_IMAGE_ORIENTATION is an enum item (not a preprocessor macro).
  // Use the documented attribute id to keep compatibility with older headers.
  constexpr ArkUI_NodeAttributeType imageOrientationAttr =
      static_cast<ArkUI_NodeAttributeType>(4020);
  constexpr int32_t autoOrientation = 0; // ImageRotateOrientation.AUTO
  ArkUI_NumberValue value[] = {{.i32 = autoOrientation}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, imageOrientationAttr, &item);
  return *this;
}

ImageNode& ImageNode::setFocusable(bool focusable) {
  ArkUI_NumberValue value[] = {{.i32 = static_cast<int32_t>(focusable)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_FOCUSABLE, &item);
  return *this;
}

ImageNode& ImageNode::setResizeMethod(std::string const& resizeMethod) {
  auto autoResize = (resizeMethod != "scale") ? 1 : 0;
  ArkUI_NumberValue value[] = {{.i32 = autoResize}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_AUTO_RESIZE, &item);
  return *this;
}

ImageNode& ImageNode::setAlt(std::string const& uri, std::string prefix) {
  if (!uri.empty()) {
    std::string resourceStr = prefix == "" ? RAWFILE_PREFIX : prefix;
    resourceStr += uri.substr(ASSET_PREFIX.size());
    ArkUI_AttributeItem item = {.string = resourceStr.c_str()};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_IMAGE_ALT, &item);
  }
  return *this;
}

ImageNode& ImageNode::setAccessibilityMode(
    facebook::react::ImportantForAccessibility importance) {
  if (importance == facebook::react::ImportantForAccessibility::Auto) {
    ArkUINode::setAccessibilityMode(
        ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_ENABLED);
  } else {
    ArkUINode::setAccessibilityMode(importance);
  }
  return *this;
}

ImageNode& ImageNode::resetFocusable() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_FOCUSABLE);
  return *this;
}
ImageNode& ImageNode::resetResizeMethod() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_IMAGE_AUTO_RESIZE);
  return *this;
}
ImageNode& ImageNode::resetSource() {
#ifdef IMAGE_CACHE_ENABLED
  disposeDrawableSource();
  m_imageCacheWating.store(false, std::memory_order_release);
#endif
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_IMAGE_SRC);
  return *this;
}

std::string ImageNode::getUri() {
  return m_uri;
}
} // namespace rnoh
