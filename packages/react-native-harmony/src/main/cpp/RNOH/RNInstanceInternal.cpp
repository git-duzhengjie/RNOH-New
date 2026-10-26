/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNInstanceInternal.h"

#include <RNOH/modalshrink/GuideLayout.h>
#include <cxxreact/JSBundleType.h>
#include <jsinspector-modern/HostTarget.h>
#include <jsinspector-modern/InspectorFlags.h>
#include <jsireact/JSIExecutor.h>
#include <react/renderer/animations/LayoutAnimationDriver.h>
#include <react/renderer/componentregistry/ComponentDescriptorProvider.h>
#include <react/renderer/componentregistry/ComponentDescriptorRegistry.h>
#include <react/renderer/runtimescheduler/RuntimeSchedulerBinding.h>
#include <react/renderer/scheduler/Scheduler.h>
#include <memory>
#include <mutex>
#include <string_view>
#include "ExceptionStackHandler.h"
#include "FreezeStackCollector.h"
#include "HarmonyTimerRegistry.h"
#include "HiCollieFreezeHandler.h"
#include "JSBigStringHelpers.h"
#include "JSEngineProvider.h"
#include "JSInspectorHostTargetDelegate.h"
#include "RNOH/EventBeat.h"
#include "RNOH/MessageQueueThread.h"
#include "RNOH/Performance/JSFpsMonitor.h"
#ifdef IMAGE_CACHE_ENABLED
#include "RNOH/ImageCache/include/ImageCacheManager.h"
#include "RNOH/ImageCache/include/MemCacheCallback.h"
#endif
#include "RNOH/Performance/RNOHMarker.h"
#include "RNOH/RNFeatureFlags.h"
#include "RNOH/SchedulerDelegate.h"
#include "RNOH/ShadowViewRegistry.h"
#include "RNOH/TurboModuleProvider.h"
#include "TextMeasurer.h"

#if USE_HERMES
#include <hermes/hermes.h>
#include <jsi/instrumentation.h>
#else
#include "RNOH/JSEngine/jsvm/JSVMRuntime.h"
#include "ark_runtime/jsvm.h"
#endif

namespace rnoh {
using namespace facebook;

/**
 * @brief Gets TaskExecutor
 * @return m_taskExecutor
 */
TaskExecutor::Shared RNInstanceInternal::getTaskExecutor() {
  return m_taskExecutor;
}

facebook::react::ContextContainer const&
rnoh::RNInstanceInternal::getContextContainer() const {
  DLOG(INFO) << "RNInstanceInternal::getContextContainer";
  return *m_contextContainer;
}

/**
 * @brief Get turboModule instance
 * @param name turboModuleName
 */
TurboModule::Shared RNInstanceInternal::getTurboModule(
    const std::string& name) {
  auto turboModule = m_turboModuleProvider->getTurboModule(name);
  if (turboModule != nullptr) {
    auto rnohTurboModule = std::dynamic_pointer_cast<TurboModule>(turboModule);
    if (rnohTurboModule != nullptr) {
      return rnohTurboModule;
    } else {
      LOG(ERROR) << "TurboModule '" << name
                 << "' should extend rnoh::TurboModule";
      return nullptr;
    }
  }
  return nullptr;
}

/**
 * @brief Send a message to the ArkTS side in C++
 * @param name Message's name
 * @param payload The parameters passed.
 */
void RNInstanceInternal::postMessageToArkTS(
    const std::string& name,
    folly::dynamic const& payload) {
  m_arkTSChannel->postMessage(name, payload);
}

/**
 * @brief Initialize the runtime environment, scheduling, and context
 */
void RNInstanceInternal::start() {
  DLOG(INFO) << "RNInstanceInternal::start";
  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::CREATE_REACT_CONTEXT_START);
  initialize();

  m_runtimeScheduler = m_reactInstance->getRuntimeScheduler();
  m_contextContainer->insert<std::weak_ptr<react::RuntimeScheduler>>(
      "RuntimeScheduler", m_runtimeScheduler);
  m_contextContainer->insert<std::weak_ptr<RNInstance>>(
      "RNOH::RNInstance", weak_from_this());

  m_turboModuleProvider = createTurboModuleProvider();
  initializeScheduler(m_turboModuleProvider);
  m_reactInstance->getBufferedRuntimeExecutor()(
      [binders = m_globalJSIBinders,
       turboModuleProvider =
           m_turboModuleProvider](facebook::jsi::Runtime& rt) {
        for (auto& binder : binders) {
          binder->createBindings(rt, turboModuleProvider);
        }
      });
  auto textMeasurer =
      m_contextContainer->at<std::shared_ptr<rnoh::TextMeasurer>>(
          "textLayoutManagerDelegate");

  RNOH_ASSERT(textMeasurer != nullptr);

  auto displayMetrics = m_arkTSBridge->getDisplayMetrics().windowPhysicalPixels;
  float fontScale = displayMetrics.fontScale;
  float scale = displayMetrics.scale;

