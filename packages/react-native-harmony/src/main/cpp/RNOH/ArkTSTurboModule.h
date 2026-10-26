/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <ReactCommon/TurboModuleUtils.h>
#include <functional>
#include <jsi/JSIDynamic.h>
#include <jsi/jsi.h>
#include <memory>
#include <react/renderer/scheduler/Scheduler.h>
#include <unordered_map>
#include "TaskExecutor/TaskExecutor.h"

#include "ArkJS.h"
#include "RNOH/DisplayMetricsManager.h"
#include "RNOH/EventDispatcher.h"
#include "RNOH/FeatureFlagRegistry.h"
#include "RNOH/MessageQueueThread.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "RNOH/TurboModule.h"

/**
 * @internal
 * Used by codegen.
 */
#define ARK_METHOD_CALLER(name)                        \
  [](facebook::jsi::Runtime& rt,                       \
     facebook::react::TurboModule& turboModule,        \
     const facebook::jsi::Value* args,                 \
     size_t count) {                                   \
    return static_cast<ArkTSTurboModule&>(turboModule) \
        .call(rt, #name, args, count);                 \
  }

/**
 * @internal
 * Used by codegen.
 */
#define ARK_ASYNC_METHOD_CALLER(name)                  \
  [](facebook::jsi::Runtime& rt,                       \
     facebook::react::TurboModule& turboModule,        \
     const facebook::jsi::Value* args,                 \
     size_t count) {                                   \
    return static_cast<ArkTSTurboModule&>(turboModule) \
        .callAsync(rt, #name, args, count);            \
  }

/**
 * @internal
 * Used by codegen.
 */
#define ARK_SCHEDULE_METHOD_CALLER(name)        \
  [](facebook::jsi::Runtime& rt,                \
     facebook::react::TurboModule& turboModule, \
     const facebook::jsi::Value* args,          \
     size_t count) {                            \
    static_cast<ArkTSTurboModule&>(turboModule) \
        .scheduleCall(rt, #name, args, count);  \
    return facebook::jsi::Value::undefined();   \
  }

/**
 * @internal
 * Used by codegen.
 */
#define ARK_METHOD_METADATA(name, argc)      \
  {                                          \
#name, { argc, ARK_METHOD_CALLER(name) } \
  }

/**
 * @internal
 * Used by codegen.
 */
#define ARK_ASYNC_METHOD_METADATA(name, argc)      \
  {                                                \
#name, { argc, ARK_ASYNC_METHOD_CALLER(name) } \
  }

/**
 * @internal
 * Used by codegen.
 */
#define ARK_SCHEDULE_METHOD_METADATA(name, argc)      \
  {                                                   \
#name, { argc, ARK_SCHEDULE_METHOD_CALLER(name) } \
  }

namespace rnoh {

inline constexpr const char* ENABLE_PROMISE_REJECT_ERROR_OBJECT =
    "ENABLE_PROMISE_REJECT_ERROR_OBJECT";

/**
 * @brief 全局追踪各 TurboModule 的 CallbackGuardRegistry，支持 JS 端按模块名主动释放回调。
 *
 * ArkTSTurboModule 构造时注册、析构时注销。
 * 以 (moduleName, weak_ptr<CallbackGuardRegistry>) 为键区分同模块的不同实例；
 * 使用 weak_ptr 防止锁释放后访问已析构的 registry 导致 UAF（lock() 返回 nullptr 即跳过）。
 */
class CallbackGlobalRegistry {
 public:
  static CallbackGlobalRegistry& instance() {
    static CallbackGlobalRegistry registry;
    return registry;
  }

  void registerModule(
      const std::string& moduleName,
      std::weak_ptr<CallbackGuardRegistry> registry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.emplace(moduleName, std::move(registry));
  }

  void unregisterModule(
      const std::string& moduleName,
      CallbackGuardRegistry* registry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto range = m_entries.equal_range(moduleName);
    for (auto it = range.first; it != range.second;) {
      auto sp = it->second.lock();
      if (sp.get() == registry) {
        it = m_entries.erase(it);
      } else {
        ++it;
      }
    }
  }

  /// 释放指定模块的待处理 CallbackWrapper。@return 本次实际释放的数量。
  /// 锁外释放（先收集 shared_ptr 再统一操作），顺手移除失效 weak_ptr 防止累积。
  size_t releaseByModule(const std::string& moduleName) {
    std::vector<std::shared_ptr<CallbackGuardRegistry>> registries;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto range = m_entries.equal_range(moduleName);
      for (auto it = range.first; it != range.second;) {
        if (auto sp = it->second.lock()) {
          registries.push_back(std::move(sp));
          ++it;
        } else {
          it = m_entries.erase(it);
        }
      }
    }
    size_t released = 0;
    for (auto& registry : registries) {
      released += registry->releaseAll();
    }
    return released;
  }

  /// 释放所有模块的待处理 CallbackWrapper。@return 本次实际释放的数量。
  /// 锁外释放，顺手移除失效 weak_ptr 防止累积。
  size_t releaseAll() {
    std::vector<std::shared_ptr<CallbackGuardRegistry>> registries;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (auto sp = it->second.lock()) {
          registries.push_back(std::move(sp));
          ++it;
        } else {
          it = m_entries.erase(it);
        }
      }
    }
    size_t released = 0;
    for (auto& registry : registries) {
      released += registry->releaseAll();
    }
    return released;
  }

  /// 返回所有模块中待处理的回调数量。并发下返回值为瞬时快照，可能立即过时。
  /// 顺手移除失效 weak_ptr 防止累积。
  size_t getPendingCount() {
    std::vector<std::shared_ptr<CallbackGuardRegistry>> registries;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (auto sp = it->second.lock()) {
          registries.push_back(std::move(sp));
          ++it;
        } else {
          it = m_entries.erase(it);
        }
      }
    }
    size_t total = 0;
    for (auto& registry : registries) {
      total += registry->pendingCount();
    }
    return total;
  }

 private:
  CallbackGlobalRegistry() = default;
  std::mutex m_mutex;
  std::unordered_multimap<std::string, std::weak_ptr<CallbackGuardRegistry>> m_entries;
};

