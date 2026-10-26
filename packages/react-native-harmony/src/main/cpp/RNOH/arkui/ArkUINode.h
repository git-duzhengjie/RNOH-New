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
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <folly/dynamic.h>
#include <react/renderer/components/FBReactNativeSpec/Props.h>
#include <react/renderer/components/view/AccessibilityPrimitives.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/graphics/BackgroundImage.h>
#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/Rect.h>
#include <react/renderer/graphics/Transform.h>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include "NodeApi.h"
#include "glog/logging.h"
#include "react/renderer/components/view/primitives.h"

namespace rnoh {

class ArkUINode;

/**
 * Delegate interface for handling ArkUI node lifecycle and interaction events.
 * Used by node implementations to notify their owners about node events without
 * tight coupling.
 *
 * @actor RNOH_LIBRARY
 */
class ArkUINodeDelegate {
 public:
  virtual ~ArkUINodeDelegate() = default;

  /**
   * Called when an ArkUI node is about to be destroyed.
   * @param node The node being destroyed
   */
  virtual void onArkUINodeDestroy(ArkUINode* /*node*/){};

  /**
   * Called when an accessibility action is triggered on the node.
   * @param node The node on which the action was triggered
   * @param actionName The name of the accessibility action performed
   */
  virtual void onArkUINodeAccessibilityAction(
      ArkUINode* /*node*/,
      const std::string& /*actionName*/){};

  /**
   * Called to handle intercepted touch events on the node.
   * @param event The touch event data
   */
  virtual void onArkUINodeTouchIntercept(const ArkUI_UIInputEvent* event){};
};

/**
 * @actor RNOH_LIBRARY
 *
 * rnoh::ArkUINode is the adaption layer between rnoh::ComponentInstance and
 * the ArkUI_Node that ArkUI system component provides. This class exposes
 * basic settings an ArkUI_Node supports, eg. setting orientation, alignment,
 * opacity etc.
 */
class ArkUINode {
  ArkUINodeDelegate* m_arkUINodeDelegate = nullptr;

 public:
  struct Context {
    Context() = default;
    explicit Context(std::shared_ptr<NodeApi> nodeApi) : nodeApi(nodeApi) {}
    using Shared = std::shared_ptr<Context>;
    std::shared_ptr<NodeApi> nodeApi = nullptr;
    std::shared_ptr<class TaskExecutor> taskExecutor = nullptr;
  };

 protected:
  ArkUINode(const ArkUINode& other) = delete;
  ArkUINode& operator=(const ArkUINode& other) = delete;
  ArkUINode& operator=(ArkUINode&& other) = delete;
  ArkUINode(ArkUINode&& other) = delete;
  using EventArgs = ArkUI_NumberValue[MAX_COMPONENT_EVENT_ARG_NUM];

 public:
  /**
   * @brief Input event source type enumeration
   */
  enum {
    UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN = 0,
    UI_INPUT_EVENT_SOURCE_TYPE_MOUSE = 1,
    UI_INPUT_EVENT_SOURCE_TYPE_TOUCH_SCREEN = 2,
    UI_INPUT_EVENT_SOURCE_TYPE_KEY = 4
  };

  using Alignment = ArkUI_Alignment;

  /**
   * @brief Gets a pointer to the underlying ArkUI_Node
   * @return Handle to the native ArkUI node
   */
  ArkUI_NodeHandle getArkUINodeHandle();

  /**
   * @brief Constructs an ArkUINode from a native node handle
   * @param nodeHandle Handle to the native ArkUI node
   */
  ArkUINode(ArkUI_NodeHandle nodeHandle);

  /**
   * @brief Constructs an ArkUINode with Context and node type
   * Creates NodeApi and then creates the node handle internally
   * @param context Shared pointer to Context containing shared NodeApi and
   * dependencies
   * @param nodeType Type of ArkUI node to create
   */
  ArkUINode(const Context::Shared context, ArkUI_NodeType nodeType);

  ArkUINodeDelegate* getArkUINodeDelegate() const {
    return m_arkUINodeDelegate;
  }