  textMeasurer->setParams(scale, fontScale);
  m_fontRegistry->updateThemeFont();
}

bool RNInstanceInternal::s_hasInitializedFeatureFlags = false;

/**
 * @brief Initialize runtime.
 */
void RNInstanceInternal::initialize() {
  DLOG(INFO) << "RNInstanceInternal::initialize";
#if USE_HERMES
  ExceptionStackHandler::configureCppCrashAppLogMergeOnce();
#endif
  // create a new event dispatcher every time RN is initialized
  m_eventDispatcher = std::make_shared<EventDispatcher>();
  m_jsQueue = std::make_shared<MessageQueueThread>(m_taskExecutor);
  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::REACT_BRIDGE_LOADING_START);
  auto onJSError = [this](
                       jsi::Runtime& runtime,
                       const JsErrorHandler::ProcessedError& error) {
    if (m_isAboutToBeDestroyed && m_isAboutToBeDestroyed->load()) {
      return;
    }
    std::function<void(const std::string)> cb;
    if (error.isFatal) {
      std::shared_ptr<LoadScriptCompletion> completion;
      {
        std::lock_guard<std::mutex> lock(m_loadScriptCompletionMtx);
        m_hasHandledFatalError.store(true);
        completion = m_loadScriptCompletion;
      }
      if (completion) {
        std::lock_guard<std::mutex> lock2(completion->mtx);
        cb = std::move(completion->callback);
        completion->callback = nullptr;
      }
    }
    std::ostringstream msg;
    msg << error.message;
    for (const auto& stackFrame : error.stack) {
      msg << "\n"
          << stackFrame.file.value_or("UNKNOWN_FILE") << " "
          << stackFrame.lineNumber.value_or(-1) << " (" << stackFrame.methodName
          << ")";
    }
    try {
      if (m_taskExecutor->isOnTaskThread(TaskThread::MAIN)) {
        m_arkTSBridge->handleError(error);
      } else {
        m_taskExecutor->runSyncTask(TaskThread::MAIN, [&error, this]() {
          m_arkTSBridge->handleError(error);
        });
      }
    } catch (const std::exception& e) {
      LOG(ERROR) << "handleError failed: " << e.what();
    }
    if (cb) {
      try {
        cb(msg.str());
      } catch (const std::exception& e) {
        LOG(ERROR) << "onFinish callback failed: " << e.what();
      }
    }
    LOG(ERROR) << "Error raised when executing JS: " << msg.str();
  };

  auto jsRuntime = m_jsEngineProvider->createJSRuntime(m_jsQueue);
  auto timerRegistry =
      std::make_unique<HarmonyTimerRegistry>(m_taskExecutor, m_arkTSMessageHub);
  auto rawTimerRegistry = timerRegistry.get();
  auto timerManager =
      std::make_shared<facebook::react::TimerManager>(std::move(timerRegistry));
  rawTimerRegistry->setTimerManager(timerManager);
  m_reactInstance = std::make_shared<facebook::react::ReactInstance>(
      std::move(jsRuntime),
      m_jsQueue,
      timerManager,
      onJSError,
      m_inspectorHostTarget ? m_inspectorHostTarget->getHostTarget().get()
                            : nullptr);
  // INIT_REACT_RUNTIME_START is logged here; INIT_REACT_RUNTIME_STOP is emitted
  // by ReactInstance::loadScript on the JS thread (via ReactMarker).
  // initializeRuntime does not log either marker — do not log STOP here
  // synchronously.
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::INIT_REACT_RUNTIME_START, m_id);
  m_reactInstance->initializeRuntime(
      {},
      // runtime installer, which is run when the runtime
      // is first initialized and provides access to the runtime
      // before the JS code is executed
      [this](facebook::jsi::Runtime& rt) {
        installJSBindings(rt);
#if USE_HERMES
        ExceptionStackHandler::installCppCrashStackBindings(
            rt,
            [weakSelf = m_weakSelf]()
                -> std::optional<ExceptionStackHandler::Context> {
              auto self =
                  std::static_pointer_cast<RNInstanceInternal>(weakSelf.lock());
              if (!self) {
                return std::nullopt;
              }

              return ExceptionStackHandler::Context{
                  self->m_id, self->m_bundlePath, self->m_hspModuleName};
            });

        try {
          auto* hermesRuntime =
              facebook::jsi::castInterface<facebook::hermes::IHermes>(&rt);
          if (hermesRuntime != nullptr) {
            hermesRuntime->registerForProfiling();

            auto* hermesRootAPI =
                facebook::jsi::castInterface<facebook::hermes::IHermesRootAPI>(
                    hermesRuntime->getHermesRootAPI());
            if (hermesRootAPI == nullptr) {
              hermesRootAPI = facebook::jsi::castInterface<
                  facebook::hermes::IHermesRootAPI>(
                  facebook::hermes::makeHermesRootAPI());
            }
            if (hermesRootAPI != nullptr) {
              hermesRootAPI->enableSamplingProfiler();
            }

            FreezeStackCollector::getInstance().registerRuntime(
                {m_id, hermesRuntime});
          }
        } catch (...) {
        }
#endif
      });
  timerManager->setRuntimeExecutor(
      m_reactInstance->getBufferedRuntimeExecutor());

