/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Inspector.h"

#include <deviceinfo.h>
#include <glog/logging.h>
#include <jsinspector-modern/InspectorFlags.h>
#include <jsinspector-modern/InspectorPackagerConnection.h>
#include <chrono>
#include <memory>
#include <vector>
#include "ArkJS.h"
#include "RNOH/ThreadGuard.h"
#include "TaskExecutor/NapiTaskRunner.h"
#include "jsinspector-modern/WebSocketInterfaces.h"

namespace rnoh {

static std::unique_ptr<NapiTaskRunner> mainThreadTaskRunner = nullptr;

namespace jsinspector_modern = facebook::react::jsinspector_modern;

// wraps the ArkTSWebsocket received from ArkTS and
// implements the CPP interface
class ArkTSWebsocket : public jsinspector_modern::IWebSocket {
 public:
  ArkTSWebsocket(const ArkTSWebsocket&) = delete;
  ArkTSWebsocket(ArkTSWebsocket&&) = default;
  ArkTSWebsocket& operator=(const ArkTSWebsocket&) = delete;
  ArkTSWebsocket& operator=(ArkTSWebsocket&&) = default;

  ArkTSWebsocket(napi_env env, NapiRef ref) : m_env(env), m_thisRef(ref) {}

  void send(std::string_view message) override {
    m_threadGuard.assertThread();
    ArkJS arkJS(m_env);
    auto websocketObject = arkJS.getObject(m_thisRef);
    auto sendImpl = websocketObject.getProperty("send");
    std::vector<napi_value> args{arkJS.createString(message)};
    arkJS.call(sendImpl, args, arkJS.getReferenceValue(m_thisRef));
  }

  ~ArkTSWebsocket() {
    m_threadGuard.assertThread();
    try {
      ArkJS arkJS(m_env);
      auto websocketObject = arkJS.getObject(m_thisRef);
      auto closeImpl = websocketObject.getProperty("close");
      // Call close() on the ArkTS websocket object.
      // If the socket was already closed (set to undefined in onClose
      // callback), this will fail gracefully and be caught by the exception
      // handler.
      arkJS.call(closeImpl, {}, arkJS.getReferenceValue(m_thisRef));
    } catch (const std::exception& e) {
      LOG(WARNING)
          << "Exception during ArkTSWebsocket cleanup, socket may already be closed: "
          << e.what();
    }
  }

