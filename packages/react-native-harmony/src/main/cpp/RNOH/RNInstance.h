/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <rawfile/raw_file_manager.h>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include <cxxreact/Instance.h>
#include <cxxreact/ModuleRegistry.h>
#include <cxxreact/NativeModule.h>
#include <folly/dynamic.h>
#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/scheduler/Scheduler.h>

#include "RNOH/DisplayMetricsManager.h"
#include "RNOH/MutationsToNapiConverter.h"
#include "RNOH/TouchTarget.h"
#include "RNOH/arkui/UIInputEventHandler.h"

namespace rnoh {
class TurboModule;
using MutationsListener = std::function<void(
    MutationsToNapiConverter const&,
    facebook::react::ShadowViewMutationList const& mutations)>;

/**
 * @actor RNOH_LIBRARY
 * @thread: MAIN
 */
class Surface {
 public:
  virtual ~Surface() = default;
  /**
   * @actor RNOH_LIBRARY
   */
  struct LayoutContext {
    facebook::react::Point viewportOffset{};
    bool swapLeftAndRightInRTL{true};

    static LayoutContext from(facebook::react::LayoutContext layoutContext) {
      return {
          .viewportOffset = layoutContext.viewportOffset,
          .swapLeftAndRightInRTL = layoutContext.swapLeftAndRightInRTL,
      };
    }
  };

  using Weak = std::weak_ptr<Surface>;

  virtual LayoutContext getLayoutContext() = 0;
  virtual DisplayMetrics getDisplayMetrics() = 0;
  virtual std::weak_ptr<UIInputEventHandler> getUIInputEventHandler() = 0;
  virtual void setProps(folly::dynamic const& props){};
};

/**
 * @actor RNOH_LIBRARY
 * @thread: MAIN
 * @brief Represents an instance of the JavaScript runtime environment with UI
 * integration capabilities. including JS execution and bridge interactions.
 */
class RNInstance {
  using ContextContainer = facebook::react::ContextContainer;

 public:
  using Weak = std::weak_ptr<RNInstance>;
  /**
   * @actor RNOH_LIBRARY
   * @brief Prevents locking RNInstance if it's about to be destroyed,
   * avoiding crashes when the last shared_ptr is held on a non-JS thread.
   */
  class SafeWeak {
   public:
    SafeWeak() = default;

    SafeWeak(
        const std::weak_ptr<RNInstance>& instance,
        const std::weak_ptr<std::atomic<bool>>& destroyFlag)
        : m_weak(instance), m_destroyFlag(destroyFlag) {}

    std::shared_ptr<RNInstance> lock() const {
      auto flag = m_destroyFlag.lock();
      if (!flag || flag->load()) {
        return nullptr;
      }

      return m_weak.lock();
    }

    bool expired() const {
      return m_weak.expired();
    }

    void reset() {
      m_weak.reset();
    }

    long use_count() const {
      return m_weak.use_count();
    }

   private:
    std::weak_ptr<RNInstance> m_weak;
    std::weak_ptr<std::atomic<bool>> m_destroyFlag;
  };

  virtual ~RNInstance();

  /**
   * @brief Returns the unique ID of the instance.
   * @return Instance identifier.
   */
  virtual size_t getId() = 0;

  /**
   * @brief Returns the context container which holds shared runtime
   * configuration and dependencies.
   * @return A reference to the context container.
   */
  virtual ContextContainer const& getContextContainer() const = 0;

  /**
   * @brief Retrieves a TurboModule by name.
   * @param name The module name.
   * @return A shared pointer to the requested TurboModule.
   */
  virtual std::shared_ptr<TurboModule> getTurboModule(
      const std::string& name) = 0;

  /**
   * @brief Retrieves a typed TurboModule by name.
   * @tparam T The target TurboModule type.
   * @param name The module name.
   * @return A shared pointer to the typed TurboModule if cast is successful.
   */
  template <typename T>
  std::shared_ptr<T> getTurboModule(const std::string& name) {
    auto turboModule = getTurboModule(name);
    return std::dynamic_pointer_cast<T>(turboModule);
  }

  virtual std::string getCacheDir() = 0;
  /**
   * @brief Returns the resource manager associated with this instance.
   * @return Pointer to the NativeResourceManager.
   */
  virtual NativeResourceManager const* getNativeResourceManager() const = 0;

  /**
   * @brief Synchronously updates a view’s properties on the UI thread.
   * @param tag The view tag to update.
   * @param props The new properties to apply.
   */
  virtual void synchronouslyUpdateViewOnUIThread(
      facebook::react::Tag tag,
      folly::dynamic props) = 0;

  /**
   * @brief Sends a message to the ArkTS layer.
   * @param name The message name or event type.
   * @param payload The message data payload.
   */
  virtual void postMessageToArkTS(
      const std::string& name,
      folly::dynamic const& payload) = 0;

  /**
   * @brief Calls a JavaScript function from native code.
   * @param module The module name.
   * @param method The method name.
   * @param params The parameters to pass.
   */
  virtual void callJSFunction(
      std::string module,
      std::string method,
      folly::dynamic params) = 0;

  /**
   * @brief Retrieves a Surface by its root tag.
   * @param rootTag The root tag of the surface.
   * @return Optional weak pointer to the surface if found.
   */
  virtual std::optional<Surface::Weak> getSurfaceByRootTag(
      facebook::react::Tag rootTag) = 0;

  /**
   * @brief Returns the path to the JS bundle.
   * @return The absolute file path of the JS bundle.
   */
  virtual std::string getBundlePath() const = 0;

  virtual std::string getHspModuleName() const = 0;
};

} // namespace rnoh