#ifdef IMAGE_CACHE_ENABLED
  if (m_taskExecutor->isTaskThreadAvailable(rnoh::TaskThread::WORKER)) {
    auto nativeResourceManager = getNativeResourceManager();
    m_imageCacheManager = ImageCachePro::ImageCacheManager::Create(
        ImageCachePro::ImageCacheContext(
            nativeResourceManager, m_taskExecutor));
    if (m_imageCacheManager != nullptr) {
      m_imageCacheManager->Init();
    }
  } else {
    DLOG(WARNING) << "ImageCacheManager not created: worker thread unavailable";
  }
#endif
  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::REACT_BRIDGE_LOADING_END);
}

/**
 * @brief Initialize the scheduling information of turboModule
 * @param turboModuleProvider
 */
void RNInstanceInternal::initializeScheduler(
    std::shared_ptr<TurboModuleProvider> turboModuleProvider) {
  DLOG(INFO) << "RNInstanceInternal::initializeScheduler";
  auto runtimeExecutor = m_reactInstance->getBufferedRuntimeExecutor();

  react::EventBeat::Factory eventBeatFactory =
      [uiTicker = m_uiTicker, reactInstance = m_reactInstance](auto ownerBox) {
        return std::make_unique<EventBeat>(
            ownerBox, reactInstance->getRuntimeScheduler(), uiTicker);
      };

  react::ComponentRegistryFactory componentRegistryFactory =
      [registry = m_componentDescriptorProviderRegistry](
          auto eventDispatcher, auto contextContainer) {
        return registry->createComponentDescriptorRegistry(
            {eventDispatcher, contextContainer});
      };

  react::SchedulerToolbox schedulerToolbox{
      .contextContainer = m_contextContainer,
      .componentRegistryFactory = std::move(componentRegistryFactory),
      .runtimeExecutor = runtimeExecutor,
      .eventBeatFactory = std::move(eventBeatFactory)};

  m_animationDriver = std::make_shared<react::LayoutAnimationDriver>(
      runtimeExecutor, m_contextContainer, this);
  auto textMeasurer =
      m_contextContainer->at<std::shared_ptr<rnoh::TextMeasurer>>(
          "textLayoutManagerDelegate");
  m_schedulerDelegate = std::make_unique<rnoh::SchedulerDelegate>(
      m_mountingManager,
      m_taskExecutor,
      m_componentInstancePreallocationRequestQueue,
      textMeasurer);
  m_scheduler = std::make_shared<react::Scheduler>(
      schedulerToolbox, m_animationDriver.get(), m_schedulerDelegate.get());
#ifdef PARALLELIZATION_ON
  m_scheduler->getUIManager()->registerNodeOperationHook(*textMeasurer);
#endif
  m_schedulerDelegate->setScheduler(m_scheduler);
  turboModuleProvider->setScheduler(m_scheduler);
  DLOG(INFO) << "RNInstanceInternal::initializeScheduler::end";
}

void RNInstanceInternal::callJSFunction(
    std::string module,
    std::string method,
    folly::dynamic params) {
  facebook::react::TraceSection s("#RNOH::RNInstanceInternal::callJSFunction");
  m_reactInstance->callFunctionOnModule(
      std::move(module), std::move(method), std::move(params));
}

/**
 * @brief Called when the component's state changes, updates the state.
 * @param env Running environment
 * @param componentName
 * @param tag Component's tag
 * @param newState The state that has changed.
 */
void RNInstanceInternal::updateState(
    napi_env env,
    std::string const& componentName,
    facebook::react::Tag tag,
    napi_value newState) {
  facebook::react::TraceSection s("#RNOH::RNInstanceInternal::updateState");
  if (auto state =
          m_shadowViewRegistry->getFabricState<facebook::react::State>(tag)) {
    m_mutationsToNapiConverter->updateState(
        env, componentName, state, newState);
  }
}

void RNInstanceInternal::onUITick(
    UITicker::Timestamp /*recentVSyncTimestamp*/) {
  facebook::react::TraceSection s("#RNOH::RNInstanceInternal::onUITick");
  if (m_scheduler != nullptr) {
    m_scheduler->animationTick();
  }
}

/**
 * @brief Load the bundle from the buffer.
 * @param bundle
 * @param sourceURL
 * @param onFinish
 */
void RNInstanceInternal::loadScriptFromBuffer(
    std::vector<uint8_t> bundle,
    std::string const sourceURL,
    std::function<void(const std::string)> onFinish) {
  this->loadScript(
      JSBigStringHelpers::fromBuffer(std::move(bundle)), sourceURL, onFinish);
}

