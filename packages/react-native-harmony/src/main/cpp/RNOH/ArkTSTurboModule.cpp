/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ReactCommon/TurboModuleUtils.h>
#include <cxxreact/TraceSection.h>
#include <glog/logging.h>
#include <jsi/JSIDynamic.h>
#include <exception>
#include <iomanip>
#include <sstream>

#include <react/bridging/CallbackWrapper.h>
#include "ArkTSTurboModule.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"

using namespace rnoh;
using namespace facebook;

using IntermediaryArg = ArkJS::IntermediaryArg;
using IntermediaryCallback = ArkJS::IntermediaryCallback;

IntermediaryCallback createIntermediaryCallback(
    std::weak_ptr<react::CallbackWrapper> weakCallback,
    std::shared_ptr<react::CallInvoker> const& jsInvoker,
    std::shared_ptr<CallbackGroup> callbackGroup,
    std::shared_ptr<CallbackGuard> callbackGuard);
const std::vector<facebook::jsi::Value> convertDynamicsToJSIValues(
    facebook::jsi::Runtime& rt,
    const std::vector<folly::dynamic>& dynamics);
facebook::jsi::Value preparePromiseResolverResult(
    facebook::jsi::Runtime& rt,
    const std::vector<folly::dynamic> args);
std::string preparePromiseRejectionResult(
    const std::vector<folly::dynamic> args);
using RNOHPromiseSetupFunctionType =
    std::function<void(jsi::Runtime& rt, std::shared_ptr<RNOHPromise>)>;
static jsi::Value createRNOHPromiseAsJSIValue(
    jsi::Runtime& rt,
    RNOHPromiseSetupFunctionType&& func);
static void rejectTurboModulePromise(
    jsi::Runtime& rt,
    const std::shared_ptr<RNOHPromise>& promise,
    const std::vector<folly::dynamic> args,
    bool useErrorObjectFormat);
static constexpr const char* kPromiseRejectDefaultCode = "EUNSPECIFIED";
static constexpr const char* kPromiseRejectDefaultMessage = "Error not specified.";

ArkTSTurboModule::ArkTSTurboModule(Context ctx, std::string name)
    : m_ctx(ctx), TurboModule(ctx, name) {
  CallbackGlobalRegistry::instance().registerModule(name_, m_callbackGuardRegistry);
}

ArkTSTurboModule::~ArkTSTurboModule() noexcept {
  // 先从全局注册表注销（按 registry 指针精确移除），再释放资源。
  CallbackGlobalRegistry::instance().unregisterModule(name_, m_callbackGuardRegistry.get());

  // releaseAll() 会触发 CallbackGuard::release() → allowRelease() → CallbackWrapper
  // 析构（含 jsi::Function，只能在 JS 线程析构）。TurboModule 可能在非 JS 线程析构，
  // 故调度到 JS 线程执行；jsInvoker 不可用（runtime 已销毁）时直接释放并吞异常。
  auto jsInvoker = m_ctx.jsInvoker;
  if (jsInvoker) {
    auto registry = m_callbackGuardRegistry;
    jsInvoker->invokeAsync([registry]() { registry->releaseAll(); });
  } else {
    try {
      m_callbackGuardRegistry->releaseAll();
    } catch (...) {
      LOG(ERROR) << "Exception in ~ArkTSTurboModule during releaseAll()";
    }
  }

  auto taskExecutor = m_ctx.taskExecutor;
  taskExecutor->runTask(
      m_ctx.turboModuleThread,
      [ref = std::move(m_ctx.arkTSTurboModuleInstanceRef)] {});
}

void ArkTSTurboModule::releaseAllCallbackWrappers() {
  m_callbackGuardRegistry->releaseAll();
}

// calls a TurboModule method and blocks until it returns, returning its result
jsi::Value ArkTSTurboModule::call(
    jsi::Runtime& runtime,
    const std::string& methodName,
    const jsi::Value* jsiArgs,
    size_t argsCount) {
  react::TraceSection s(std::string(
                            "#RNOH::ArkTSTurboModule::call (" + this->name_ +
                            "::" + methodName + ")")
                            .c_str());
  auto args = convertJSIValuesToIntermediaryValues(
      runtime, m_ctx.jsInvoker, jsiArgs, argsCount);
  return jsi::valueFromDynamic(runtime, callSync(methodName, std::move(args)));
}

