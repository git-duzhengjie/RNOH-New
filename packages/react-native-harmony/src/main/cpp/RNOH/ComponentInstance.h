/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/**
 * @architecture: C-API
 */
#pragma once
#include <arkui/native_type.h>
#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/EventEmitter.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/core/Props.h>
#include <react/renderer/core/ReactPrimitives.h>
#include <react/renderer/core/State.h>
#include <vector>
#include "RNOH/ArkTSChannel.h"
#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/ArkTSTurboModule.h"
#include "RNOH/DisplayMetricsManager.h"
#include "RNOH/ImageSourceResolver.h"
#include "RNOH/RNInstance.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "RNOH/TouchTarget.h"
#include "RNOH/arkui/ArkUINode.h"
#include "RNOH/arkui/UIInputEventHandler.h"
#include "TaskExecutor/TaskExecutor.h"

namespace rnoh {

class MountingManagerCAPI;

/**
 * @actor RNOH_LIBRARY
 * @architecture: C-API
 *
 * ComponentInstance is an equivalent of RN Android's ViewManager. However,
 * unlike ViewManagers, which are instantiated once per component type,
 * ComponentInstances are created for each individual instance of a component.
 *
 * Consider extending CppComponentInstance, for components implements in C++.
 */
class ComponentInstance
    : public TouchTarget,
      public std::enable_shared_from_this<ComponentInstance> {
 private:
  std::string m_componentName;
  std::unordered_set<std::string> m_nativeResponderBlockOrigins;

  /**
   * @brief Mark whether the current component is a JS responder.
   * @param isJSResponder
   */
  virtual void setIsJSResponder(bool isJSResponder){};

 protected:
  /**
   * @brief React Tag of the component instance.
   */
  using Tag = facebook::react::Tag;

  /**
   * @brief React Handle of the component instance.
   */
  using ComponentHandle = facebook::react::ComponentHandle;

 public:
  /**
   * @brief shared_ptr to the component instance.
   */
  using Shared = std::shared_ptr<ComponentInstance>;

  /**
   * @brief weak_ptr to the component instance.
   */
  using Weak = std::weak_ptr<ComponentInstance>;

  /**
   * @actor RNOH_LIBRARY
   * Component instance registry.
   */
  class Registry {
   public:
    /**
     * @brief weak_ptr to the component instance registry.
     */
    using Weak = std::weak_ptr<Registry>;

    /**
     * @brief find a component instance by its id.
     * @param id
     * @return shared_ptr to the component instance
     */
    virtual ComponentInstance::Shared findById(const std::string& id) const = 0;
  };

  /**
   * @brief set MountingManagerCAPI as a friend class to allow it to access
   */
  friend MountingManagerCAPI;

  /**
   * @actor RNOH_LIBRARY
   * allows ComponentInstances to interact with TurboModules.
   * This is needed in react-native-harmony-gesture-handler.
   */
  struct Dependencies {
    /**
     * @brief shared_ptr to the const Dependencies.
     */
    using Shared = std::shared_ptr<const Dependencies>;

    /**
     * @deprecated: use rnInstance->postMessageToArkTS() instead
     * (latestRNOHVersion: 0.72.28)
     */
    ArkTSChannel::Shared arkTSChannel;

    /**
     * @deprecated: use arkTSMessageHub instead (latestRNOHVersion: 0.72.29)
     */
    ArkTSMessageHub::Shared arkTsMessageHub;
    /**
     * @brief Shared pointer to the ArkTSMessageHub.
     * This is used to communicate with the ArkTS side.
     */
    ArkTSMessageHub::Shared arkTSMessageHub;

    /**
     * @brief weak_ptr to the RNInstance.
     */
    RNInstance::Weak rnInstance;

    /**
     * @brief shared_ptr to the DisplayMetricsManager.
     */
    DisplayMetricsManager::Shared displayMetricsManager;

    /**
     * @brief shared_ptr to the ImageSourceResolver.
     */
    ImageSourceResolver::Shared imageSourceResolver;

    /**
     * @brief weak_ptr to the component instance registry.
     */
    Registry::Weak componentInstanceRegistry;

    /**
     * @brief shared_ptr to the TaskExecutor.
     */
    TaskExecutor::Shared taskExecutor;
  };

  /**
   * @actor RNOH_LIBRARY
   * context of the component instance.
   */
  struct Context {
    /**
     * @brief React tag
     */
    Tag tag;

    /**
     * @brief React componentHandle
     */
    ComponentHandle componentHandle;

    /**
     * @brief componentName
     */
    std::string componentName;

    /**
     * @brief shared_ptr to the Dependencies.
     */
    Dependencies::Shared dependencies;

    /**
     * @brief the UIContext Of ArkUI.
     */
    ArkUINode::Context::Shared arkUINodeContext;
  };

  /**
   * @actor RNOH_LIBRARY
   * Some components (such as SVG elements in RNSVG) don't represent native
   * nodes, but the `ComponentInstance` interface requires an `ArkUINode` to
   * be returned from `getLocalRootArkUINode`. This introduces a
   * `NullArkUINode` class for representing these non-existent nodes.
   */
  class NoArkUINodeError final : public RNOHError {
   public:
    /**
     * @brief Construct a new NoArkUINodeError object
     * @param whatHappened
     * @param howCanItBeFixed
     */
    NoArkUINodeError(
        std::string whatHappened =
            "Called `getLocalRootArkUINode` on a ComponentInstance with no `ArkUINode`s",
        std::vector<std::string> howCanItBeFixed = {});
  };

  /**
   * @brief Get the Local Root ArkUI Node object
   * @return ArkUINode&
   */
  virtual ArkUINode& getLocalRootArkUINode() = 0;

  /**
   * @brief Construct a new Component Instance object
   * @param ctx context of the component instance
   */
  ComponentInstance(Context ctx);

  virtual ~ComponentInstance() = default;

  /**
   * In C++ virtual methods shouldn't be called from constructor. Override this
   * method to bypass this limitation.
   */
  virtual void onCreate() {}

  /**
   * @brief Get the  React Tag of the component instance.
   * @return Tag
   */
  Tag getTag() const {
    return m_tag;
  }

  /**
   * @brief Get the Native component Id
   * @return std::string A string representing the nativeId if it exists;
   * otherwise, an empty string.
   */
  std::string getId() const {
    auto props = getProps();
    if (props == nullptr) {
      return "";
    }
    return props->nativeId;
  }

  /**
   * @return Either the test ID, the native ID, or a generated unique ID.
   */
  virtual std::string getCompId() = 0;

  /**
   * @brief Get the Component Name
   * @return const std::string
   */
  const std::string getComponentName() const {
    return m_componentName;
  }

  /**
   * @brief Get the React Component Handle
   * @return ComponentHandle
   */
  ComponentHandle getComponentHandle() const {
    return m_componentHandle;
  }

  /**
   * @internal
   */
  void insertChild(
      ComponentInstance::Shared childComponentInstance,
      std::size_t index);

  /**
   * @internal
   * @deprecated Do not use it.
   * This method was used by a hack in MountingManager in RNOH 0.72.
   */
  virtual bool checkUpdateBaseNode() {
    return false;
  }

  /**
   * @internal
   */
  void removeChild(ComponentInstance::Shared childComponentInstance);

  /**
   * @brief Get the Props of the component instance.
   * @return facebook::react::Props::Shared
   */
  virtual facebook::react::Props::Shared getProps() const = 0;

  /**
   * @brief Indicates if this component is in an accessibility group (i.e.,
   * should be treated as a group by screen readers).
   * @return True if this is an accessibility group, false otherwise.
   */
  virtual bool getAccessibilityGroup() const = 0;

  /**
   * TODO: change to protected — those methods are intended to be called
   * only by MountingManagerCAPI which is a friend of this class
   * (latestRNOHVersion: 0.72.27)
   */
 public:
  /**
   * @deprecated This is internal method. Do not use it, it will be private in
   * the future (latestRNOHVersion: 0.72.27)
   */
  virtual void setProps(facebook::react::Props::Shared props){};
  /**
   * @deprecated This is internal method. Do not use it, it will be private in
   * the future (latestRNOHVersion: 0.72.27)
   */
  virtual void setState(facebook::react::State::Shared state){};
  /**
   * @deprecated This is internal method. Do not use it, it will be private in
   * the future (latestRNOHVersion: 0.72.27)
   */
  virtual void setLayout(facebook::react::LayoutMetrics layoutMetrics){};
  /**
   * @deprecated This is internal method. Do not use it, it will be private in
   * the future (latestRNOHVersion: 0.72.27)
   */
  virtual void setEventEmitter(
      facebook::react::SharedEventEmitter eventEmitter){};
  /**
   * @deprecated This is internal method. Do not use it, it will be private in
   * the future (latestRNOHVersion: 0.72.27)
   */
  virtual void handleCommand(
      std::string const& commandName,
      folly::dynamic const& args) {
    this->onCommandReceived(commandName, args);
  }
  /**
   * @deprecated This is internal method. Do not use it, it will be private in
   * the future (latestRNOHVersion: 0.72.27)
   */
  virtual void finalizeUpdates() {
    this->onFinalizeUpdates();
  }
  /**
   * @deprecated: This method shouldn't be part of the API. It's an internal
   * method. Deprecated when preparing 0.77 branch for a release.
   * @internal
   * @return Index of this component within its parent.
   */
  virtual std::size_t const getIndex() const {
    return m_index;
  }

  /**
   * @deprecated: This method shouldn't be part of the API. It's an internal
   * method. Deprecated when preparing 0.77 branch for a release.
   * @internal
   * @param index — of this component within its parent.
   */
  virtual void setIndex(std::size_t index) {
    m_index = index;
  }

 public:
  /**
   * @brief This method is necessary to support "aria-labelledby" and
   * "accessibilityLabelledBy" props.
   * @return The accessibility label string.
   */
  virtual const std::string& getAccessibilityLabel() const {
    static const std::string empty = "";
    return empty;
  }

  /**
   * @brief Get the Children
   * @return std::vector<ComponentInstance::Shared> const&
   */
  virtual std::vector<ComponentInstance::Shared> const& getChildren() const {
    return m_children;
  }

  /**
   * @brief Get the Parent
   * @return ComponentInstance::Weak const
   */
  virtual ComponentInstance::Weak const getParent() const {
    return m_parent;
  }

  /**
   * @internal
   */
  virtual void setParent(ComponentInstance::Shared parent) {
    m_parent = parent;
  }

  /**
   * @internal
   * @deprecated: Do not use it. This method is internal. This method was added
   * and deprecated when preparing 0.77 version.
   */
  virtual facebook::react::ShadowView& getShadowView() {
    return m_shadowView;
  }

  /**
   * @internal
   * @deprecated: Do not use it. This method is meant to be internal. This
   * method was added and immediately deprecated when preparing 0.77 version.
   */
  virtual void setShadowView(facebook::react::ShadowView const& shadowView) {
    /**
     * NOOP, because it was NOOP in RNOH 0.72, but some library may override
     * this method. This method could make sense if methods like onPropsChanged,
     * onStateChanged etc. were replaced with with e.g.
     * onShadowViewChanged(newShadowView, oldShadowView).
     */
  }

  /**
   * @brief this method is used to block native responder for a component.
   * @param blocked
   * @param origin recorded origin of the block.
   */
  void setNativeResponderBlocked(
      bool blocked,
      const std::string& origin = "REACT_NATIVE") {
    if (blocked) {
      m_nativeResponderBlockOrigins.insert(origin);
    } else if (
        m_nativeResponderBlockOrigins.find(origin) !=
        m_nativeResponderBlockOrigins.end()) {
      m_nativeResponderBlockOrigins.erase(origin);
    }
    onNativeResponderBlockChange(m_nativeResponderBlockOrigins.size() > 0);
  }

  /**
   * @brief get UIInputEventHandler
   * @return std::weak_ptr<UIInputEventHandler>
   */
  virtual std::weak_ptr<UIInputEventHandler> getUIInputEventHandler() {
    return {};
  }

  /**
   * @brief The JSResponder is the view currently recognized by the JavaScript
   * context as the "responder". It is part of the responder system and
   * represents the view that exclusively handles touch interactions,
   * responsible for receiving and processing user touch events.
   * @return bool
   */
  bool isJSResponder() const override {
    return false;
  };

  /**
   * @brief Get the TouchTarget Tag
   * @return Tag
   */
  Tag getTouchTargetTag() const override {
    return getTag();
  }

  /**
   * @brief Get the Touch Event Emitter
   * @return facebook::react::SharedTouchEventEmitter
   */
  facebook::react::SharedTouchEventEmitter getTouchEventEmitter()
      const override {
    return nullptr;
  }

  /**
   * @brief Get the Touch Target Children
   * @return std::vector<TouchTarget::Shared>
   */
  virtual std::vector<TouchTarget::Shared> getTouchTargetChildren() override {
    auto children = getChildren();
    return std::vector<TouchTarget::Shared>(children.begin(), children.end());
  }

  /**
   * @brief Called when the parent component's group status changes.
   * This allows child components to re-evaluate their mode.
   */
  virtual void onParentAccessibilityGroupChanged() {}

  /**
   * @internal
   */
  void setIgnoredPropKeys(std::unordered_set<std::string> propKeys) {
    m_ignoredPropKeys = std::move(propKeys);
  }

  /**
   * @internal
   */
  std::unordered_set<std::string> const& getIgnoredPropKeys() const {
    return m_ignoredPropKeys;
  }

  /**
   * @deprecated: It's no longer part of the API. Do NOT use it. Use downcasting
   * instead.
   * @internal
   */
  virtual bool isRefreshControlComponentInstance() const {
    return false;
  }

 protected:
  /**
   * @brief insert a child component instance by index
   * @param childComponentInstance
   * @param index
   */
  virtual void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) {}
  /**
   * @brief remove a child component instance
   * @param childComponentInstance
   */
  virtual void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) {}

  /**
   * @brief Called when the component instance is finished updating its props.
   */
  virtual void onFinalizeUpdates() {}

  /**
   * @brief Called when a command is received from JS.
   * @param commandName
   * @param args
   */
  virtual void onCommandReceived(
      std::string const& commandName,
      folly::dynamic const& args) {}

  /**
   * Override this method if your component has scrollable functionality.
   * Temporarily blocking the scrolling is required to support continuous
   * gestures like panning within these scrollable components.
   *
   * @param isBlocked A boolean indicating whether scrolling should be blocked
   * or not.
   */
  virtual void onNativeResponderBlockChange(bool isBlocked) {}

  /**
   * @internal
   */
  bool m_isRadiusSetValid = 0;
  /**
   * @actor RNOH_LIBRARY
   * Identifier of the component instance in the React side.
   */
  Tag m_tag;
  /**
   * @actor RNOH_LIBRARY
   * Handle of the component instance in the React side.
   */
  ComponentHandle m_componentHandle;
  /**
   * @actor RNOH_LIBRARY
   * Vector container for storing child components.
   */
  std::vector<ComponentInstance::Shared> m_children;
  /**
   * @actor RNOH_LIBRARY
   * Weak pointer to the parent component instance.
   */
  ComponentInstance::Weak m_parent;
  /**
   * @actor RNOH_LIBRARY
   * Stores the LayoutMetrics data of the component.
   */
  facebook::react::LayoutMetrics m_layoutMetrics;
  /**
   * @actor RNOH_LIBRARY
   * Stores the Dependencies data of the component.
   */
  Dependencies::Shared m_deps;
  /**
   * @actor RNOH_LIBRARY
   * the UIContext Of ArkUI.
   */
  ArkUINode::Context::Shared m_arkUINodeCtx;
  /**
   * @internal
   * @deprecated Do not use it. It's an internal property. This property was
   * added and immediately deprecated when preparing 0.77 version.
   * Warning: This property isn't always set.
   */
  facebook::react::ShadowView m_shadowView;
  /**
   * @internal
   */
  std::unordered_set<std::string> m_ignoredPropKeys;
  /**
   * @actor RNOH_LIBRARY
   * RNInstance ID
   */
  int m_rnInstanceId = -1;
  /**
   * @deprecated This property is no longer part of the native API. It was
   * deprecated when preparing 0.77 branch for release.
   * @internal
   */
  float m_oldPointScaleFactor = 0.0f;
  /**
   * @deprecated This property is no longer part of the native API. It was
   * deprecated when preparing 0.77 branch for release. This property should be
   * private and in CppComponentInstance
   * @internal
   */
  facebook::react::BorderMetrics m_oldBorderMetrics;
  /**
   * @deprecated: This property should be private.
   * Deprecated when preparing 0.77 version for a release.
   * Index of this component within its parent.
   * @internal
   */
  std::size_t m_index = 0;
};

} // namespace rnoh