/**
 * @brief Loads a bundle from sandboxed path
 * @param fileUrl
 * @param onFinish
 */
void RNInstanceInternal::loadScriptFromFile(
    std::string const fileUrl,
    std::function<void(const std::string)> onFinish) {
  auto jsBundle = JSBigStringHelpers::fromFilePath(fileUrl);
  if (jsBundle) {
    DLOG(INFO) << "Loaded bundle from file";
  }
  this->loadScript(std::move(jsBundle), fileUrl, onFinish);
}

/**
 * @brief Loads a bundle from hap resource.
 * @param rawFileUrl
 * @param onFinish
 */
void RNInstanceInternal::loadScriptFromRawFile(
    std::string const rawFileUrl,
    std::function<void(const std::string)> onFinish) {
  // Magic value used to indicate hermes bytecode
  const uint64_t hermesMagic = 0x1F1903C103BC1FC6;
  try {
    auto jsBundle = JSBigStringHelpers::fromRawFilePath(
        rawFileUrl, m_nativeResourceManager.get());
    uint64_t extractedMagic{};
    if (jsBundle->size() >= sizeof(uint64_t)) {
      const char* source = jsBundle->c_str();
      std::copy(
          source,
          source + sizeof(uint64_t),
          reinterpret_cast<uint8_t*>(&extractedMagic));
    }
    if (jsBundle) {
      DLOG(INFO) << "Loaded bundle from rawfile resource";
    }
    if (extractedMagic == hermesMagic) {
      this->loadScript(std::move(jsBundle), rawFileUrl, onFinish);
    } else {
      // NOTE: JS needs to be null terminated to be handled correctly by hermes.
      // Buffers read from a rawfile aren't null terminated, so we pass the
      // buffer as a string.
      std::string s(jsBundle->c_str(), jsBundle->c_str() + jsBundle->size());
      this->loadScript(
          std::make_unique<facebook::react::JSBigStdString>(std::move(s)),
          rawFileUrl,
          onFinish);
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
    try {
      onFinish(e.what());
    } catch (const std::exception& ex) {
      LOG(ERROR) << "onFinish callback failed: " << ex.what();
    }
  }
}

void RNInstanceInternal::loadScript(
    std::unique_ptr<react::JSBigString const> jsBundle,
    std::string const sourceURL,
    std::function<void(const std::string)> onFinish) {
  if (m_bundlePath.empty()) {
    m_bundlePath = sourceURL;
  }

  if (m_isAboutToBeDestroyed && m_isAboutToBeDestroyed->load()) {
    try {
      onFinish("Instance is being destroyed");
    } catch (const std::exception& e) {
      LOG(ERROR) << "onFinish callback failed: " << e.what();
    }
    if (!m_hasLoggedAppStartupStop.exchange(true)) {
      RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::APP_STARTUP_STOP, m_id);
    }
    return;
  }

  auto completion = std::make_shared<LoadScriptCompletion>();
  bool conflict = false;
  bool fatalAlreadyHandled = false;
  {
    std::lock_guard<std::mutex> lock(m_loadScriptCompletionMtx);
    if (m_hasHandledFatalError.load()) {
      fatalAlreadyHandled = true;
    } else {
      if (m_loadScriptCompletion) {
        std::lock_guard<std::mutex> lock2(m_loadScriptCompletion->mtx);
        if (m_loadScriptCompletion->callback) {
          conflict = true;
        }
      }
      if (!conflict) {
        completion->callback = std::move(onFinish);
        m_loadScriptCompletion = completion;
      }
    }
  }
  if (fatalAlreadyHandled) {
    LOG(ERROR) << "loadScript: instance has handled fatal error, "
               << "onJSError will not fire for future errors, "
               << "calling onFinish to prevent Promise hang";
    try {
      onFinish("Instance has handled fatal error, cannot load script");
    } catch (const std::exception& e) {
      LOG(ERROR) << "onFinish callback failed: " << e.what();
    }
    return;
  }
  if (conflict) {
    try {
      onFinish("Another loadScript is already in progress");
    } catch (const std::exception& e) {
      LOG(ERROR) << "onFinish callback failed: " << e.what();
    }
    return;
  }

  try {
    RNOHMarker::logMarker(
        RNOHMarker::RNOHMarkerId::BUNDLE_SIZE, m_id, jsBundle->size());
    m_reactInstance->loadScript(
        std::move(jsBundle),
        sourceURL,
        nullptr,
        [weakCompletion = std::weak_ptr<LoadScriptCompletion>(completion),
         id = m_id,
         this](jsi::Runtime&) {
          m_hasLoggedAppStartupStop.store(true);
          auto completion = weakCompletion.lock();
          if (completion) {
            std::function<void(const std::string)> cb;
            {
              std::lock_guard<std::mutex> lock(completion->mtx);
              cb = std::move(completion->callback);
              completion->callback = nullptr;
            }
            if (cb) {
              LOG(INFO) << "Script execution finished (id=" << id << ")";
              try {
                cb("");
              } catch (const std::exception& e) {
                LOG(ERROR) << "onFinish callback failed: " << e.what();
              }
            }
          }
        });
  } catch (std::exception const& e) {
    std::function<void(const std::string)> cb;
    {
      std::lock_guard<std::mutex> lock(completion->mtx);
      cb = std::move(completion->callback);
      completion->callback = nullptr;
    }
    if (cb) {
      std::string errorMsg = e.what();
      try {
        std::rethrow_if_nested(e);
      } catch (const std::exception& nested) {
        errorMsg = e.what() + std::string("\n") + nested.what();
      }
      try {
        cb(errorMsg);
      } catch (const std::exception& ex) {
        LOG(ERROR) << "onFinish callback failed: " << ex.what();
      }
    }
  }
}