// the cpp side calls a ArkTs TurboModule method and blocks until it returns,
// returning its result
folly::dynamic ArkTSTurboModule::callSync(
    const std::string& methodName,
    std::vector<IntermediaryArg> args) {
  react::TraceSection s(std::string(
                            "#RNOH::ArkTSTurboModule::callSync (" +
                            this->name_ + "::" + methodName + ")")
                            .c_str());
  auto start = std::chrono::high_resolution_clock::now();

  if (!m_ctx.arkTSTurboModuleInstanceRef) {
    auto errorMsg = "Couldn't find turbo module '" + name_ +
        "' on ArkUI side. Did you link RNPackage that provides this turbo module?";
    LOG(FATAL) << errorMsg;
    throw std::runtime_error(errorMsg);
  }
  folly::dynamic result;
  m_ctx.taskExecutor->runSyncTask(
      m_ctx.turboModuleThread, [ctx = m_ctx, &methodName, &args, &result]() {
        ArkJS arkJS(ctx.env);
        auto napiArgs =
            arkJS.convertIntermediaryValuesToNapiValues(std::move(args));
        auto napiTurboModuleObject =
            arkJS.getObject(ctx.arkTSTurboModuleInstanceRef);
        auto napiResult = napiTurboModuleObject.call(methodName, napiArgs);
        if (napiResult == nullptr) {
          std::string message = "ArkTS TurboModule method '" + methodName +
              "' threw an exception";
          throw std::runtime_error(message);
        }
        result = arkJS.getDynamic(napiResult);
      });
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
  if (duration.count() > 2) {
    LOG(WARNING) << "ArkTSTurboModule::callSync: execution time — "
                 << duration.count()
                 << " ms (" + this->name_ + "::" + methodName + ")";
  }
  return result;
}

namespace {
folly::dynamic extractPromiseValue(const std::vector<folly::dynamic>& args) {
  if (args.empty()) {
    return folly::dynamic();
  }
  return args[0];
}
} // namespace

void ArkTSTurboModule::callAsyncNative(
    const std::string& methodName,
    std::vector<IntermediaryArg> args,
    std::function<void(folly::dynamic)> onSuccess,
    std::function<void(std::string)> onError) {
  react::TraceSection s(std::string(
                            "#RNOH::ArkTSTurboModule::callAsyncNative (" +
                            this->name_ + "::" + methodName + ")")
                            .c_str());
  if (!m_ctx.arkTSTurboModuleInstanceRef) {
    if (onError) {
      m_ctx.taskExecutor->runTask(TaskThread::WORKER, [onError = std::move(onError)]() {
        onError("TurboModule unavailable");
      });
    }
    return;
  }

  m_ctx.taskExecutor->runTask(
      m_ctx.turboModuleThread,
      [ctx = m_ctx,
       methodName,
       args = std::move(args),
       onSuccess = std::move(onSuccess),
       onError = std::move(onError)]() mutable {
        try {
          ArkJS arkJS(ctx.env);
          auto napiArgs =
              arkJS.convertIntermediaryValuesToNapiValues(std::move(args));
          auto napiTurboModuleObject =
              arkJS.getObject(ctx.arkTSTurboModuleInstanceRef);
          auto napiResult = napiTurboModuleObject.call(methodName, napiArgs);
          Promise(ctx.env, napiResult)
              .then([taskExecutor = ctx.taskExecutor,
                     onSuccess = std::move(onSuccess)](
                        std::vector<folly::dynamic> promiseArgs) mutable {
                if (!onSuccess) {
                  return;
                }
                auto result = extractPromiseValue(promiseArgs);
                onSuccess(result);
              })
              .catch_([taskExecutor = ctx.taskExecutor,
                        onError = std::move(onError)](
                           std::vector<folly::dynamic> promiseArgs) mutable {
                if (!onError) {
                  return;
                }
                auto message = preparePromiseRejectionResult(promiseArgs);
                onError(message);
              });
        } catch (const std::exception& e) {
          if (onError) {
            onError(std::string(e.what()));
          }
        }
      });
}

