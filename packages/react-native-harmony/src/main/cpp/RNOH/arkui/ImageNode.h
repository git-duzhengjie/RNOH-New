/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <react/renderer/imagemanager/primitives.h>
#include <cstdint>
#include <mutex>
#include "ArkUINode.h"

#ifdef IMAGE_CACHE_ENABLED
#include <arkui/drawable_descriptor.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <memory>
namespace ImageCachePro {
struct ImageCacheData;
}
#endif

namespace rnoh {
/**
 * @actor RNOH_LIBRARY
 * @brief This interface defines methods that can be overriden to react on
 * ImageNode events.
 */
class ImageNodeDelegate {
 public:
  /**
   * @brief Destructor for ImageNodeDelegate.
   */
  virtual ~ImageNodeDelegate() = default;

  /**
   * @brief Called when the image loading is completed.
   * @param width The width of the loaded image in vp units.
   * @param height The height of the loaded image in vp units.
   * This method can be overridden to perform actions after an image is
   * successfully loaded.
   */
  virtual void onComplete(float width, float height){};

  /**
   * @brief Called when there is an error during the image loading process.
   * @param errorCode The error code indicating the type of failure.
   * This method can be overridden to handle errors such as failed image
   * loading or network issues.
   */
  virtual void onError(int32_t errorCode){};

  /**
   * @brief Called periodically during the image loading process to
   * provide progress updates.
   * @param loaded The amount of data that has been loaded.
   * @param total The total amount of data to be loaded.
   * This method can be overridden to track and display progress while loading
   * an image.
   */
  virtual void onProgress(uint32_t loaded, uint32_t total){};
};

/**
 * @actor RNOH_LIBRARY
 * @brief Represents an ArkUI Image component node.
 *
 * This class allows configuration of image source, resize mode, tint color,
 * alternative text for assistive technologies, blur effect, and more. Supports
 * event handling via delegate.
 */
class ImageNode : public ArkUINode {
 protected:
  /// Handle to the underlying child ArkUI node, if any.
  ArkUI_NodeHandle m_childArkUINodeHandle;

  /// Pointer to an image event delegate (optional).
  ImageNodeDelegate* m_imageNodeDelegate;

  /// The URI of the image source.
  std::string m_uri;

#ifdef IMAGE_CACHE_ENABLED
  void disposeDrawableSource();

  ArkUI_DrawableDescriptor* m_drawableDescriptor = nullptr;
  std::atomic<bool> m_imageCacheWating{true};
  std::shared_ptr<ImageCachePro::ImageCacheData> m_cachedImageData;

  void setDrawableAnimationTiming(
      ArkUI_DrawableDescriptor* drawable,
      const int32_t* delayTimeList,
      uint32_t frameCount);
  /**
   * @brief Sets image content from a decoded PixelMap (API 12+).
   */
  ImageNode& setPixelMap(OH_PixelmapNative* pixelmap);

  /**
   * @brief Sets animated image content from decoded PixelMap frames (API 12+).
   */
  ImageNode& setAnimatedPixelMaps(
      OH_PixelmapNative** pixelmapList,
      uint32_t frameCount,
      const int32_t* delayTimeList = nullptr);
#endif

