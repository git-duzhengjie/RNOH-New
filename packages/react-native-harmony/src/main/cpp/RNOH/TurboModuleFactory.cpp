/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOH/TurboModuleFactory.h"
#include <react/nativemodule/defaults/DefaultTurboModules.h>
#include "Assert.h"
#include "RNOH/Performance/RNOHMarker.h"
#include "RNOH/RNInstance.h"
#include "RNOH/RNOHError.h"
#include "TaskExecutor/TaskExecutor.h"
#include "TurboModuleFactory.h"

using namespace rnoh;
using namespace facebook;

TurboModuleFactory::TurboModuleFactory(
    std::unordered_map<TaskThread, ArkTSTurboModuleEnvironment>
        arkTSTurboModuleEnvironmentByTaskThread,
    FeatureFlagRegistry::Shared featureFlagRegistry,
    TaskExecutor::Shared taskExecutor,
    std::vector<std::shared_ptr<TurboModuleFactoryDelegate>> delegates,
    std::shared_ptr<ArkTSMessageHub> arkTSMessageHub,
    DisplayMetricsManager::Shared displayMetricsManager)
    : m_arkTSTurboModuleEnvironmentByTaskThread(
          std::move(arkTSTurboModuleEnvironmentByTaskThread)),
      m_featureFlagRegistry(std::move(featureFlagRegistry)),
      m_taskExecutor(taskExecutor),
      m_delegates(delegates),
      m_arkTSMessageHub(arkTSMessageHub),
      m_displayMetricsManager(displayMetricsManager) {}

rnoh::TurboModuleFactory::~TurboModuleFactory() noexcept {
  for (auto&& [thread, turboModuleEnv] :
       m_arkTSTurboModuleEnvironmentByTaskThread) {
    if (turboModuleEnv.arkTSTurboModuleProviderRef) {
      m_taskExecutor->runTask(
          thread,
          [ref = std::move(turboModuleEnv.arkTSTurboModuleProviderRef)] {});
    }
  }
}

TurboModuleFactory::SharedTurboModule TurboModuleFactory::create(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const std::string& name,
    std::shared_ptr<EventDispatcher> eventDispatcher,
    std::shared_ptr<MessageQueueThread> jsQueue,
    std::shared_ptr<facebook::react::Scheduler> scheduler,
    RNInstance::SafeWeak instance) const {
  LOG(INFO) << "Creating Turbo Module: " << name;
  auto arkTSTurboModuleThread =
      this->findArkTSTurboModuleThread(name).value_or(TaskThread::JS);
  if (arkTSTurboModuleThread == TaskThread::MAIN) {
    if (auto weakSelf = instance.lock()) {
      RNOHMarker::logMarker(
          RNOHMarker::RNOHMarkerId::TURBO_MODULE_MAIN_THREAD,
          weakSelf->getId());
    }
  }
  auto arkTSTurboModuleEnvironment =
      this->getArkTSTurboModuleEnvironmentByTaskThread(arkTSTurboModuleThread);
  Context ctx{
      {.jsInvoker = jsInvoker,
       .instance = instance.lock(),
       .safeInstance = instance,
       .arkTSMessageHub = m_arkTSMessageHub},
      .env = arkTSTurboModuleEnvironment.napiEnv,
      .arkTSTurboModuleInstanceRef = arkTSTurboModuleThread == TaskThread::JS
          ? NapiRef{}
          : this->maybeGetArkTsTurboModuleInstanceRef(
                name, arkTSTurboModuleThread, arkTSTurboModuleEnvironment),
      .turboModuleThread = arkTSTurboModuleThread,
      .taskExecutor = m_taskExecutor,
      .eventDispatcher = eventDispatcher,
      .jsQueue = jsQueue,
      .scheduler = scheduler,
      .displayMetricsManager = m_displayMetricsManager,
      .featureFlagRegistry = m_featureFlagRegistry};

  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_MODULE_START);
  auto result = this->delegateCreatingTurboModule(ctx, name);
  if (result != nullptr) {
    auto arkTSTurboModule =
        std::dynamic_pointer_cast<const ArkTSTurboModule>(result);
    if (arkTSTurboModule != nullptr && !ctx.arkTSTurboModuleInstanceRef) {
      std::vector<std::string> suggestions = {
          "Have you linked a package that provides this turbo module on the ArkTS side?"};
      if (!m_featureFlagRegistry->isFeatureFlagOn("WORKER_THREAD_ENABLED")) {
        suggestions.push_back(
            "Is this a WorkerTurboModule? If so, it requires the Worker thread to be enabled. Check RNAbility::getRNOHWorkerScriptUrl.");
      }
      RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_MODULE_END);
      throw FatalRNOHError(
          std::string("Couldn't find Turbo Module on the ArkTs side, name: '")
              .append(name),
          suggestions);
    }
    RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_MODULE_END);
    return result;
  }

  // Fall back to upstream default C++ TurboModules (e.g. NativeDOM, feature
  // flags, microtasks, idle-callbacks) before treating the module as
  // unregistered.
  auto defaultModule =
      facebook::react::DefaultTurboModules::getTurboModule(name, jsInvoker);
  if (defaultModule != nullptr) {
    RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_MODULE_END);
    return defaultModule;
  }

  // ArkTS module was created, but CPP module was not found
  if (ctx.arkTSTurboModuleInstanceRef) {
    // NOTE: move the ref to the correct thread to ensure proper cleanup
    m_taskExecutor->runTask(
        arkTSTurboModuleThread,
        [tmRef = std::move(ctx.arkTSTurboModuleInstanceRef)] {});
    std::vector<std::string> suggestions = {
        "Have you linked a package that provides this turbo module on the CPP side?"};
    RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_MODULE_END);
    throw FatalRNOHError(
        std::string("Couldn't find Turbo Module on the ArkTs side, name: '")
            .append(name),
        suggestions);
  }

  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_MODULE_END);

  return this->handleUnregisteredModuleRequest(ctx, name);
}