// calls a TurboModule method without blocking and ignores its result
void rnoh::ArkTSTurboModule::scheduleCall(
    facebook::jsi::Runtime& runtime,
    const std::string& methodName,
    const facebook::jsi::Value* jsiArgs,
    size_t argsCount) {
  react::TraceSection s(std::string(
                            "#RNOH::ArkTSTurboModule::scheduleCall (" +
                            this->name_ + "::" + methodName + ")")
                            .c_str());
  if (!m_ctx.arkTSTurboModuleInstanceRef) {
    auto errorMsg = "Couldn't find turbo module '" + name_ +
        "' on ArkUI side. Did you link RNPackage that provides this turbo module?";
    LOG(FATAL) << errorMsg;
    throw std::runtime_error(errorMsg);
  }
  auto args = convertJSIValuesToIntermediaryValues(
      runtime, m_ctx.jsInvoker, jsiArgs, argsCount);
  m_ctx.taskExecutor->runTask(
      m_ctx.turboModuleThread,
      [name = name_,
       methodName,
       args = std::move(args),
       env = m_ctx.env,
       arkTSTurboModuleInstanceRef = m_ctx.arkTSTurboModuleInstanceRef,
       &runtime]() mutable {
        try {
          ArkJS arkJS(env);
          auto napiArgs =
              arkJS.convertIntermediaryValuesToNapiValues(std::move(args));
          auto napiTurboModuleObject =
              arkJS.getObject(arkTSTurboModuleInstanceRef);
          napiTurboModuleObject.call(methodName, napiArgs);
        } catch (const std::exception& e) {
          LOG(ERROR) << "Exception thrown while calling " << name
                     << " TurboModule method " << methodName << ": "
                     << e.what();
        }
      });
}

// calls an async TurboModule method and returns a Promise
jsi::Value ArkTSTurboModule::callAsync(
    jsi::Runtime& runtime,
    const std::string& methodName,
    const jsi::Value* jsiArgs,
    size_t argsCount) {
  react::TraceSection s(std::string(
                            "#RNOH::ArkTSTurboModule::callAsync (" +
                            this->name_ + "::" + methodName + ")")
                            .c_str());
  if (!m_ctx.arkTSTurboModuleInstanceRef) {
    auto errorMsg = "Couldn't find turbo module '" + name_ +
        "' on ArkUI side. Did you link RNPackage that provides this turbo module?";
    LOG(FATAL) << errorMsg;
    throw std::runtime_error(errorMsg);
  }
  auto args = convertJSIValuesToIntermediaryValues(
      runtime, m_ctx.jsInvoker, jsiArgs, argsCount);
  const bool usePromiseRejectErrorObject =
      m_ctx.featureFlagRegistry != nullptr &&
      m_ctx.featureFlagRegistry->isFeatureFlagOn(
          ENABLE_PROMISE_REJECT_ERROR_OBJECT);
  return createRNOHPromiseAsJSIValue(
      runtime,
      [&, args = std::move(args), usePromiseRejectErrorObject](
          jsi::Runtime& runtime2,
          std::shared_ptr<RNOHPromise> jsiPromise) mutable {
        react::LongLivedObjectCollection::get(runtime2).add(jsiPromise);
        m_ctx.taskExecutor->runTask(
            m_ctx.turboModuleThread,
            [name = this->name_,
             methodName,
             args = std::move(args),
             env = m_ctx.env,
             arkTSTurboModuleInstanceRef = m_ctx.arkTSTurboModuleInstanceRef,
             jsInvoker = m_ctx.jsInvoker,
             usePromiseRejectErrorObject,
             &runtime2,
             weakJsiPromise =
                 std::weak_ptr<RNOHPromise>(jsiPromise)]() mutable {
              ArkJS arkJS(env);
              try {
                auto n_promisedResult =
                    arkJS.getObject(arkTSTurboModuleInstanceRef)
                        .call(
                            methodName,
                            arkJS.convertIntermediaryValuesToNapiValues(
                                std::move(args)));
                Promise(env, n_promisedResult)
                    .then(
                        [&runtime2, weakJsiPromise, env, jsInvoker](auto args) {
                          jsInvoker->invokeAsync([&runtime2,
                                                  weakJsiPromise,
                                                  args = std::move(args)]() {
                            auto jsiPromise = weakJsiPromise.lock();
                            if (!jsiPromise) {
                              return;
                            }
                            jsiPromise->resolve(
                                preparePromiseResolverResult(runtime2, args));
                            jsiPromise->allowRelease();
                          });
                        })
                    .catch_([&runtime2, weakJsiPromise, env, jsInvoker, usePromiseRejectErrorObject](
                                auto args) {
                      jsInvoker->invokeAsync([&runtime2,
                                              weakJsiPromise,
                                              usePromiseRejectErrorObject,
                                              args = std::move(args)]() {
                        auto jsiPromise = weakJsiPromise.lock();
                        if (!jsiPromise) {
                          return;
                        }
                        try {
                          rejectTurboModulePromise(runtime2, jsiPromise, std::move(args), usePromiseRejectErrorObject);
                        } catch (const std::exception& e) {
                          if (usePromiseRejectErrorObject) {
                            jsiPromise->reject(kPromiseRejectDefaultCode, e.what());
                          } else {
                            jsiPromise->reject(e.what());
                          }
                        }
                        jsiPromise->allowRelease();
                      });
                    });
              } catch (const std::exception& e) {
                jsInvoker->invokeAsync(
                    [message = std::string(e.what()), weakJsiPromise, usePromiseRejectErrorObject] {
                      auto jsiPromise = weakJsiPromise.lock();
                      if (!jsiPromise) {
                        return;
                      }
                      if (usePromiseRejectErrorObject) {
                        jsiPromise->reject(kPromiseRejectDefaultCode, message);
                      } else {
                        jsiPromise->reject(message);
                      }
                      jsiPromise->allowRelease();
                    });
                return;
              }
            });
      });
}