/**
 * @brief Send an event to the component
 * @param env Running environment
 * @param tag Component's tag
 * @param eventName
 * @param payload The parameters passed.
 */
void RNInstanceInternal::emitComponentEvent(
    napi_env env,
    react::Tag tag,
    std::string eventName,
    napi_value payload) {
  DLOG(INFO) << "RNInstanceInternal::emitComponentEvent";
  facebook::react::TraceSection s(
      "#RNOH::RNInstanceInternal::emitComponentEvent");
  EventEmitRequestHandler::Context ctx{
      .env = env,
      .tag = tag,
      .eventName = std::move(eventName),
      .payload = payload,
      .shadowViewRegistry = m_shadowViewRegistry,
  };

  if (m_eventDispatcher != nullptr) {
    m_eventDispatcher->sendEvent(ctx);
  }

  for (auto& eventEmitRequestHandler : m_eventEmitRequestHandlers) {
    eventEmitRequestHandler->handleEvent(ctx);
  }
}

/**
 * @brief It is called by arkts to monitor memory changes.
 * @param memoryLevel
 */
void RNInstanceInternal::onMemoryLevel(size_t memoryLevel) {
  // Android memory levels are 5, 10, 15, while Ark's are 0, 1, 2
  static const int memoryLevels[] = {5, 10, 15};
  constexpr size_t memoryLevelsCount = std::size(memoryLevels);
  if (memoryLevel >= memoryLevelsCount) {
    LOG(WARNING) << "RNInstanceInternal::onMemoryLevel: invalid memoryLevel="
                 << memoryLevel;
    return;
  }
  facebook::react::TraceSection s("#RNOH::RNInstanceInternal::onMemoryLevel");
  if (m_reactInstance) {
    m_reactInstance->handleMemoryPressureJs(memoryLevels[memoryLevel]);
  }
#ifdef IMAGE_CACHE_ENABLED
  // MemCacheCallback::notifyAll is a static (global) broadcast entry point and
  // does not depend on whether m_imageCacheManager has been created.
  DLOG(INFO) << "ImageCache onMemoryLevel: " << memoryLevel;
  ImageCachePro::MemCacheCallback::notifyAll(memoryLevel);
#endif
}

void RNInstanceInternal::onBackground() {
  DLOG(INFO) << "RNInstanceInternal::onBackground";
  // Match TRIM_MEMORY_BACKGROUND (40). ReactInstance will translate
  // that to a "TRIM_MEMORY_BACKGROUND" cause and trigger runtime GC.
  if (m_shouldEnableBackgroundGC) {
    constexpr int TRIM_MEMORY_BACKGROUND = 40;
    if (m_reactInstance) {
      m_reactInstance->handleMemoryPressureJs(TRIM_MEMORY_BACKGROUND);
    }
  }
#ifdef IMAGE_CACHE_ENABLED
  // MemCacheCallback::notifyAll is a static (global) broadcast entry point and
  // does not depend on whether m_imageCacheManager has been created.
  DLOG(INFO) << "ImageCache onBackground.";
  ImageCachePro::MemCacheCallback::notifyAll(MEMORY_LEVEL_CRITICAL);
#endif
}

void RNInstanceInternal::addArkTSMessageHandler(
    ArkTSMessageHandler::Shared handler) {
  m_arkTSMessageHandlers.push_back(handler);
}

void RNInstanceInternal::removeArkTSMessageHandler(
    ArkTSMessageHandler::Shared handler) {
  for (auto it = m_arkTSMessageHandlers.begin();
       it != m_arkTSMessageHandlers.end();) {
    if (*it == handler) {
      it = m_arkTSMessageHandlers.erase(it);
      break;
    } else {
      ++it;
    }
  }
}

/**
 * @brief Handle the messages sent by arkts
 * @param name Message‘s name
 * @param payload The parameters passed.
 */