std::optional<TaskThread> TurboModuleFactory::findArkTSTurboModuleThread(
    const std::string& turboModuleName) const {
  if (m_featureFlagRegistry->isFeatureFlagOn("WORKER_THREAD_ENABLED")) {
    auto workerArkTSTurboModuleEnv =
        this->getArkTSTurboModuleEnvironmentByTaskThread(TaskThread::WORKER);
    if (this->hasArkTSTurboModule(
            turboModuleName,
            TaskThread::WORKER,
            workerArkTSTurboModuleEnv.napiEnv,
            workerArkTSTurboModuleEnv.arkTSTurboModuleProviderRef)) {
      return TaskThread::WORKER;
    }
  }
  auto mainArkTSTurboModuleEnv =
      this->getArkTSTurboModuleEnvironmentByTaskThread(TaskThread::MAIN);
  if (this->hasArkTSTurboModule(
          turboModuleName,
          TaskThread::MAIN,
          mainArkTSTurboModuleEnv.napiEnv,
          mainArkTSTurboModuleEnv.arkTSTurboModuleProviderRef)) {
    return TaskThread::MAIN;
  }
  return std::nullopt;
}

bool TurboModuleFactory::hasArkTSTurboModule(
    const std::string& turboModuleName,
    TaskThread thread,
    napi_env env,
    NapiRef const& arkTSTurboModuleProviderRef) const {
  ArkJS arkJS(env);
  bool result = false;
  m_taskExecutor->runSyncTask(thread, [&] {
    RNOH_ASSERT(arkTSTurboModuleProviderRef);
    result = arkJS.getBoolean(
        arkJS.getObject(arkTSTurboModuleProviderRef)
            .call("hasModule", {arkJS.createString(turboModuleName)}));
  });
  return result;
}

TurboModuleFactory::ArkTSTurboModuleEnvironment
TurboModuleFactory::getArkTSTurboModuleEnvironmentByTaskThread(
    TaskThread taskThread) const {
  auto it = m_arkTSTurboModuleEnvironmentByTaskThread.find(taskThread);
  if (it != m_arkTSTurboModuleEnvironmentByTaskThread.end()) {
    return it->second;
  }
  return {.napiEnv = nullptr, .arkTSTurboModuleProviderRef = {}};
}

TurboModuleFactory::SharedTurboModule
TurboModuleFactory::delegateCreatingTurboModule(
    Context ctx,
    const std::string& name) const {
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::INITIALIZE_MODULE_START, name.c_str());
  for (const auto& delegate : m_delegates) {
    auto result = delegate->createTurboModule(ctx, name);
    if (result != nullptr) {
      RNOHMarker::logMarker(
          RNOHMarker::RNOHMarkerId::INITIALIZE_MODULE_END, name.c_str());
      return result;
    }
  }
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::INITIALIZE_MODULE_END, name.c_str());
  return nullptr;
}

NapiRef TurboModuleFactory::maybeGetArkTsTurboModuleInstanceRef(
    const std::string& name,
    TaskThread thread,
    ArkTSTurboModuleEnvironment arkTSTurboModuleEnv) const {
  VLOG(3) << "TurboModuleFactory::maybeGetArkTsTurboModuleInstanceRef: start";
  RNOH_ASSERT(arkTSTurboModuleEnv.arkTSTurboModuleProviderRef);
  NapiRef result{};
  m_taskExecutor->runSyncTask(
      thread, [tmEnv = arkTSTurboModuleEnv, name, &result]() {
        VLOG(3)
            << "TurboModuleFactory::maybeGetArkTsTurboModuleInstanceRef: started calling hasModule";
        ArkJS arkJS(tmEnv.napiEnv);
        {
          auto result = arkJS.getObject(tmEnv.arkTSTurboModuleProviderRef)
                            .call("hasModule", {arkJS.createString(name)});
          if (!arkJS.getBoolean(result)) {
            return;
          }
        }
        auto n_turboModuleInstance =
            arkJS.getObject(tmEnv.arkTSTurboModuleProviderRef)
                .call("getModule", {arkJS.createString(name)});
        result = arkJS.createNapiRef(n_turboModuleInstance);
      });
  VLOG(3) << "TurboModuleFactory::maybeGetArkTsTurboModuleInstanceRef: stop";
  return result;
}

TurboModuleFactory::SharedTurboModule
TurboModuleFactory::handleUnregisteredModuleRequest(
    Context /*ctx*/,
    const std::string& name) const {
  LOG(WARNING) << "Turbo Module '" << name << "' not found.";
  return nullptr;
}
