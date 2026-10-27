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
#include <arkui/native_type.h>
#include <cxxreact/TraceSection.h>
#include <glog/logging.h>
#include <react/renderer/components/view/TouchEventEmitter.h>
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/EventEmitter.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/core/State.h>
#include <type_traits>
#include <utility>
#include <vector>
#include "RNOH/ApiVersionCheck.h"
#include "RNOH/Assert.h"
#include "RNOH/ComponentInstance.h"

using facebook::react::isZero;

namespace rnoh {

/**
 * 探测 props 类型是否具备 needsOffscreenAlphaCompositing 字段。
 *
 * RN 0.88 起该字段从跨平台的 BaseViewProps 移入平台专用的
 * HostPlatformViewProps（目前仅 Android 版提供）。这里用 traits 做条件编译，
 * 保证在字段不存在的新版本上仍可编译。
 */
template <typename T, typename = void>
struct HasNeedsOffscreenAlphaCompositing : std::false_type {};

template <typename T>
struct HasNeedsOffscreenAlphaCompositing<
    T,
    std::void_t<
        decltype(std::declval<const T&>().needsOffscreenAlphaCompositing)>>
    : std::true_type {};

inline facebook::react::Rect transformRectAroundPoint(
    const facebook::react::Rect& rect,
    const facebook::react::Point& point,
    const facebook::react::Transform& transform);

/**
 * @brief Template class that implements core React Native component
 * functionality for the OpenHarmony platform.
 *
 * CppComponentInstance serves as a bridge between React Native's shadow tree
 * and ArkUI native components. It manages component lifecycle, handles
 * property updates, state management, event handling, and touch interaction.
 *
 * Key features:
 * - Manages component props, state, and event emitters
 * - Handles layout updates and transformations
 * - Processes touch events and hit testing
 * - Manages accessibility properties
 * - Handles component borders, opacity, and other visual properties
 *
 * @tparam ShadowNodeT The concrete shadow node type this instance represents
 */
template <typename ShadowNodeT>
class CppComponentInstance : public ComponentInstance,
                             public ArkUINodeDelegate {
  static_assert(
      std::is_base_of_v<facebook::react::ShadowNode, ShadowNodeT>,
      "ShadowNodeT must be a subclass of facebook::react::ShadowNode");

 protected:
  using ConcreteProps = typename ShadowNodeT::ConcreteProps;
  using ConcreteState = typename ShadowNodeT::ConcreteState;
  using ConcreteEventEmitter = typename ShadowNodeT::ConcreteEventEmitter;
  using SharedConcreteProps = std::shared_ptr<const ConcreteProps>;
  using SharedConcreteState = std::shared_ptr<const ConcreteState>;
  using SharedConcreteEventEmitter =
      std::shared_ptr<const ConcreteEventEmitter>;

 public:
  using Shared = std::shared_ptr<CppComponentInstance>;

  CppComponentInstance(Context context)
      : ComponentInstance(std::move(context)) {}

  /**
   * @brief Called when the component instance is created
   */
  void onCreate() override {
    try {
      this->getLocalRootArkUINode().setArkUINodeDelegate(this);
    } catch (NoArkUINodeError&) {
      // NOOP
    }
  }

  /**
   * @brief Returns the React tag (unique integer identifier) for this
   * component instance.
   * @return The tag associated with this component instance.
   */
  facebook::react::Tag getTag() const {
    return m_tag;
  }

  /**
   * @brief Returns a component ID string, prioritizing testId, then
   * nativeId, or generates a fallback ID if neither is set.
   * @return The component's ID string.
   */
  std::string getCompId() override {
    return this->getIdFromProps(m_props);
  }

  /**
   * @brief Gets the current React props for this component instance.
   * @return Shared pointer to the current props object.
   */
  facebook::react::Props::Shared getProps() const override {
    return m_props;
  }

  /**
   * @brief Gets the current state object for this component instance.
   * @return Shared pointer to the current state object.
   */
  SharedConcreteState const& getState() const {
    return m_state;
  }

  /**
   * @brief Gets the current event emitter for this component instance.
   * @return Shared pointer to the current event emitter object.
   */
  SharedConcreteEventEmitter const& getEventEmitter() const {
    return m_eventEmitter;
  }

  /**
   * @brief Returns the accessibility label for this component.
   * @return The accessibility label string.
   */
  const std::string& getAccessibilityLabel() const override {
    return m_accessibilityLabel;
  }

  /**
   * @brief Indicates if this component is in an accessibility group (i.e.,
   * should be treated as a group by screen readers).
   * @return True if this is an accessibility group, false otherwise.
   */
  bool getAccessibilityGroup() const override {
    return m_accessibilityGroup;
  }

  /**
   * TODO: change to private — those methods are intended to be called
   * only by MountingManagerCAPI which is a friend of this class
   * (latestRNOHVersion: 0.72.27)
   */
 public:
  /**
   * @deprecated
   * @brief Sets the React props for this component instance. If the new props
   * are different, triggers onPropsChanged and updates internal state.
   * @param props Shared pointer to the new props object.
   */
  void setProps(facebook::react::Props::Shared props) final {
    auto newProps = std::dynamic_pointer_cast<const ConcreteProps>(props);
    if (!newProps || m_props == newProps) {
      return;
    }
    this->onPropsChanged(newProps);
    m_props = newProps;
  }

  /**
   * @deprecated
   * @brief Sets the React state for this component instance. If the new state
   * is different, triggers onStateChanged and updates internal state.
   * @param state Shared pointer to the new state object.
   */
  void setState(facebook::react::State::Shared state) final {
    auto newState = std::dynamic_pointer_cast<const ConcreteState>(state);
    if (!newState || m_state == newState) {
      return;
    }

    this->onStateChanged(newState);
    m_state = newState;
  }

  /**
   * @deprecated
   * @brief Sets the event emitter for this component instance. If the new
   * event emitter is different, triggers onEventEmitterChanged and updates
   * internal state.
   * @param eventEmitter Shared pointer to the new event emitter object.
   */
  void setEventEmitter(facebook::react::SharedEventEmitter eventEmitter) final {
    auto newEventEmitter =
        std::dynamic_pointer_cast<const ConcreteEventEmitter>(eventEmitter);
    if (!newEventEmitter || m_eventEmitter == newEventEmitter) {
      return;
    }
    this->onEventEmitterChanged(newEventEmitter);
    m_eventEmitter = newEventEmitter;
  }

  /**
   * @deprecated
   * @brief Sets the layout metrics (position, size, etc.) for this component.
   * Triggers onLayoutChanged and updates internal layout state.
   * @param layoutMetrics The new layout metrics.
   */
  void setLayout(facebook::react::LayoutMetrics layoutMetrics) override {
    if (layoutMetrics == m_layoutMetrics) {
      return;
    }
    this->onLayoutChanged(layoutMetrics);
    m_layoutMetrics = layoutMetrics;
  }

 public:
  // TouchTarget implementation
  /**
   * @brief Returns the current layout metrics (position, size, etc.) for this
   * component instance.
   * @return The current layout metrics.
   */
  facebook::react::LayoutMetrics getLayoutMetrics() const override {
    return m_layoutMetrics;
  }

  /**
   * @brief Checks if a given point is within the hit rectangle of this
   * component (used for touch handling).
   * @param point The point to check (position is relative to upperleft corner
   * of this component).
   * @return True if the point is within the hit rect, false otherwise.
   */
  bool containsPoint(facebook::react::Point const& point) const override {
    auto hitRect = getHitRect();
    return hitRect.containsPoint(point);
  }

  /**
   * @brief Checks if a given point is within the bounding box of this
   * component (including children, if not clipped).
   * @param point The point to check (position is relative to upperleft corner
   * of this component).
   * @return True if the point is within the bounding box, false otherwise.
   */
  bool containsPointInBoundingBox(
      facebook::react::Point const& point) override {
    auto boundingBox = this->getBoundingBox();
    return boundingBox.containsPoint(point);
  }

  /**
   * @brief Determines if this component can handle touch events directly.
   * Considers parent and pointerEvents prop.
   * @return True if this component can handle touch, false otherwise.
   */
  bool canHandleTouch() const override {
    auto parent = m_parent.lock();
    if (parent && !parent->canChildrenHandleTouch()) {
      return false;
    }
    if (m_props != nullptr) {
      auto props =
          std::dynamic_pointer_cast<const facebook::react::ViewProps>(m_props);
      return props->pointerEvents == facebook::react::PointerEventsMode::Auto ||
          props->pointerEvents == facebook::react::PointerEventsMode::BoxOnly;
    }
    return true;
  };

  /**
   * @brief Determines if this component's children can handle touch events.
   * Considers parent and pointerEvents prop.
   * @return True if children can handle touch, false otherwise.
   */
  bool canChildrenHandleTouch() const override {
    auto parent = m_parent.lock();
    if (parent && !parent->canChildrenHandleTouch()) {
      return false;
    }
    if (m_props != nullptr) {
      auto props =
          std::dynamic_pointer_cast<const facebook::react::ViewProps>(m_props);
      return props->pointerEvents == facebook::react::PointerEventsMode::Auto ||
          props->pointerEvents == facebook::react::PointerEventsMode::BoxNone;
    }
    return true;
  };

  /**
   * @brief Recursively checks if any child in the subtree can handle a touch
   * event at the given point.
   * @param point The point to check (position is relative to upperleft corner
   * of this component).
   * @return True if any child can handle the touch, false otherwise.
   */
  bool canSubtreeHandleTouch(facebook::react::Point const& point) override {
    auto children = getTouchTargetChildren();
    for (auto const& child : children) {
      if (child == nullptr) {
        RNOH_ASSERT(child != nullptr);
        continue;
      }

      auto childPoint = computeChildPoint(point, child);
      if (child->canHandleTouch() && child->containsPoint(childPoint) &&
          child->getTouchEventEmitter()) {
        return true;
      }

      if (child->canChildrenHandleTouch() &&
          child->containsPointInBoundingBox(childPoint)) {
        if (child->canSubtreeHandleTouch(childPoint)) {
          return true;
        }
      }
    }
    return false;
  };

  /**
   * @brief Returns the tag of the touch target for this component
   * @return The tag of the touch target.
   */
  facebook::react::Tag getTouchTargetTag() const override {
    return getTag();
  }

  /**
   * @brief Returns the parent component of this component, if any.
   * @return Shared pointer to the parent touch target, or nullptr if none.
   */
  TouchTarget::Shared getTouchTargetParent() const override {
    return getParent().lock();
  }

  /**
   * @brief Returns the touch event emitter for this component instance.
   * @return Shared pointer to the touch event emitter.
   */
  facebook::react::SharedTouchEventEmitter getTouchEventEmitter()
      const override {
    return m_eventEmitter;
  }

  /**
   * @brief Returns a vector of shared pointers to the touch target children
   * of this component instance.
   * @return Vector of shared pointers to child touch targets.
   */
  std::vector<TouchTarget::Shared> getTouchTargetChildren() override {
    auto children = getChildren();
    return std::vector<TouchTarget::Shared>(children.begin(), children.end());
  }

  /**
   * @brief Returns the current transform matrix for this component instance.
   * @return The transform matrix.
   */
  facebook::react::Transform getTransform() const override {
    return m_transform;
  }

  /**
   * @brief Returns the bounding box of this component, including children if
   * not clipped. Caches the result for efficiency.
   * @return The bounding box rectangle.
   */
  facebook::react::Rect getBoundingBox() override {
    if (!m_boundingBox.has_value()) {
      calculateBoundingBox();
    }
    return m_boundingBox.value();
  };

  /**
   * @brief Indicates if this component clips its subviews (children) to its
   * bounds.
   * @return True if subviews are clipped, false otherwise.
   */
  bool isClippingSubviews() const override {
    return m_isClipping;
  }

  /**
   * @brief Marks the cached bounding box as dirty, so it will be recalculated
   * on next access. Propagates up the parent chain if not clipped.
   */
  void markBoundingBoxAsDirty() override {
    if (m_boundingBox.has_value()) {
      m_boundingBox.reset();
      auto parent = getTouchTargetParent();
      while (parent != nullptr && !parent->isClippingSubviews()) {
        parent->markBoundingBoxAsDirty();
        parent = parent->getTouchTargetParent();
      }
    }
  };

 protected:
  /**
   * @brief Called when the layout metrics for this component change.
   *
   * Updates the local root node's layout, transform, shadow, and direction as
   * needed. Also marks the bounding box as dirty to trigger recalculation.
   *
   * @param layoutMetrics The new layout metrics for the component.
   */
  virtual void onLayoutChanged(
      facebook::react::LayoutMetrics const& layoutMetrics) {
    auto maybeLocalRoot = maybeGetLocalRoot();
    if (!maybeLocalRoot) {
      return;
    }
    auto& localRoot = *maybeLocalRoot;
    localRoot.setLayoutRect(
        layoutMetrics.frame.origin,
        layoutMetrics.frame.size,
        layoutMetrics.pointScaleFactor);
    auto transform = m_props->resolveTransform(layoutMetrics);
    if (transform != m_transform) {
      m_transform = transform;
      localRoot.setTransform(m_transform, layoutMetrics.pointScaleFactor);
    }
    if (layoutMetrics.pointScaleFactor != m_layoutMetrics.pointScaleFactor) {
      auto props =
          std::static_pointer_cast<const facebook::react::ViewProps>(m_props);
      applyShadow(localRoot, props, layoutMetrics.pointScaleFactor);
    }
    if (layoutMetrics.layoutDirection != m_layoutMetrics.layoutDirection) {
      ArkUI_Direction direction =
          convertLayoutDirection(layoutMetrics.layoutDirection);
      localRoot.setDirection(direction);
    }
    // Re-resolve linear gradient stop positions when the frame size changes.
    // Pixel (Point) stop positions are relative to the gradient line length,
    // which depends on the current size; re-resolved on layout change
    // (aligns with other platforms).
    if (m_props && layoutMetrics.frame.size != m_layoutMetrics.frame.size) {
      auto bgProps =
          std::static_pointer_cast<const facebook::react::ViewProps>(m_props);
      if (!bgProps->backgroundImage.empty()) {
        localRoot.setBackgroundImage(
            bgProps->backgroundImage,
            layoutMetrics.frame.size.width,
            layoutMetrics.frame.size.height);
      }
    }
    bool shouldReapplyState = m_state &&
        (!m_hasStateLayoutMetrics || m_stateLayoutMetrics != layoutMetrics);
    if (shouldReapplyState) {
      m_layoutMetrics = layoutMetrics;
      onStateChanged(m_state);
      m_stateLayoutMetrics = layoutMetrics;
      m_hasStateLayoutMetrics = true;
    }
    markBoundingBoxAsDirty();
  }

  /**
   * @brief Called when the props for this component change.
   *
   * Updates the local root node's properties and triggers any necessary
   * updates.
   *
   * @param concreteProps The new props for the component.
   */
  virtual void onPropsChanged(SharedConcreteProps const& concreteProps) {
    auto maybeLocalRoot = maybeGetLocalRoot();
    if (!maybeLocalRoot) {
      return;
    }
    auto& localRoot = *maybeLocalRoot;

    auto props = std::static_pointer_cast<const facebook::react::ViewProps>(
        concreteProps);
    auto old =
        std::static_pointer_cast<const facebook::react::ViewProps>(m_props);
    RNOH_ASSERT(old != nullptr);
    auto isTransformManagedByAnimated =
        getIgnoredPropKeys().count("transform") > 0;
    if (*(props->backgroundColor) != *(old->backgroundColor)) {
      localRoot.setBackgroundColor(props->backgroundColor);
    }

    if (props->accessibilityState != old->accessibilityState) {
      localRoot.setAccessibilityState(props->accessibilityState);
    }

    if (props->shadowColor != old->shadowColor ||
        props->shadowOffset != old->shadowOffset ||
        props->shadowOpacity != old->shadowOpacity ||
        props->shadowRadius != old->shadowRadius ||
        props->boxShadow != old->boxShadow) {
      applyShadow(localRoot, props, m_layoutMetrics.pointScaleFactor);
    }
    if (props->backgroundImage != old->backgroundImage) {
      localRoot.resetBackgroundImage();
      if (!props->backgroundImage.empty()) {
        localRoot.setBackgroundImage(
            props->backgroundImage,
            m_layoutMetrics.frame.size.width,
            m_layoutMetrics.frame.size.height);
      }
    }
    if (props->accessibilityActions != old->accessibilityActions) {
      localRoot.setAccessibilityActions(props->accessibilityActions);
    }

    if (props->filter != old->filter) {
      localRoot.resetBlur();
      localRoot.resetGrayscale();
      localRoot.resetBrightness();
      localRoot.resetInvert();
      localRoot.resetSaturate();
      localRoot.resetSepia();
      localRoot.resetContrast();
      if (!props->filter.empty()) {
        for (const auto& filterFunc : props->filter) {
          if (std::holds_alternative<facebook::react::Float>(
                  filterFunc.parameters)) {
            auto value =
                std::get<facebook::react::Float>(filterFunc.parameters);
            switch (filterFunc.type) {
              case facebook::react::FilterType::Blur:
                localRoot.setBlur(value);
                break;
              case facebook::react::FilterType::Grayscale:
                localRoot.setGrayscale(value);
                break;
              case facebook::react::FilterType::Brightness:
                localRoot.setBrightness(value);
                break;
              case facebook::react::FilterType::Invert:
                localRoot.setInvert(value);
                break;
              case facebook::react::FilterType::Saturate:
                localRoot.setSaturate(value);
                break;
              case facebook::react::FilterType::Sepia:
                localRoot.setSepia(value);
                break;
              case facebook::react::FilterType::Contrast:
                localRoot.setContrast(value);
                break;
              case facebook::react::FilterType::Opacity:
                // Opacity filter is not set directly on the node.
                // Instead, it multiplies with props->opacity in setOpacity()
                // Multiple opacity filters are multiplied together.
                break;
              default:
                LOG(WARNING) << "Unsupported filter type encountered: "
                             << static_cast<int>(filterFunc.type);
                break;
            }
          }
        }
      }
    }

    if (props->outlineWidth != old->outlineWidth) {
      if (props->outlineWidth == 0.0f) {
        localRoot.resetOutlineWidth();
      } else {
        facebook::react::BorderWidths outlineWidths{
            props->outlineWidth,
            props->outlineWidth,
            props->outlineWidth,
            props->outlineWidth};
        localRoot.setOutlineWidth(outlineWidths);
      }
    }

    if (props->outlineColor != old->outlineColor) {
      if (!props->outlineColor) {
        localRoot.resetOutlineColors();
      } else {
        facebook::react::BorderColors outlineColors{
            props->outlineColor,
            props->outlineColor,
            props->outlineColor,
            props->outlineColor};
        localRoot.setOutlineColors(outlineColors);
      }
    }

    if (!isTransformManagedByAnimated) {
      /**
       * NOTE: resolveTransform returns identity when layoutMetrics width or
       * height is 0, which happens during the preallocation phase.
       */
      auto transform = props->resolveTransform(m_layoutMetrics);
      /**
       * NOTE: rn and arkui uses different measure units but that only affects
       * the translation component in transform, not scale etc.
       */
      bool pointScaleFactorChanged =
          abs(m_oldPointScaleFactor - m_layoutMetrics.pointScaleFactor) >
          0.001f;
      bool hasTranslationInTransform = !isZero(transform.matrix[12]) ||
          !isZero(transform.matrix[13]) || !isZero(transform.matrix[14]);
      if (pointScaleFactorChanged) {
        m_oldPointScaleFactor = m_layoutMetrics.pointScaleFactor;
      }
      if (transform != m_transform ||
          (pointScaleFactorChanged && hasTranslationInTransform)) {
        m_transform = transform;
        this->getLocalRootArkUINode().setTransform(
            transform, m_layoutMetrics.pointScaleFactor);
        markBoundingBoxAsDirty();
      }
    }

    if (props->accessibilityRole != old->accessibilityRole &&
        props->role == facebook::react::Role::None) {
      localRoot.setAccessibilityRole(props->accessibilityRole);
    }

    if (props->role != old->role) {
      localRoot.setAccessibilityRole(props->role);
    }

    if (props->accessibilityHint != old->accessibilityHint) {
      localRoot.setAccessibilityDescription(props->accessibilityHint);
    }

    if (props->accessibilityLabel != old->accessibilityLabel) {
      localRoot.setAccessibilityText(props->accessibilityLabel);
    }

    if (props->accessible != old->accessible) {
      localRoot.setAccessibilityGroup(props->accessible);
      m_accessibilityGroup = props->accessible;
      m_accessibilityGroupChanged = true;
    }

    if (props->importantForAccessibility != old->importantForAccessibility) {
      localRoot.setAccessibilityMode(props->importantForAccessibility);
    }

    this->setOpacity(props);

    auto newOverflow = props->getClipsContentToBounds();
    if ((old->getClipsContentToBounds() != newOverflow)) {
      m_isClipping = newOverflow;
      localRoot.setClip(newOverflow);
      markBoundingBoxAsDirty();
    }

    if constexpr (HasNeedsOffscreenAlphaCompositing<
                      std::remove_reference_t<decltype(*props)>>::value) {
      if (props->needsOffscreenAlphaCompositing !=
          old->needsOffscreenAlphaCompositing) {
        localRoot.setRenderGroup(props->needsOffscreenAlphaCompositing);
      }
    }

    localRoot.setId(getIdFromProps(props));
  };

  /**
   * @brief Called when the state for this component changes.
   *
   * Can be overridden to handle state changes.
   *
   * @param state The new state for the component.
   */
  virtual void onStateChanged(SharedConcreteState const& /*state*/) {
    facebook::react::TraceSection s(std::string(
                                        "#RNOH::CppComponentInstance(" +
                                        this->getComponentName() +
                                        ")::onStateChanged")
                                        .c_str());
  };

  /**
   * @brief Called when the event emitter for this component changes.
   *
   * Can be overridden to handle event emitter changes.
   *
   * @param eventEmitter The new event emitter for the component.
   */
  virtual void onEventEmitterChanged(
      SharedConcreteEventEmitter const& /*eventEmitter*/){};

  /**
   * @brief Calculates and updates the bounding box for this component.
   *
   * The bounding box includes this component and, if not clipping, all its
   * children. The result is cached for efficiency.
   */
  void calculateBoundingBox() {
    facebook::react::TraceSection s(std::string(
                                        "#RNOH::CppComponentInstance(" +
                                        this->getComponentName() +
                                        ")::calculateBoundingBox")
                                        .c_str());
    auto newBoundingBox = getHitRect();
    if (!m_isClipping) {
      for (auto& child : m_children) {
        auto childBoundingBox = child->getBoundingBox();
        childBoundingBox.origin += child->getLayoutMetrics().frame.origin;

        auto childCenter = child->getLayoutMetrics().frame.getCenter();
        auto childTransform = child->getTransform();

        childBoundingBox = transformRectAroundPoint(
            childBoundingBox, childCenter, childTransform);
        newBoundingBox.unionInPlace(childBoundingBox);
      }
    }
    m_boundingBox = newBoundingBox;
  };

  /**
   * @brief Finalizes updates after all property and state changes.
   *
   * Applies any pending updates to the local root node, such as accessibility
   * and border properties, after all changes have been processed.
   */
  void onFinalizeUpdates() override {
    auto maybeLocalRoot = maybeGetLocalRoot();
    if (!maybeLocalRoot) {
      return;
    }
    auto& localRoot = *maybeLocalRoot;

    ComponentInstance::onFinalizeUpdates();
    if (m_props->accessibilityLabelledBy.value != m_accessibilityLabelledBy) {
      std::string targetId = "";
      if (!m_props->accessibilityLabelledBy.value.empty()) {
        targetId = m_props->accessibilityLabelledBy.value[0];
      }
      if (!targetId.empty()) {
        auto componentInstanceRegistry =
            m_deps->componentInstanceRegistry.lock();
        if (componentInstanceRegistry != nullptr) {
          auto componentInstance =
              componentInstanceRegistry->findById(targetId);
          if (componentInstance != nullptr) {
            std::string newAccessibilityLabel = "";
            if (m_props->accessibilityLabel != "") {
              newAccessibilityLabel += m_props->accessibilityLabel;
            }
            const auto& targetAccessibilityLabel =
                componentInstance->getAccessibilityLabel();
            if (!targetAccessibilityLabel.empty()) {
              newAccessibilityLabel += " " + targetAccessibilityLabel;
            }
            if (!newAccessibilityLabel.empty()) {
              localRoot.setAccessibilityText(newAccessibilityLabel);
            }
          } else {
            DLOG(WARNING) << "Couldn't find ComponentInstance with Id: "
                          << targetId;
          }
        }
      }
    }
    m_accessibilityLabelledBy = m_props->accessibilityLabelledBy.value;

    facebook::react::BorderMetrics borderMetrics =
        m_props->resolveBorderMetrics(this->m_layoutMetrics);
    
    auto isRTL = m_layoutMetrics.layoutDirection ==
        facebook::react::LayoutDirection::RightToLeft;

    // Prefer props-resolved borderWidths (avoids Slider double-border from
    // layout-metric-driven updates). When Yoga swapped L/R in RTL and
    // borderLeft/RightWidth are set, manually swap to match Yoga's physical mapping.
    facebook::react::BorderWidths physicalBorderWidths = borderMetrics.borderWidths;
    if (m_layoutMetrics.wasLeftAndRightSwapped) {
      auto cascadedWidths = m_props->getBorderWidths();
      if (cascadedWidths.left.has_value() ||
          cascadedWidths.right.has_value()) {
        std::swap(physicalBorderWidths.left, physicalBorderWidths.right);
      }
    }
    if (physicalBorderWidths != m_oldLayoutBorderWidths) {
      localRoot.setBorderWidth(physicalBorderWidths);
      m_oldLayoutBorderWidths = physicalBorderWidths;
    }
    auto resolvedBorderColors =
        m_props->borderColors.resolve(isRTL, facebook::react::blackColor());
    if (resolvedBorderColors != m_oldBorderMetrics.borderColors) {
      localRoot.setBorderColor(resolvedBorderColors);
    }
    if (borderMetrics.borderRadii != m_oldBorderMetrics.borderRadii ||
        !m_isRadiusSetValid) {
      if (this->m_layoutMetrics.frame.size != facebook::react::Size{0, 0}) {
        m_isRadiusSetValid = true;
      }
      localRoot.setBorderRadius(borderMetrics.borderRadii);
    }
    if (borderMetrics.borderStyles != m_oldBorderMetrics.borderStyles) {
      localRoot.setBorderStyle(borderMetrics.borderStyles);
    }
    m_oldBorderMetrics = borderMetrics;
    m_oldBorderMetrics.borderColors = resolvedBorderColors;

    // Notify children to re-evaluate their mode
    // group changed
    if (m_accessibilityGroupChanged) {
      m_accessibilityGroupChanged = false;
      for (auto& child : m_children) {
        child->onParentAccessibilityGroupChanged();
      }
    }
  }

  /**
   * @brief Returns the hit rectangle for this component, adjusted by the
   *        hitSlop values from the props. This rectangle is used for touch
   *        event detection.
   * @return The adjusted hit rectangle.
   */
  facebook::react::Rect getHitRect() const {
    facebook::react::Point origin = {0, 0};
    auto size = m_layoutMetrics.frame.size;
    if (m_props) {
      origin -= {m_props->hitSlop.left, m_props->hitSlop.top};
      size +=
          {m_props->hitSlop.right + m_props->hitSlop.left,
           m_props->hitSlop.top + m_props->hitSlop.bottom};
    }
    return {origin, size};
  };

  facebook::react::Transform getTransform() {
    return m_transform;
  }

 private:
  facebook::react::Transform m_transform =
      facebook::react::Transform::Identity();
  // Track border widths separately for change detection.
  // Border colors are tracked via m_oldBorderMetrics.borderColors
  // with blackColor() as default, while resolveBorderMetrics() uses
  // undefined color, so we override it after assignment.
  facebook::react::BorderWidths m_oldLayoutBorderWidths{};

  static bool isValidOuterShadow(const facebook::react::BoxShadow& shadow) {
    if (shadow.inset) {
      return false;
    }
    if (!shadow.color) {
      return shadow.offsetX != 0 || shadow.offsetY != 0 ||
             shadow.blurRadius > 0 || shadow.spreadDistance != 0;
    }
    return facebook::react::alphaFromColor(shadow.color) != 0;
  }

  // ArkUI NODE_CUSTOM_SHADOW limitations:
  // - Only a single shadow per node (setAttribute overwrites), so only the
  //   first outer (non-inset) shadow in the boxShadow array is applied.
  // - Inset shadows are not supported by ArkUI. If boxShadow contains only
  //   inset entries, the fallback path uses legacy shadow props
  //   (shadowColor/shadowOffset/shadowOpacity/shadowRadius).
  // - spreadDistance is not supported by the ArkUI shadow interface.
  // - boxShadow has no separate opacity field; transparency is carried
  //   entirely by color's alpha channel, so opacity is passed as 1.0f
  //   (setShadow multiplies color.alpha * opacity internally).
  // - When boxShadow.color is empty, the default is black.
  void applyShadow(
      ArkUINode& localRoot,
      facebook::react::SharedViewProps const& props,
      float pointScaleFactor) {
    auto outerShadow = std::find_if(
        props->boxShadow.begin(),
        props->boxShadow.end(),
        isValidOuterShadow);
    if (outerShadow != props->boxShadow.end()) {
      facebook::react::SharedColor color = outerShadow->color
          ? outerShadow->color
          : facebook::react::blackColor();
      localRoot.setShadow(
          color,
          {outerShadow->offsetX, outerShadow->offsetY},
          1.0f,
          outerShadow->blurRadius,
          pointScaleFactor);
    } else {
      localRoot.setShadow(
          props->shadowColor,
          props->shadowOffset,
          props->shadowOpacity,
          props->shadowRadius,
          pointScaleFactor);
    }
  }

  void setOpacity(facebook::react::SharedViewProps const& props) {
    auto isOpacityManagedByAnimated = getIgnoredPropKeys().count("opacity") > 0;
    auto isTransformManagedByAnimated =
        getIgnoredPropKeys().count("transform") > 0;
    bool shouldSetOpacity = true;

    if (isOpacityManagedByAnimated) {
      shouldSetOpacity = false;
    }

    auto opacity = props->opacity;
    float validOpacity = std::max(0.0f, std::min((float)opacity, 1.0f));

    for (const auto& filterFunc : props->filter) {
      if (filterFunc.type == facebook::react::FilterType::Opacity &&
          std::holds_alternative<facebook::react::Float>(
              filterFunc.parameters)) {
        validOpacity *= std::get<facebook::react::Float>(filterFunc.parameters);
      }
    }
    validOpacity = std::max(0.0f, std::min(validOpacity, 1.0f));

    auto transform = props->resolveTransform(m_layoutMetrics);
    if (!isTransformManagedByAnimated &&
        props->backfaceVisibility ==
            facebook::react::BackfaceVisibility::Hidden) {
      facebook::react::Vector vec{0, 0, 1, 0};
      auto resVec = transform * vec;
      if (resVec.z < 0.0) {
        validOpacity = 0.0;
        shouldSetOpacity = true;
      }
    }
    if (shouldSetOpacity) {
      this->getLocalRootArkUINode().setOpacity(validOpacity);
    }
  }

 protected:
  /**
   * @brief Called when an accessibility action is triggered on the ArkUI node.
   * Forwards the action to the event emitter if one exists.
   * @param node Unused ArkUI node pointer
   * @param actionName The name of the accessibility action that was triggered
   */
  void onArkUINodeAccessibilityAction(ArkUINode*, const std::string& actionName)
      override {
    if (m_eventEmitter == nullptr) {
      return;
    }
    m_eventEmitter->onAccessibilityAction(actionName);
  }

  /**
   * @brief Handles touch event interception for ArkUI nodes.
   * Determines the hit test mode based on whether this component or its
   * children can handle touch events at the given point.
   * @param event The input event containing touch information
   */
  void onArkUINodeTouchIntercept(const ArkUI_UIInputEvent* event) override {
    auto mode = HitTestMode::HTM_NONE;
    if (this->canHandleTouch()) {
      mode = HitTestMode::HTM_DEFAULT;
    } else if (this->canChildrenHandleTouch()) {
      auto x = OH_ArkUI_PointerEvent_GetX(event);
      auto y = OH_ArkUI_PointerEvent_GetY(event);
      if (this->canSubtreeHandleTouch({x, y})) {
        mode = HitTestMode::HTM_DEFAULT;
      }
    } else if (IsAtLeastApi20()) {
      mode = HitTestMode::HTM_BLOCK_DESCENDANTS;
    }
    OH_ArkUI_PointerEvent_SetInterceptHitTestMode(event, mode);
  }

  /**
   * @brief Returns a component ID string, prioritizing testId, then
   * nativeId, or generates a fallback ID if neither is set.
   * @return The component's ID string.
   */
  std::string getIdFromProps(
      facebook::react::SharedViewProps const& props) const {
    if (props->testId != "") {
      return props->testId;
    } else if (props->nativeId != "") {
      return props->nativeId;
    } else {
      std::ostringstream id;
      id << ShadowNodeT::Name() << "(" << m_tag << ")"
         << "@" << this;
      return id.str();
    }
  }

  /**
   * Tries to get the local root ArkUI node
   * @return Local root ArkUI node if found, null otherwise
   */
  ArkUINode* maybeGetLocalRoot() {
    try {
      return &getLocalRootArkUINode();
    } catch (NoArkUINodeError&) {
      return nullptr;
    }
  }

  struct ViewRawProps {
    bool needsOffscreenAlphaCompositing = false;
    static ViewRawProps getFromDynamic(folly::dynamic value) {
      auto needsOffscreenAlphaCompositing =
          (value.count("needsOffscreenAlphaCompositing") > 0)
          ? value["needsOffscreenAlphaCompositing"].asBool()
          : false;

      return {needsOffscreenAlphaCompositing};
    }
  };

  ViewRawProps m_rawProps;

  SharedConcreteProps m_props = ShadowNodeT::defaultSharedProps();
  SharedConcreteState m_state;
  SharedConcreteEventEmitter m_eventEmitter;
  facebook::react::LayoutMetrics m_stateLayoutMetrics;
  bool m_hasStateLayoutMetrics = false;
  std::optional<facebook::react::Rect> m_boundingBox;
  bool m_isClipping = false;

  /**
   * Converts RN layout direction enum to ArkUI's enum
   * @param layoutDirection
   * @return
   */
  static ArkUI_Direction convertLayoutDirection(
      facebook::react::LayoutDirection layoutDirection) {
    switch (layoutDirection) {
      using facebook::react::LayoutDirection;
      case LayoutDirection::LeftToRight:
        return ArkUI_Direction::ARKUI_DIRECTION_LTR;
      case facebook::react::LayoutDirection::RightToLeft:
        return ArkUI_Direction::ARKUI_DIRECTION_RTL;
      default:
        return ArkUI_Direction::ARKUI_DIRECTION_AUTO;
    }
  }

 private:
  std::vector<std::string> m_accessibilityLabelledBy{};
  std::string m_accessibilityLabel;
  bool m_accessibilityGroup = false;
  bool m_accessibilityGroupChanged = false;
};

/**
 * @brief Transforms a rectangle around a given point using a transform matrix.
 * Calculates the transformed positions of all four corners of the rectangle
 * and returns the bounding rectangle.
 *
 * @param rect The rectangle to transform
 * @param point The point to transform around (pivot point)
 * @param transform The transform matrix to apply
 * @return The bounding rectangle that contains all transformed corner points
 */
inline facebook::react::Rect transformRectAroundPoint(
    const facebook::react::Rect& rect,
    const facebook::react::Point& point,
    const facebook::react::Transform& transform) {
  using Point = facebook::react::Point;

  auto leftTopTransformed =
      (Point({rect.getMinX(), rect.getMinY()}) - point) * transform + point;
  auto leftBottomTransformed =
      (Point({rect.getMinX(), rect.getMaxY()}) - point) * transform + point;
  auto rightTopTransformed =
      (Point({rect.getMaxX(), rect.getMinY()}) - point) * transform + point;
  auto rightBottomTransformed =
      (Point({rect.getMaxX(), rect.getMaxY()}) - point) * transform + point;
  return facebook::react::Rect::boundingRect(
      leftTopTransformed,
      leftBottomTransformed,
      rightTopTransformed,
      rightBottomTransformed);
}
} // namespace rnoh