void RNInstanceInternal::handleArkTSMessage(
    const std::string& name,
    folly::dynamic const& payload) {
  facebook::react::TraceSection s("RNInstanceInternal::handleArkTSMessage");
  if (name == "CONFIGURATION_UPDATE") {
    onConfigurationChange(payload);
  } else if (name == "BACKGROUND") {
    onBackground();
  } else if (name == "KEYBOARD_VISIBLE") {
    float h = payload.count("keyboardHeight")
        ? static_cast<float>(payload["keyboardHeight"].asDouble())
        : 0.0f;
    facebook::react::GuideLayout::getInstance().setKeyboardHeight(h);
  } else if (name == "KEYBOARD_HIDDEN") {
    facebook::react::GuideLayout::getInstance().setKeyboardHeight(0.0f);
  } else if (name == "STATUS_BAR_HEIGHT") {
    float h = payload.count("statusBarHeight")
        ? static_cast<float>(payload["statusBarHeight"].asDouble())
        : 0.0f;
    facebook::react::GuideLayout::getInstance().setStatusBarHeight(h);
  } else if (name == "DEVICE_TYPE") {
    std::string dt =
        payload.count("deviceType") ? payload["deviceType"].asString() : "";
    facebook::react::GuideLayout::getInstance().setDeviceType(dt);
  }

  for (auto const& arkTSMessageHandler : m_arkTSMessageHandlers) {
    arkTSMessageHandler->handleArkTSMessage(
        {.messageName = name,
         .messagePayload = payload,
         .rnInstance = shared_from_this()});
  }
}

/**
 * @brief Call it when the animation is start
 */
void RNInstanceInternal::onAnimationStarted() {
  facebook::react::TraceSection s("RNInstanceInternal::onAnimationStarted");
  if (m_unsubscribeUITickListener != nullptr) {
    return;
  }
  m_unsubscribeUITickListener =
      m_uiTicker->subscribe([this](auto recentVSyncTimestamp) {
        m_taskExecutor->runTask(
            TaskThread::MAIN,
            [weakSelf = weak_from_this(), recentVSyncTimestamp]() {
              if (auto self = std::dynamic_pointer_cast<RNInstanceInternal>(
                      weakSelf.lock())) {
                self->onUITick(recentVSyncTimestamp);
              }
            });
      });
}

/**
 * @brief Call it when the animation is finished
 */
void RNInstanceInternal::onAllAnimationsComplete() {
  facebook::react::TraceSection s(
      "#RNOH::RNInstanceInternal::onAllAnimationsComplete");
  if (m_unsubscribeUITickListener == nullptr) {
    return;
  }
  m_unsubscribeUITickListener();
  m_unsubscribeUITickListener = nullptr;
}

/**
 * @brief Called from the arkts side for the registration of fonts
 * @param fontFamily
 * @param fontFilePath
 */
void RNInstanceInternal::registerFont(
    std::string const& fontFamily,
    std::string const& fontFilePath) {
  m_fontRegistry->registerFont(fontFamily, fontFilePath);
}

RNInstanceInternal::RNInstanceRNOHMarkerListener::RNInstanceRNOHMarkerListener(
    ArkTSChannel::Weak arkTSChannel)
    : m_arkTSChannel(arkTSChannel) {};

void RNInstanceInternal::RNInstanceRNOHMarkerListener::onMarkerReceived(
    const RNOHMarker::RNOHMarkerId markerId,
    size_t rnInstanceId,
    const std::string& tag,
    double timestamp,
    uint64_t value) {
  auto arkTSChannel = m_arkTSChannel.lock();
  if (!arkTSChannel) {
    return;
  }
  folly::dynamic payload = folly::dynamic::object(
      "markerId", RNOHMarker::harmonyMarkerIdToString(markerId))("tag", tag)(
      "timestamp", timestamp);
  arkTSChannel->postMessage("logRNOHMarker", payload);
}

/**
 * @brief Get the path of the bundle
 * @return m_bundlePath
 */
std::string RNInstanceInternal::getBundlePath() const {
  return m_bundlePath;
}

std::string RNInstanceInternal::getHspModuleName() const {
  return m_hspModuleName;
}

#if USE_HERMES
static void hermesFatalHandler(const std::string& reason) {
  LOG(FATAL) << "Hermes fatal: " << reason;
}

static void installHermesFatalHandler() {
  auto* fatalHandlerInterface =
      facebook::jsi::castInterface<facebook::hermes::ISetFatalHandler>(
          facebook::hermes::makeHermesRootAPI());
  if (fatalHandlerInterface != nullptr) {
    fatalHandlerInterface->setFatalHandler(&hermesFatalHandler);
  }
}
#endif

void RNInstanceInternal::onCreate() {
  m_weakSelf = RNInstance::SafeWeak(shared_from_this(), m_isAboutToBeDestroyed);
#if USE_HERMES
  HiCollieFreezeHandler::registerInstance(
      std::static_pointer_cast<RNInstanceInternal>(shared_from_this()));
  static std::once_flag s_hermesFatalHandlerFlag;
  std::call_once(s_hermesFatalHandlerFlag, installHermesFatalHandler);
#endif
}