std::vector<IntermediaryArg>
ArkTSTurboModule::convertJSIValuesToIntermediaryValues(
    jsi::Runtime& runtime,
    std::shared_ptr<react::CallInvoker> jsInvoker,
    const jsi::Value* jsiArgs,
    size_t argsCount) {
  std::vector<IntermediaryArg> args(argsCount);

  std::vector<int> callbackIndices;
  for (int argIdx = 0; argIdx < argsCount; argIdx++) {
    if (jsiArgs[argIdx].isObject()) {
      auto obj = jsiArgs[argIdx].getObject(runtime);
      if (obj.isFunction(runtime)) {
        callbackIndices.push_back(argIdx);
      }
    }
  }

  // >=2 个回调参数时创建 CallbackGroup：
  // 2 个回调 → 互斥模式（如 promise resolve/reject），调用任一回调时释放兄弟（issue #2652）；
  // 3+ 个回调 → 非互斥模式，releaseSiblings() 为空操作。
  std::shared_ptr<CallbackGroup> callbackGroup;
  if (callbackIndices.size() == 2) {
    callbackGroup = std::make_shared<CallbackGroup>(/*exclusive=*/true);
  } else if (callbackIndices.size() > 2) {
    callbackGroup = std::make_shared<CallbackGroup>(/*exclusive=*/false);
  }

  // 创建中间回调
  for (int argIdx = 0; argIdx < argsCount; argIdx++) {
    if (jsiArgs[argIdx].isObject()) {
      auto obj = jsiArgs[argIdx].getObject(runtime);
      if (obj.isFunction(runtime)) {
        auto weakCallbackWrapper = react::CallbackWrapper::createWeak(
            std::move(obj.getFunction(runtime)), runtime, jsInvoker);
        // 注册表和闭包共享同一 shared_ptr，回调调用后注册表副本也反映
        // isReleased()=true，使 compact() 能回收。
        auto callbackGuard = std::make_shared<CallbackGuard>(weakCallbackWrapper);
        m_callbackGuardRegistry->addShared(callbackGuard);
        // 互斥组：添加为兄弟，任一回调被调用时通过 guard->release() 释放所有兄弟。
        // 非互斥组跳过——releaseSiblings() 为空操作，存储兄弟浪费内存。
        if (callbackGroup && callbackGroup->isExclusive()) {
          callbackGroup->addSibling(callbackGuard);
        }
        args[argIdx] = createIntermediaryCallback(
            std::move(weakCallbackWrapper), jsInvoker, callbackGroup,
            callbackGuard);
        continue;
      }
    }
    args[argIdx] = jsi::dynamicFromValue(runtime, jsiArgs[argIdx]);
  }
  return args;
}