 private:
  napi_env m_env;
  NapiRef m_thisRef;
  ThreadGuard m_threadGuard{};
};

// wraps the InspectorPackagerConnectionDelegate received from ArkTS and
// implements the CPP interface
class ArkTSConnectionDelegate
    : public jsinspector_modern::InspectorPackagerConnectionDelegate {
 public:
  ArkTSConnectionDelegate(const ArkTSConnectionDelegate&) = delete;
  ArkTSConnectionDelegate(ArkTSConnectionDelegate&&) = delete;
  ArkTSConnectionDelegate& operator=(const ArkTSConnectionDelegate&) = delete;
  ArkTSConnectionDelegate& operator=(ArkTSConnectionDelegate&&) = delete;

  ArkTSConnectionDelegate(napi_env env, NapiRef ref)
      : m_env(env), m_thisRef(ref) {}

  napi_value wrapWebSocketDelegate(
      std::weak_ptr<jsinspector_modern::IWebSocketDelegate> weakDelegate) {
    ArkJS arkJS(m_env);
    return arkJS.createObjectBuilder()
        .addProperty(
            "didFailWithError",
            arkJS.createCallback(
                [env = m_env, weakDelegate](std::vector<folly::dynamic> args) {
                  RNOH_ASSERT(args.size() == 2);
                  auto delegate = weakDelegate.lock();
                  if (!delegate) {
                    return ArkJS(env).getUndefined();
                  }
                  auto posixCode = args[0].isNull()
                      ? std::optional<int>{std::nullopt}
                      : args[0].getInt();
                  auto error = args[1].getString();

                  delegate->didFailWithError(posixCode, std::move(error));
                  return ArkJS(env).getUndefined();
                }))
        .addProperty(
            "didReceiveMessage",
            arkJS.createCallback(
                [env = m_env, weakDelegate](std::vector<folly::dynamic> args) {
                  RNOH_ASSERT(args.size() == 1);
                  auto delegate = weakDelegate.lock();
                  if (!delegate) {
                    return ArkJS(env).getUndefined();
                  }
                  const auto& error = args[0].getString();

                  delegate->didReceiveMessage(error);
                  return ArkJS(env).getUndefined();
                }))
        .addProperty(
            "didClose",
            arkJS.createCallback(
                [env = m_env, weakDelegate](std::vector<folly::dynamic>) {
                  auto delegate = weakDelegate.lock();
                  if (!delegate) {
                    return ArkJS(env).getUndefined();
                  }

                  delegate->didClose();
                  return ArkJS(env).getUndefined();
                }))
        .build();
  }

  std::unique_ptr<jsinspector_modern::IWebSocket> connectWebSocket(
      std::string const& url,
      std::weak_ptr<jsinspector_modern::IWebSocketDelegate> weakDelegate)
      override {
    RNOH_ASSERT(
        mainThreadTaskRunner && mainThreadTaskRunner->isOnCurrentThread());
    ArkJS arkJS(m_env);

    auto wrappedDelegate = wrapWebSocketDelegate(weakDelegate);
    auto delegateObject = arkJS.getObject(m_thisRef);

    // call "connectWebSocket" on the ArkTS object
    auto connectWebSocketImpl = delegateObject.getProperty("connectWebSocket");
    std::vector<napi_value> args{arkJS.createString(url), wrappedDelegate};
    auto result = arkJS.call(
        connectWebSocketImpl, args, arkJS.getReferenceValue(m_thisRef));
    auto websocketRef = arkJS.createNapiRef(result);

    return std::unique_ptr<jsinspector_modern::IWebSocket>(
        new ArkTSWebsocket(m_env, std::move(websocketRef)));
  }

  void scheduleCallback(
      std::function<void(void)> callback,
      std::chrono::milliseconds delayMs) override {
    RNOH_ASSERT(mainThreadTaskRunner != nullptr);
    mainThreadTaskRunner->runAsyncTask(
        [callback = std::move(callback), delayMs]() mutable {
          mainThreadTaskRunner->runDelayedTask(
              std::move(callback), delayMs.count());
        });
  }

 private:
  napi_env m_env;
  NapiRef m_thisRef;
};

static napi_value wrapInspectorPackagerConnection(
    napi_env env,
    std::shared_ptr<jsinspector_modern::InspectorPackagerConnection>
        packagerConnection) {
  ArkJS arkJS(env);
  return arkJS.createObjectBuilder()
      .addProperty(
          "isConnected", arkJS.createCallback([env, packagerConnection](auto) {
            return ArkJS(env).createBoolean(packagerConnection->isConnected());
          }))
      .addProperty(
          "connect", arkJS.createCallback([env, packagerConnection](auto) {
            packagerConnection->connect();
            return ArkJS(env).getUndefined();
          }))
      .addProperty(
          "closeQuietly", arkJS.createCallback([env, packagerConnection](auto) {
            packagerConnection->closeQuietly();
            return ArkJS(env).getUndefined();
          }))
      .addProperty(
          "sendEventToAllConnections",
          arkJS.createCallback(
              [env, packagerConnection](std::vector<folly::dynamic> args) {
                RNOH_ASSERT(args.size() == 1);
                auto event = args[0].getString();
                packagerConnection->sendEventToAllConnections(std::move(event));
                return ArkJS(env).getUndefined();
              }))
      .build();
}

napi_value getInspectorPackagerConnection(
    napi_env env,
    napi_callback_info info) {
  if (mainThreadTaskRunner == nullptr) {
    mainThreadTaskRunner =
        std::make_unique<NapiTaskRunner>("RNOH_MAIN_INSPECTOR", env);
  }

  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info);
  RNOH_ASSERT(args.size() == 3);
  auto url = arkJS.getString(args[0]);
  auto app = arkJS.getString(args[1]);
  auto delegateRef = arkJS.createNapiRef(args[2]);
  auto delegate =
      std::make_unique<ArkTSConnectionDelegate>(env, std::move(delegateRef));

  auto deviceName = OH_GetMarketName();
  auto packagerConnection =
      std::make_shared<jsinspector_modern::InspectorPackagerConnection>(
          url, deviceName, app, std::move(delegate));

  return wrapInspectorPackagerConnection(env, packagerConnection);
}

napi_value getInspectorFlagIsFuseboxEnabled(
    napi_env env,
    napi_callback_info info) {
  auto& inspectorFlags = jsinspector_modern::InspectorFlags::getInstance();
  return ArkJS(env).createBoolean(inspectorFlags.getFuseboxEnabled());
}

} // namespace rnoh