 public:
  /**
   * @brief Constructs an ImageNode with default parameters.
   *
   * @param context The Context of ArkUINode.
   */
  explicit ImageNode(const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief Destroys the ImageNode and unregisters its events.
   */
  ~ImageNode();

  /**
   * @brief Sets a single image source from the given URI.
   * @param src Image URI string.
   * @return Reference to this ImageNode.
   */
  ImageNode& setSource(std::string const& src);

#ifdef IMAGE_CACHE_ENABLED
  /**
   * @brief Sets image content from cached ImageCacheData (API 12+).
   * DrawableDescriptor and ImageCacheData share the same lifecycle.
   * @param data Decoded image cache data.
   * @return Reference to this ImageNode.
   */
  ImageNode& setImageCacheData(
      std::shared_ptr<ImageCachePro::ImageCacheData> data);
#endif

  /**
   * @brief Sets the image source and optionally prepends a prefix.
   * @param uri The image URI.
   * @param prefix Optional prefix (default is RAWFILE path).
   * @return Reference to this ImageNode.
   */
  ImageNode& setSources(std::string const& uri, std::string prefix = "");

  /**
   * @brief Sets the image resize (object-fit) mode.
   * @param mode Enum specifying the resize behavior (cover, contain, etc.).
   * @return Reference to this ImageNode.
   */
  ImageNode& setResizeMode(facebook::react::ImageResizeMode const& mode);

  /**
   * @brief Applies a tint color to the image.
   * @param sharedColor SharedColor object specifying RGBA color.
   * @return Reference to this ImageNode.
   */
  ImageNode& setTintColor(facebook::react::SharedColor const& sharedColor);

  /**
   * @brief Applies a blur effect to the image.
   * @param blur The blur radius.
   * @return Reference to this ImageNode.
   */
  ImageNode& setBlur(facebook::react::Float blur);

  /**
   * @brief Sets repeat behavior when using repeat mode.
   * @param resizeMode Image resize mode (Repeat or not).
   * @return Reference to this ImageNode.
   */
  ImageNode& setObjectRepeat(
      facebook::react::ImageResizeMode const& resizeMode);

  /**
   * @brief Sets the cap insets for 9-patch style image resizing.
   * @param capInsets Insets from the edges (top, left, bottom, right).
   * @param dpi Current display DPI for correct scaling.
   * @return Reference to this ImageNode.
   */
  ImageNode& setCapInsets(
      facebook::react::EdgeInsets const& capInsets,
      float dpi);

  /**
   * @brief Sets the image interpolation method.
   * @param interpolation Interpolation mode (e.g., linear).
   * @return Reference to this ImageNode.
   */
  ImageNode& setInterpolation(int32_t interpolation);

  /**
   * @brief Enables or disables image drag-and-drop interaction.
   * @param draggable True to enable dragging.
   * @return Reference to this ImageNode.
   */
  ImageNode& setDraggable(bool draggable);

  /**
   * @brief Sets whether the image can receive focus from keyboard navigation or
   * screen readers.
   * @param focusable True to make image focusable.
   * @return Reference to this ImageNode.
   */
  ImageNode& setFocusable(bool focusable);

  /**
   * @brief Sets how the image is resized (e.g., "auto", "scale").
   * @param resizeMethod Resize method string.
   * @return Reference to this ImageNode.
   */
  ImageNode& setResizeMethod(std::string const& resizeMethod);

  /**
   * @brief Sets alternative text to be used when the image cannot be displayed.
   * @param uri Alt text resource URI.
   * @param prefix Optional prefix (defaults to RAWFILE).
   * @return Reference to this ImageNode.
   */
  ImageNode& setAlt(std::string const& uri, std::string prefix = "");

  /**
   * @brief Sets the duration for fade-in transition.
   * @param duration Transition time in milliseconds.
   * @return Reference to this ImageNode.
   */
  ImageNode& setFadeDuration(int32_t duration);

  /**
   * @brief Sets image rotate orientation to AUTO when the platform supports it.
   * @return Reference to this ImageNode.
   */
  ImageNode& setOrientationAuto();
  /**
   * @brief Specifies the level of importance used by assistive technologies
   * when interpreting this image.
   * @param importance A value that indicates the importance level used by
   * assistive technologies (e.g., screen readers) when interpreting this image.
   * @return Reference to this ImageNode.
   */
  ImageNode& setAccessibilityMode(
      facebook::react::ImportantForAccessibility importance) override;

  /**
   * @brief Resets the focusable attribute to its default state.
   * @return Reference to this ImageNode.
   */
  ImageNode& resetFocusable();

  /**
   * @brief Resets the resize method attribute to its default state.
   * @return Reference to this ImageNode.
   */
  ImageNode& resetResizeMethod();

  /**
   * @brief Resets the image source to empty.
   * @return Reference to this ImageNode.
   */
  ImageNode& resetSource();

  /**
   * @brief Event dispatcher for image-related ArkUI events.
   * @param eventType The type of the UI event.
   * @param eventArgs Arguments passed with the event.
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;

  /**
   * @brief Assigns an image event delegate for callback notifications.
   * @param imageNodeDelegate The delegate pointer to assign.
   */
  void setNodeDelegate(ImageNodeDelegate* imageNodeDelegate);

  /**
   * @brief Retrieves the currently set image URI.
   * @return Image URI string.
   */
  std::string getUri();
};
} // namespace rnoh