void RNInstanceInternal::markSelfAboutToDestroyed() {
  if (m_isAboutToBeDestroyed) {
    m_isAboutToBeDestroyed->store(true);
  }
  std::function<void(const std::string)> cb;
  {
    std::lock_guard<std::mutex> lock(m_loadScriptCompletionMtx);
    if (m_loadScriptCompletion) {
      std::lock_guard<std::mutex> lock2(m_loadScriptCompletion->mtx);
      cb = std::move(m_loadScriptCompletion->callback);
      m_loadScriptCompletion->callback = nullptr;
    }
    m_loadScriptCompletion = nullptr;
  }
  if (cb) {
    try {
      cb("Instance is being destroyed");
    } catch (const std::exception& e) {
      LOG(ERROR) << "onFinish callback failed: " << e.what();
    }
  }
#if USE_HERMES
  FreezeStackCollector::getInstance().unregisterRuntime(m_id);
  HiCollieFreezeHandler::markInstanceDestroyed(m_id);
#endif
}

/**
 * @brief Get the path of the bundle
 * @return m_bundlePath
 */
NativeResourceManager const* RNInstanceInternal::getNativeResourceManager()
    const {
  RNOH_ASSERT(m_nativeResourceManager != nullptr);
  return m_nativeResourceManager.get();
}

/**
 * @brief Unregister instance from inspector
 */
void RNInstanceInternal::unregisterFromInspector() {
  if (m_reactInstance) {
    m_reactInstance->unregisterFromInspector();
  }
}

/**
 * @brief Mainly do operations related to RNInstace
 * @param id rnInstanceId
 * @param contextContainer Running context
 * @param turboModuleFactory
 * @param taskExecutor Task execution-related scheduling.
 * @param componentDescriptorProviderRegistry The registration of the Descriptor
 * of the component
 * @param mutationsToNapiConverter Listen to the changes of mutations.
 * @param eventEmitRequestHandlers Deal with event-related issues.
 * @param globalJSIBinders Global JavaScript bridge.
 * @param uiTicker Monitor changes in the UI and update accordingly.
 * @param shadowViewRegistry View the registration of the shadow tree node.
 * @param arkTSChannel Arkts scheduling and execution.
 * @param mountingManager updating and destroying ComponentInstances
 * @param arkTSMessageHandlers Handle ARKTS messages.
 * @param componentInstancePreallocationRequestQueue Component processing queue
 * @param nativeResourceManager the native implementation of the JavaScript
 * resource manager
 * @param shouldEnableDebugger Control whether you can debug.
 * @param arkTSBridge
 * @param fontRegistry Handle font-related issues
 * @param jsEngineProvider hermes/jsvm
 * @return descriptorWrapper
 */
RNInstanceInternal::RNInstanceInternal(
    size_t id,
    std::shared_ptr<const facebook::react::ContextContainer> contextContainer,
    TurboModuleFactory turboModuleFactory,
    TaskExecutor::Shared taskExecutor,
    std::shared_ptr<facebook::react::ComponentDescriptorProviderRegistry>
        componentDescriptorProviderRegistry,
    MutationsToNapiConverter::Shared mutationsToNapiConverter,
    EventEmitRequestHandlers eventEmitRequestHandlers,
    GlobalJSIBinders globalJSIBinders,
    UITicker::Shared uiTicker,
    ShadowViewRegistry::Shared shadowViewRegistry,
    ArkTSChannel::Shared arkTSChannel,
    MountingManager::Shared mountingManager,
    std::vector<ArkTSMessageHandler::Shared> arkTSMessageHandlers,
    ComponentInstancePreallocationRequestQueue::Shared
        componentInstancePreallocationRequestQueue,
    SharedNativeResourceManager nativeResourceManager,
    bool shouldEnableDebugger,
    ArkTSBridge::Shared arkTSBridge,
    FontRegistry::Shared fontRegistry,
    std::shared_ptr<facebook::react::JSRuntimeFactory> jsEngineProvider,
    std::shared_ptr<InspectorHostTarget> inspectorHostTarget,
    ArkTSMessageHub::Shared arkTSMessageHub,
    std::string hspModuleName)
    : m_id(id),
      m_taskExecutor(std::move(taskExecutor)),
      m_isAboutToBeDestroyed(std::make_shared<std::atomic<bool>>(false)),
      m_contextContainer(std::move(contextContainer)),
      m_mountingManager(std::move(mountingManager)),
      m_componentDescriptorProviderRegistry(
          std::move(componentDescriptorProviderRegistry)),
      m_shadowViewRegistry(std::move(shadowViewRegistry)),
      m_turboModuleFactory(std::move(turboModuleFactory)),
      m_mutationsToNapiConverter(std::move(mutationsToNapiConverter)),
      m_eventEmitRequestHandlers(std::move(eventEmitRequestHandlers)),
      m_globalJSIBinders(std::move(globalJSIBinders)),
      m_uiTicker(std::move(uiTicker)),
      m_nativeResourceManager(std::move(nativeResourceManager)),
      m_shouldEnableDebugger(shouldEnableDebugger),
      m_arkTSMessageHandlers(std::move(arkTSMessageHandlers)),
      m_arkTSMessageHub(std::move(arkTSMessageHub)),
      m_arkTSChannel(std::move(arkTSChannel)),
      m_arkTSBridge(std::move(arkTSBridge)),
      m_componentInstancePreallocationRequestQueue(
          std::move(componentInstancePreallocationRequestQueue)),
      m_inspectorHostDelegate(
          std::make_unique<JSInspectorHostTargetDelegate>(m_arkTSChannel)),
      m_fontRegistry(std::move(fontRegistry)),
      m_jsEngineProvider(std::move(jsEngineProvider)),
      m_inspectorHostTarget(std::move(inspectorHostTarget)),
      m_hspModuleName(hspModuleName) {}