  /**
   * @brief Sets the delegate for handling node events and actions
   * @param arkUiNodeDelegate Pointer to the delegate implementation
   */
  void setArkUINodeDelegate(ArkUINodeDelegate* arkUiNodeDelegate);

  /**
   * @brief Marks this node as needing layout, measure and render
   */
  void markDirty();

  /**
   * @brief Sets accessibility role from string name
   * @param role Role name like "button", "searchbox" etc. See
   * NODE_TYPE_BY_ROLE_NAME for all supported roles
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityRole(std::string const& role);

  /**
   * @brief Sets accessibility role from React Native role enum
   * @param role React Native accessibility role
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityRole(facebook::react::Role role);

  /**
   * @brief Sets the node's upper left corner position relative to its parent
   * @param position Point containing x,y coordinates
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setPosition(facebook::react::Point const& position);

  /**
   * @brief Sets the node's size
   * @param size Size containing width and height
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setSize(facebook::react::Size const& size);

  /**
   * @deprecated Do not use it. This is a method specific to CustomNode and
   * CustomNode already has an equivalent method called updateMeasuredSize.
   * Deprecated when preparing RNOH 0.77 for release.
   * @param width Width value to save
   * @param height Height value to save
   */
  void saveSize(int32_t width, int32_t height);

  /**
   * @deprecated Do not use it. This is a method specific to CustomNode and
   * CustomNode already has an equivalent method called getMeasuredWidth.
   * This method shouldn't be public. Deprecated when preparing RNOH 0.77 for
   * release.
   * @return Saved width value
   */
  int32_t getSavedWidth();

  /**
   * @deprecated Do not use it. This is a method specific to CustomNode and
   * CustomNode already has an equivalent method called getMeasuredHeight.
   * This method shouldn't have been public. Deprecated when preparing RNOH 0
   * .77 for release.
   * @return Saved height value
   */
  int32_t getSavedHeight();

  /**
   * @brief Sets the layout rectangle properties
   * @param position Position relative to parent
   * @param size Size including border and padding
   * @param pointScaleFactor Device pixel density scale factor
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setLayoutRect(
      facebook::react::Point const& position,
      facebook::react::Size const& size,
      facebook::react::Float pointScaleFactor);

  /**
   * @brief Sets layout metrics including position and size
   * @param layoutMetrics Layout metrics object with frame info
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setLayoutRect(
      facebook::react::LayoutMetrics const& layoutMetrics);

  /**
   * @deprecated Internal method, use setAccessibilityMode instead
   */
  virtual ArkUINode& setHitTestMode(ArkUI_HitTestMode hitTestMode);

  /**
   * @deprecated The matching of pointerEvents and hitTestBehavior has been
   * moved to CppComponentInstance::onArkUINodeTouchIntercept
   */
  virtual ArkUINode& setHitTestMode(
      facebook::react::PointerEventsMode const& pointerEvents);