IntermediaryCallback createIntermediaryCallback(
    std::weak_ptr<react::CallbackWrapper> weakCallbackWrapper,
    std::shared_ptr<react::CallInvoker> const& jsInvoker,
    std::shared_ptr<CallbackGroup> callbackGroup,
    std::shared_ptr<CallbackGuard> callbackGuard) {
  auto weakInvoker = std::weak_ptr(jsInvoker);

  // GC 兜底：ArkTS 回调被 GC 回收时，napi finalizer 销毁 std::function →
  // gcReleaseTrigger 析构 → callbackGuard->release() → allowRelease()。
  // 无此机制，注册表持有 shared_ptr 会使 ~CallbackGuard 永不触发，
  // 未调用的回调（如被取消的事件监听器）会残留到 TurboModule 析构，造成泄漏。
  // release() 幂等，GC 触发时若回调已被调用，仅为空操作。
  auto gcReleaseTrigger = std::shared_ptr<void>(
      nullptr, [callbackGuard](void*) { callbackGuard->release(); });

  // callbackGuard/callbackGroup 以值（拷贝）捕获，非移动捕获：
  // std::function 可多次调用，移动捕获会使首次调用后外层副本变为 null。
  // gcReleaseTrigger 也以值捕获，确保 std::function 析构时触发 deleter。
  //
  // 内层 lambda 用 weak_ptr 捕获 CallbackWrapper，通过 lock() 判空：
  // TurboModule 析构后或 JS runtime 销毁后，lock() 返回 nullptr，安全跳过。
  // 代价：竞态下已入队的回调可能被丢弃（如 promise 永不 resolve），
  // 但 crash 风险比回调丢失更严重。
  return std::function(
      [weakCallbackWrapper,
       callbackGuard,
       callbackGroup,
       gcReleaseTrigger,
       weakInvoker](std::vector<folly::dynamic> cbArgs) -> void {
        auto jsInvoker = weakInvoker.lock();
        if (!jsInvoker) {
          return;
        }
        jsInvoker->invokeAsync([weakCallbackWrapper,
                                callbackArgs = std::move(cbArgs),
                                callbackGuard,
                                callbackGroup]() {
          auto callbackWrapper = weakCallbackWrapper.lock();
          if (!callbackWrapper) {
            LOG(WARNING)
                << "Couldn't call a callback provided to TurboModule's method. This shouldn't happen.";
            return;
          }
          const auto jsArgs = convertDynamicsToJSIValues(
              callbackWrapper->runtime(), callbackArgs);
          callbackWrapper->callback().call(
              callbackWrapper->runtime(), jsArgs.data(), jsArgs.size());
          if (callbackGroup && callbackGroup->isExclusive()) {
            callbackGuard->release();
            callbackGroup->releaseSiblings();
          }
        });
      });
}

const std::vector<jsi::Value> convertDynamicsToJSIValues(
    jsi::Runtime& rt,
    const std::vector<folly::dynamic>& dynamics) {
  std::vector<jsi::Value> values;
  for (const auto& dynamic : dynamics) {
    values.push_back(jsi::valueFromDynamic(rt, dynamic));
  }
  return values;
}

jsi::Value preparePromiseResolverResult(
    jsi::Runtime& rt,
    const std::vector<folly::dynamic> args) {
  if (args.size() == 0) {
    return jsi::Value::undefined();
  }
  if (args.size() > 1) {
    throw std::invalid_argument("`resolve` accepts only one argument");
  }
  return jsi::valueFromDynamic(rt, args[0]);
}

std::string preparePromiseRejectionResult(
    const std::vector<folly::dynamic> args) {
  if (args.size() == 0) {
    return "";
  }
  if (args.size() > 1) {
    throw std::invalid_argument("`reject` accepts only one argument");
  }

  auto error = args[0];
  if (error.isObject()) {
    auto message = error["message"];
    if (message.isString()) {
      return message.getString();
    }
  } else if (error.isString()) {
    return error.getString();
  }
  throw std::invalid_argument(
      "The type of argument provided `reject` must be string or contain a string 'message' "
      "field. It's going to be used as an error message");
}

struct ParsedPromiseRejection {
  std::string code;
  std::string message;
  folly::dynamic userInfo;
  std::vector<std::string> nativeStackHarmonyLines;
};