/**
 * @brief The destructor of RNInstanceInternal.
 */
RNInstanceInternal::~RNInstanceInternal() noexcept {
#if USE_HERMES
  FreezeStackCollector::getInstance().unregisterRuntime(m_id);
  HiCollieFreezeHandler::unregisterInstance(m_id);
#endif
  unregisterFromInspector();
};

void RNInstanceInternal::startJSFpsMonitor(
    std::function<void(double)> callback) {
  // Stop existing monitor if present
  if (m_jsFpsMonitor) {
    m_jsFpsMonitor->stop();
    m_jsFpsMonitor.reset();
  }

  m_jsFpsMonitor = JSFpsMonitor::create(m_uiTicker, m_taskExecutor);

  if (callback) {
    m_jsFpsMonitor->setPublishCallback(
        [exec = m_taskExecutor, callback = std::move(callback)](double fps) {
          exec->runTask(TaskThread::MAIN, [callback, fps]() { callback(fps); });
        });
  }
  m_jsFpsMonitor->start();
}

void RNInstanceInternal::stopJsFpsMonitor() {
  if (m_jsFpsMonitor) {
    m_jsFpsMonitor->stop();
    m_jsFpsMonitor->setPublishCallback(nullptr);
    m_jsFpsMonitor.reset();
  }
}

/**
 * @brief Returns JS runtime heap memory usage in bytes
 */
int64_t RNInstanceInternal::getHeapUsageFromJSRuntime(jsi::Runtime& rt) const {
#if USE_HERMES
  try {
    auto heapInfo = rt.instrumentation().getHeapInfo(false);
    auto it = heapInfo.find("hermes_allocatedBytes");
    return (it != heapInfo.end()) ? it->second : 0;
  } catch (...) {
    DLOG(ERROR) << "Hermes getHeapInfo failed";
    return 0;
  }
#else
  try {
    auto* jsvmRuntime = static_cast<jsvm::JSVMRuntime*>(&rt);
    if (jsvmRuntime == nullptr) {
      return 0;
    }
    JSVM_HeapStatistics stats;
    return (jsvmRuntime->getHeapStatistics(&stats) == JSVM_OK)
        ? static_cast<int64_t>(stats.usedHeapSize)
        : 0;
  } catch (...) {
    DLOG(ERROR) << "JSVM getHeapStatistics failed";
    return 0;
  }
#endif
}

int64_t RNInstanceInternal::getJSRuntimeHeapUsage() {
  m_reactInstance->getBufferedRuntimeExecutor()([weakSelf = m_weakSelf](
                                                    jsi::Runtime& rt) {
    if (auto self =
            std::static_pointer_cast<RNInstanceInternal>(weakSelf.lock())) {
      self->m_cachedJSRuntimeHeapUsage = self->getHeapUsageFromJSRuntime(rt);
    }
  });

  // Lamba passed to getBufferedRuntimeExecutor is executed async,
  // so we are returning the previous value and scheduling an update
  return m_cachedJSRuntimeHeapUsage;
}

std::string RNInstanceInternal::captureCurrentJSStackSync() {
#if USE_HERMES
  static constexpr auto MAX_CACHED_STACK_AGE = std::chrono::seconds(10);

  auto& freezeStackCollector = FreezeStackCollector::getInstance();
  auto stackRecord = freezeStackCollector.captureStackFromProfiler(m_id);
  if (!stackRecord.empty()) {
    freezeStackCollector.cacheStackResult(m_id, stackRecord);
    return stackRecord;
  }

  auto cachedResult = freezeStackCollector.getCachedStackWithAge(m_id);
  if (cachedResult.ageMs >= 0 &&
      cachedResult.ageMs <=
          std::chrono::duration_cast<std::chrono::milliseconds>(
              MAX_CACHED_STACK_AGE)
              .count()) {
    return cachedResult.stack;
  }

  return "";
#else
  return "";
#endif
}

} // namespace rnoh