  /**
   * @brief Sets the node height
   * @param height Height value in vp units
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setHeight(float height);

  /**
   * @brief Sets the node width
   * @param width Width value in vp units
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setWidth(float width);

  /**
   * @brief Sets border widths for all sides
   * @param borderWidths Border width values
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBorderWidth(
      facebook::react::BorderWidths const& borderWidths);

  /**
   * @brief Sets border colors for all sides
   * @param borderColors Border color values
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBorderColor(
      facebook::react::BorderColors const& borderColors);

  /**
   * @brief Sets border radius for all corners
   * @param borderRadius Border radius values
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBorderRadius(
      facebook::react::BorderRadii const& borderRadius);

  /**
   * @brief Sets border styles for all sides
   * @param borderStyles Border style values
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBorderStyle(
      facebook::react::BorderStyles const& borderStyles);

  /**
   * @brief Sets background color from React Native color
   * @param color Shared color value
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBackgroundColor(
      facebook::react::SharedColor const& color);

  /**
   * @brief Sets background color from ARGB value
   * @param color Color in ARGB format (e.g. 0xFFFF0000 for red)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBackgroundColor(uint32_t color);

  /**
   * @brief Sets a linear gradient background from BackgroundImage variants.
   * @param backgroundImage Vector of BackgroundImage variants (only the first
   * LinearGradient element is applied; RadialGradient is currently ignored).
   * @param width Current frame width (for resolving pixel stop positions).
   * @param height Current frame height (for resolving pixel stop positions).
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBackgroundImage(
      std::vector<facebook::react::BackgroundImage> const& backgroundImage,
      float width,
      float height);

  /**
   * @brief Sets a linear gradient background from a single BackgroundImage.
   * @param backgroundImage A BackgroundImage variant (LinearGradient applied;
   * RadialGradient currently ignored).
   * @param width Current frame width (for resolving pixel stop positions).
   * @param height Current frame height (for resolving pixel stop positions).
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBackgroundImage(
      facebook::react::BackgroundImage const& backgroundImage,
      float width,
      float height);

  /**
   * @brief Clears the linear gradient background.
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetBackgroundImage();

  /**
   * @brief Applies transform with scaling
   * @param transform Transform matrix. The unit is vp
   * @param pointScaleFactor Device pixel density scale factor
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setTransform(
      facebook::react::Transform const& transform,
      facebook::react::Float pointScaleFactor);

  /**
   * @brief Sets translation in 3D space
   * @param x X translation
   * @param y Y translation
   * @param z Z translation (optional)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setTranslate(float x, float y, float z = 0.0f);

  /**
   * @brief Sets shadow properties
   * @param shadowColor Shadow color
   * @param shadowOffset Shadow offset. The unit is vp
   * @param shadowOpacity Shadow opacity. 0 means fully transparent
   * @param shadowRadius Shadow blur radius
   * @param pointScaleFactor Device pixel density scale
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setShadow(
      facebook::react::SharedColor const& shadowColor,
      facebook::react::Size const& shadowOffset,
      float const shadowOpacity,
      float const shadowRadius,
      facebook::react::Float pointScaleFactor);

  /**
   * @brief Sets accessibility actions
   * @param rnActions Vector of accessibility actions
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityActions(
      const std::vector<facebook::react::AccessibilityAction>& rnActions);

  /**
   * @brief Description for the accessibility action, preferably informs the
   * user what would happen after the action is taken. If the component
   * contains text, accessibility description would be read after the
   * contained text
   * would be
   * @param accessibilityDescription Description text
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityDescription(
      std::string const& accessibilityDescription);

  /**
   * @deprecated: Use setAccessibilityState(const
   * std::optional<facebook::react::AccessibilityState>& state) instead.
   * (latestRNOHVersion 0.72.40)
   */
  virtual ArkUINode& setAccessibilityState(
      const facebook::react::AccessibilityState& state);

  /**
   * @brief Sets accessibility state
   * @param state Optional accessibility state
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityState(
      const std::optional<facebook::react::AccessibilityState>& state);

  /**
   * @deprecated Use setAccessibilityMode instead
   */
  virtual ArkUINode& setAccessibilityLevel(
      facebook::react::ImportantForAccessibility importance);

  /**
   * @brief Sets accessibility mode from React Native importance
   * @param importance Accessibility importance level
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityMode(
      facebook::react::ImportantForAccessibility importance);

  /**
   * @brief Sets accessibility mode with ArkUI type directly
   * @param mode Accessibility mode value
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityMode(ArkUI_AccessibilityMode mode);

  /**
   * @brief Sets what would be read when the component is selected under
   * accessibility mode. When this property is set, text inside the component
   * won't be read
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityText(
      std::string const& accessibilityLabel);

  /**
   * @brief Clears accessibility text
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetAccessibilityText();

  /**
   * @brief When set to true, this node and all it's child would be
   * considered one candidate for accessibility selection. The accessibility
   * service won't recurse to it's children
   * @param accessible True to enable group
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAccessibilityGroup(bool accessible);

  /**
   * @brief Sets node identifier
   * @param id Identifier string
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setId(std::string const& id);

  /**
   * @brief Gets node identifier
   * @return Node ID string
   */
  virtual std::string getId() const;