/**
 * @internal
 * Used by codegen.
 */
class ArkTSTurboModule : public TurboModule {
 public:
  struct Context : public TurboModule::Context {
    napi_env env;
    NapiRef arkTSTurboModuleInstanceRef;
    TaskThread turboModuleThread;
    TaskExecutor::Shared taskExecutor;
    std::shared_ptr<EventDispatcher> eventDispatcher;
    std::shared_ptr<MessageQueueThread> jsQueue;
    std::shared_ptr<facebook::react::Scheduler> scheduler;
    DisplayMetricsManager::Shared displayMetricsManager;
    FeatureFlagRegistry::Shared featureFlagRegistry;
  };

  ArkTSTurboModule(Context ctx, std::string name);
  ~ArkTSTurboModule() noexcept override;

  facebook::jsi::Value call(
      facebook::jsi::Runtime& runtime,
      const std::string& methodName,
      const facebook::jsi::Value* args,
      size_t argsCount);

  folly::dynamic callSync(
      const std::string& methodName,
      std::vector<ArkJS::IntermediaryArg> args);

  void callAsyncNative(
      const std::string& methodName,
      std::vector<ArkJS::IntermediaryArg> args,
      std::function<void(folly::dynamic)> onSuccess,
      std::function<void(std::string)> onError = nullptr);

  void scheduleCall(
      facebook::jsi::Runtime& runtime,
      const std::string& methodName,
      const facebook::jsi::Value* args,
      size_t argsCount);

  facebook::jsi::Value callAsync(
      facebook::jsi::Runtime& runtime,
      const std::string& methodName,
      const facebook::jsi::Value* args,
      size_t argsCount);

  std::vector<ArkJS::IntermediaryArg>
  convertJSIValuesToIntermediaryValues(
      facebook::jsi::Runtime& runtime,
      std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
      const facebook::jsi::Value* jsiArgs,
      size_t argsCount);

  /// 释放所有待处理 CallbackWrapper，无需等待 GC。离开含回调页面时主动释放内存。
  void releaseAllCallbackWrappers();

  /**
   * @brief 向 JS 暴露回调管理接口：
   *   - releaseAllCallbacks()：释放所有待处理 CallbackWrapper，返回实际释放数量。
   *   - releaseCallbacksByModule(moduleName)：释放指定模块的，返回实际释放数量。
   *   - getPendingCallbackCount()：返回待处理回调数量。
   *
   * 并发下返回值可能为 0 或过时——回调仍会被释放，只是由其他路径（如析构）抢先完成。
   */
  facebook::jsi::Value get(
      facebook::jsi::Runtime& rt,
      const facebook::jsi::PropNameID& name) override;

  std::vector<facebook::jsi::PropNameID> getPropertyNames(
      facebook::jsi::Runtime& rt) override;

 protected:
  Context m_ctx;

  std::shared_ptr<CallbackGuardRegistry> m_callbackGuardRegistry = std::make_shared<CallbackGuardRegistry>();
};

struct RNOHPromise : public facebook::react::Promise {
  RNOHPromise(
      facebook::jsi::Runtime& rt,
      facebook::jsi::Function resolve,
      facebook::jsi::Function reject)
      : Promise(rt, std::move(resolve), std::move(reject)){};

  void reject(const std::string& message) {
    Promise::reject(message);
  }
  void reject(const std::string& code, const std::string& message);
  void reject(
      const std::string& code,
      const std::string& message,
      const facebook::jsi::Value& userInfo,
      const facebook::jsi::Array& nativeStackHarmony);
};
} // namespace rnoh
