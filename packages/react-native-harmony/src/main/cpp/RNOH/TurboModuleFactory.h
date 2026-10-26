/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <ReactCommon/TurboModule.h>
#include "RNOH/ArkTSTurboModule.h"
#include "RNOH/FeatureFlagRegistry.h"

namespace rnoh {

class TurboModuleProvider;

class TurboModuleFactoryDelegate {
 public:
  using Context = ArkTSTurboModule::Context;
  using SharedTurboModule = std::shared_ptr<facebook::react::TurboModule>;

  virtual ~TurboModuleFactoryDelegate(){};
  virtual SharedTurboModule createTurboModule(
      Context ctx,
      const std::string& name) const = 0;
};

class TurboModuleFactory final {
 public:
  struct ArkTSTurboModuleEnvironment {
    napi_env napiEnv;
    NapiRef arkTSTurboModuleProviderRef;
  };

  using Context = ArkTSTurboModule::Context;
  using SharedTurboModule = std::shared_ptr<facebook::react::TurboModule>;

  TurboModuleFactory(
      std::unordered_map<TaskThread, ArkTSTurboModuleEnvironment>
          arkTSTurboModuleEnvironmentByTaskThread,
      FeatureFlagRegistry::Shared featureFlagRegistry,
      TaskExecutor::Shared,
      std::vector<std::shared_ptr<TurboModuleFactoryDelegate>>,
      std::shared_ptr<ArkTSMessageHub> arkTSMessageHub,
      DisplayMetricsManager::Shared displayMetricsManager);

  ~TurboModuleFactory() noexcept;
  // rule of five, otherwise std::move() will call copy ctor
  TurboModuleFactory(TurboModuleFactory&&) = default;
  TurboModuleFactory(const TurboModuleFactory&) = default;
  TurboModuleFactory& operator=(const TurboModuleFactory&) = default;
  TurboModuleFactory& operator=(TurboModuleFactory&&) = default;

  virtual SharedTurboModule create(
      std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
      const std::string& name,
      std::shared_ptr<EventDispatcher> eventDispatcher,
      std::shared_ptr<MessageQueueThread> jsQueue,
      std::shared_ptr<facebook::react::Scheduler> scheduler,
      RNInstance::SafeWeak instance) const;

 protected:
  std::optional<TaskThread> findArkTSTurboModuleThread(
      const std::string& turboModuleName) const;

  bool hasArkTSTurboModule(
      const std::string& turboModuleName,
      TaskThread thread,
      napi_env env,
      NapiRef const& turboModuleProviderRef) const;

  SharedTurboModule delegateCreatingTurboModule(
      Context ctx,
      const std::string& name) const;

  NapiRef maybeGetArkTsTurboModuleInstanceRef(
      const std::string& name,
      TaskThread thread,
      ArkTSTurboModuleEnvironment arkTSTurboModuleEnv) const;

  ArkTSTurboModuleEnvironment getArkTSTurboModuleEnvironmentByTaskThread(
      TaskThread taskThread) const;

  virtual SharedTurboModule handleUnregisteredModuleRequest(
      Context ctx,
      const std::string& name) const;

  std::unordered_map<TaskThread, ArkTSTurboModuleEnvironment>
      m_arkTSTurboModuleEnvironmentByTaskThread;
  TaskExecutor::Shared m_taskExecutor;
  std::vector<std::shared_ptr<TurboModuleFactoryDelegate>> m_delegates;
  std::shared_ptr<ArkTSMessageHub> m_arkTSMessageHub;
  FeatureFlagRegistry::Shared m_featureFlagRegistry;
  DisplayMetricsManager::Shared m_displayMetricsManager;
};

} // namespace rnoh