static std::string parseRejectionCodeField(const folly::dynamic* codeField) {
  if (codeField == nullptr) {
    return kPromiseRejectDefaultCode;
  }
  if (codeField->isString()) {
    return codeField->getString();
  }
  if (codeField->isNumber()) {
    if (codeField->isInt()) {
      return std::to_string(codeField->asInt());
    }
    std::ostringstream oss;
    // 15 significant digits
    oss << std::setprecision(15) << codeField->asDouble();
    return oss.str();
  }
  throw std::invalid_argument(
      "The `code` field provided to `reject` must be a string or number");
}

static std::string parseRejectionMessageField(
    const folly::dynamic* messageField) {
  if (messageField == nullptr) {
    return kPromiseRejectDefaultMessage;
  }
  if (!messageField->isString()) {
    throw std::invalid_argument(
        "The `message` field provided to `reject` must be a string");
  }
  return messageField->getString();
}

static folly::dynamic parseRejectionUserInfoField(
    const folly::dynamic* userInfoField) {
  if (userInfoField == nullptr || !userInfoField->isObject()) {
    return folly::dynamic::object();
  }
  return *userInfoField;
}

static std::vector<std::string> parseNativeStackHarmonyField(
    const folly::dynamic* stackField) {
  if (stackField == nullptr || !stackField->isString()) {
    return {};
  }
  const auto& stackStr = stackField->getString();
  std::vector<std::string> lines;
  std::istringstream stream(stackStr);
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

static ParsedPromiseRejection parsePromiseRejectionPayload(
    const folly::dynamic& error) {
  ParsedPromiseRejection result;
  result.userInfo = folly::dynamic::object();

  if (!error.isObject()) {
    if (error.isString()) {
      result.code = kPromiseRejectDefaultCode;
      result.message = error.getString();
      return result;
    }
    throw std::invalid_argument(
        "The type of argument provided to `reject` must be string or Object");
  }

  const folly::dynamic* codeField =
      error.count("code") ? &error["code"] : nullptr;
  const folly::dynamic* messageField =
      error.count("message") ? &error["message"] : nullptr;
  const folly::dynamic* userInfoField =
      error.count("userInfo") ? &error["userInfo"] : nullptr;
  const folly::dynamic* stackField = error.count("nativeStackHarmony")
      ? &error["nativeStackHarmony"]
      : nullptr;

  result.code = parseRejectionCodeField(codeField);
  result.message = parseRejectionMessageField(messageField);
  result.userInfo = parseRejectionUserInfoField(userInfoField);
  result.nativeStackHarmonyLines = parseNativeStackHarmonyField(stackField);
  return result;
}

static ParsedPromiseRejection prepareEnhancedPromiseRejectionResult(
    const std::vector<folly::dynamic> args) {
  if (args.size() > 1) {
    throw std::invalid_argument("`reject` accepts only one argument");
  }
  if (args.size() == 0) {
    return ParsedPromiseRejection{
        kPromiseRejectDefaultCode,
        kPromiseRejectDefaultMessage,
        folly::dynamic::object(),
        {}};
  }

  return parsePromiseRejectionPayload(args[0]);
}

static void rejectTurboModulePromise(
    facebook::jsi::Runtime& rt,
    const std::shared_ptr<RNOHPromise>& promise,
    const std::vector<folly::dynamic> args,
    bool useErrorObjectFormat) {
  if (!useErrorObjectFormat) {
    promise->reject(preparePromiseRejectionResult(args));
  } else {
    auto rejectionResult = prepareEnhancedPromiseRejectionResult(args);
    auto jsiUserInfo = jsi::valueFromDynamic(rt, rejectionResult.userInfo);
    const auto& stackLines = rejectionResult.nativeStackHarmonyLines;
    jsi::Array nativeStackHarmony(rt, stackLines.size());
    for (size_t i = 0; i < stackLines.size(); i++) {
      nativeStackHarmony.setValueAtIndex(
          rt, i, jsi::String::createFromUtf8(rt, stackLines[i]));
    }
    promise->reject(
        rejectionResult.code,
        rejectionResult.message,
        jsiUserInfo,
        nativeStackHarmony);
  }
}

void RNOHPromise::reject(const std::string& code, const std::string& message) {
  jsi::Array emptyStack(runtime_, 0);
  jsi::Value emptyUserInfo(runtime_, jsi::Object(runtime_));
  RNOHPromise::reject(code, message, emptyUserInfo, emptyStack);
}

void RNOHPromise::reject(
    const std::string& code,
    const std::string& message,
    const jsi::Value& userInfo,
    const jsi::Array& nativeStackHarmony) {
  auto& rt = runtime_;
  auto errorCtor = rt.global().getPropertyAsFunction(rt, "Error");
  auto error = errorCtor
      .call(rt, jsi::String::createFromUtf8(rt, message))
      .getObject(rt);
  error.setProperty(rt, "message", jsi::String::createFromUtf8(rt, message));
  error.setProperty(rt, "code", jsi::String::createFromUtf8(rt, code));
  error.setProperty(rt, "userInfo", userInfo);
  error.setProperty(rt, "nativeStackHarmony", nativeStackHarmony);
  reject_.call(rt, error);
}

static jsi::Value createRNOHPromiseAsJSIValue(
    jsi::Runtime& rt,
    RNOHPromiseSetupFunctionType&& func) {
  jsi::Function JSPromise = rt.global().getPropertyAsFunction(rt, "Promise");
  jsi::Function fn = jsi::Function::createFromHostFunction(
      rt,
      jsi::PropNameID::forAscii(rt, "fn"),
      2,
      [func = std::move(func)](
          jsi::Runtime& rt2,
          const jsi::Value& /*thisVal*/,
          const jsi::Value* args,
          size_t /*count*/) {
        jsi::Function resolve = args[0].getObject(rt2).getFunction(rt2);
        jsi::Function reject = args[1].getObject(rt2).getFunction(rt2);
        auto wrapper = std::make_shared<RNOHPromise>(
            rt2, std::move(resolve), std::move(reject));
        func(rt2, wrapper);
        return jsi::Value::undefined();
      });

  return JSPromise.callAsConstructor(rt, fn);
}

jsi::Value ArkTSTurboModule::get(
    jsi::Runtime& rt,
    const jsi::PropNameID& name) {
  auto propName = name.utf8(rt);

  if (propName == "releaseAllCallbacks") {
    return jsi::Function::createFromHostFunction(
        rt,
        name,
        0,
        [](jsi::Runtime& /*rt2*/,
           const jsi::Value& /*thisVal*/,
           const jsi::Value* /*args*/,
           size_t /*count*/) -> jsi::Value {
          auto released =
              CallbackGlobalRegistry::instance().releaseAll();
          return jsi::Value(static_cast<double>(released));
        });
  }

  if (propName == "releaseCallbacksByModule") {
    return jsi::Function::createFromHostFunction(
        rt,
        name,
        1,
        [](jsi::Runtime& rt2,
           const jsi::Value& /*thisVal*/,
           const jsi::Value* args,
           size_t count) -> jsi::Value {
          if (count < 1 || !args[0].isString()) {
            return jsi::Value(static_cast<double>(0));
          }
          auto moduleName = args[0].asString(rt2).utf8(rt2);
          auto released =
              CallbackGlobalRegistry::instance().releaseByModule(moduleName);
          return jsi::Value(static_cast<double>(released));
        });
  }

  if (propName == "getPendingCallbackCount") {
    return jsi::Function::createFromHostFunction(
        rt,
        name,
        0,
        [](jsi::Runtime& /*rt2*/,
           const jsi::Value& /*thisVal*/,
           const jsi::Value* /*args*/,
           size_t /*count*/) -> jsi::Value {
          auto pending =
              CallbackGlobalRegistry::instance().getPendingCount();
          return jsi::Value(static_cast<double>(pending));
        });
  }

  return TurboModule::get(rt, name);
}

std::vector<jsi::PropNameID> ArkTSTurboModule::getPropertyNames(
    jsi::Runtime& rt) {
  auto names = TurboModule::getPropertyNames(rt);
  names.push_back(jsi::PropNameID::forAscii(rt, "releaseAllCallbacks"));
  names.push_back(jsi::PropNameID::forAscii(rt, "releaseCallbacksByModule"));
  names.push_back(jsi::PropNameID::forAscii(rt, "getPendingCallbackCount"));
  return names;
}