  /**
   * @brief Sets opacity value
   * @param opacity Opacity from 0 (transparent) to 1 (opaque)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setOpacity(facebook::react::Float opacity);

  /**
   * @brief Sets content clipping
   * @param clip True to enable content clipping
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setClip(bool clip);

  /**
   * @brief Sets alignment within parent
   * @param alignment Alignment value
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setAlignment(Alignment alignment);

  /**
   * @brief Sets translation transition animation
   * @param translateX Target X translation
   * @param translateY Target Y translation
   * @param animationDurationMillis Animation duration in ms
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setTranslateTransition(
      float translateX,
      float translateY,
      int32_t animationDurationMillis);

  /**
   * @brief Clears translation transition
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetTranslateTransition();

  /**
   * @brief Sets opacity transition animation
   * @param animationDurationMillis Animation duration in ms
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setOpacityTransition(int32_t animationDurationMillis);

  /**
   * @brief Clears opacity transition
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetOpacityTransition();

  /**
   * @brief Sets the component's childs' offset relative to itself
   * @param x X offset
   * @param y Y offset
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setOffset(float x, float y);

  /**
   * @brief Sets if the node can be interacted with by the user
   * @param enabled True to enable interaction
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setEnabled(bool enabled);

  /**
   * @brief Sets if this component has focus
   * @note Setting the parameter to 0 shifts focus from the currently
   * focused component on the current level of the page to the root container
   * @param focus Focus status value
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setFocusStatus(int32_t focus);

  /**
   * @brief Sets margin for all sides
   * @param left Left margin
   * @param top Top margin
   * @param right Right margin
   * @param bottom Bottom margin
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode&
  setMargin(float left, float top, float right, float bottom);

  /**
   * @brief Sets padding for all sides
   * @param left Left padding
   * @param top Top padding
   * @param right Right padding
   * @param bottom Bottom padding
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode&
  setPadding(float left, float top, float right, float bottom);

  /**
   * @brief Sets if the node should be visible or even be skipped during layout
   * @param visibility Visibility value
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setVisibility(ArkUI_Visibility visibility);

  /**
   * @brief Sets z-index for stacking order
   * @param index Z-index value (larger = in front)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setZIndex(float index);

  /**
   * @brief Sets whether the current component and it's children should be
   * rendered off the screen first and then fused with the parent
   * @param renderOffscreen True to enable offscreen rendering
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setRenderGroup(bool renderOffscreen);

  /**
   * @brief Sets layout direction
   * @param direction Text/layout direction
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setDirection(ArkUI_Direction direction);

  /**
   * @brief Sets blur effect
   * @param blur Blur radius in pixels
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBlur(facebook::react::Float blur);

  /**
   * @brief Resets blur effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetBlur();

  /**
   * @brief Sets grayscale effect
   * @param grayscale Grayscale amount (0-1, where 1 is fully grayscale)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setGrayscale(facebook::react::Float grayscale);

  /**
   * @brief Resets grayscale effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetGrayscale();

  /**
   * @brief Sets brightness effect
   * @param brightness Brightness amount (0-1 for normal, >1 for brighter, <0
   * for darker)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setBrightness(facebook::react::Float brightness);

  /**
   * @brief Resets brightness effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetBrightness();

  /**
   * @brief Sets invert effect
   * @param invert Invert amount (0-1, where 1 is fully inverted)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setInvert(facebook::react::Float invert);

  /**
   * @brief Resets invert effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetInvert();

  /**
   * @brief Sets saturate effect
   * @param saturate Saturation amount (0-1 for desaturated, 1 normal, >1 for
   * saturated)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setSaturate(facebook::react::Float saturate);

  /**
   * @brief Resets saturate effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetSaturate();

  /**
   * @brief Sets sepia effect
   * @param sepia Sepia amount (0-1, where 1 is fully sepia)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setSepia(facebook::react::Float sepia);

  /**
   * @brief Resets sepia effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetSepia();

  /**
   * @brief Sets contrast effect
   * @param contrast Contrast amount (0-1 for low contrast, 1 normal, >1 for
   * high contrast)
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setContrast(facebook::react::Float contrast);

  /**
   * @brief Resets contrast effect to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetContrast();

  /**
   * @brief Sets outline widths for all sides
   * @param outlineWidths Outline width values for left, top, right, bottom
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setOutlineWidth(
      facebook::react::BorderWidths const& outlineWidths);

  /**
   * @brief Resets outline width to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetOutlineWidth();

  /**
   * @brief Sets outline colors for all sides
   * @param outlineColors Outline color values for left, top, right, bottom
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& setOutlineColors(
      facebook::react::BorderColors const& outlineColors);

  /**
   * @brief Resets outline colors to default
   * @return Reference to this node for method chaining
   */
  virtual ArkUINode& resetOutlineColors();

  /**
   * @brief Gets layout position offset
   * @return Position offset
   */
  virtual ArkUI_IntOffset getLayoutPosition();

  /**
   * @brief Method for handling node events with ArkUI_NodeComponentEvent type
   * @note Events that are marked ArkUI_NodeComponentEvent in
   * ArkUI_NodeEventType would be handled by this method
   * @param eventType Event type identifier
   * @param eventArgs Event arguments
   */
  virtual void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs);

  /**
   * @brief Method for handling node events with ArkUI_StringAsyncEvent type
   * @note Events that are marked ArkUI_StringAsyncEvent in
   * ArkUI_NodeEventType would be handled by this method
   * @param eventType Event type identifier
   * @param eventString Event string data
   */
  virtual void onNodeEvent(
      ArkUI_NodeEventType eventType,
      std::string_view eventString);

  /**
   * @brief Method for handling node events without a specific type
   * @param eventType Event type identifier
   * @param ArkUI_NodeEvent* pointer to the event
   */
  virtual void onNodeEvent(
      ArkUI_NodeEventType eventType,
      ArkUI_NodeEvent* event);

  /**
   * @brief Handles touch intercept events
   * @param event Touch input event
   */
  virtual void onTouchIntercept(const ArkUI_UIInputEvent* event);

  /**
   * @brief Verify the focus status of the TextInput.
   * @return Focus status: true-focused, false-unfocused.
   */
  virtual bool isFocused();

  virtual ~ArkUINode() noexcept;

 protected:
  void maybeThrow(int32_t status) const {
    // TODO: map status to error message, maybe add a new error type
    static const auto ARKUI_ERROR_CODE_NOT_SUPPORTED_FOR_ARKTS_NODE = 106103;
    if (status == ARKUI_ERROR_CODE_NOT_SUPPORTED_FOR_ARKTS_NODE) {
      // This is a quick fix for a problem that has arisen after updating the
      // ROM and SDK.
      LOG(WARNING) << "Couldn't set a property on ArkTS node";
      return;
    }
    if (status != 0) {
      auto message = std::string("ArkUINode operation failed with status: ") +
          std::to_string(status);
      LOG(ERROR) << message;
      throw std::runtime_error(std::move(message));
    }
  }

  void registerNodeEvent(ArkUI_NodeEventType eventType);
  void unregisterNodeEvent(ArkUI_NodeEventType eventType);

  const ArkUI_AttributeItem& getAttribute(
      ArkUI_NodeAttributeType attribute) const;

  void setAttribute(
      ArkUI_NodeAttributeType attribute,
      ArkUI_AttributeItem const& item);

  void setAttribute(
      ArkUI_NodeAttributeType attribute,
      std::initializer_list<ArkUI_NumberValue> values);

  template <size_t N>
  void setAttribute(
      ArkUI_NodeAttributeType attribute,
      std::array<ArkUI_NumberValue, N> const& values) {
    ArkUI_AttributeItem item{.value = values.data(), .size = N};
    setAttribute(attribute, item);
  }

  ArkUI_NodeHandle m_nodeHandle;

 protected:
  std::shared_ptr<NodeApi> m_nodeApi = std::make_shared<NodeApi>();

 private:
  int32_t m_measuredWidth = 0;
  int32_t m_measuredHeight = 0;
};
} // namespace rnoh
