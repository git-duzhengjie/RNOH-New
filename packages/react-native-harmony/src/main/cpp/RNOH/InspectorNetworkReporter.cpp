/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "InspectorNetworkReporter.h"

#include <glog/logging.h>
#include <react/networking/NetworkReporter.h>
#include <react/networking/NetworkTypes.h>

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

#include "ArkJS.h"

namespace rnoh {

namespace {

facebook::react::Headers convertHeadersObject(
    ArkJS& arkJS,
    napi_value headersValue) {
  facebook::react::Headers headers;
  if (headersValue == nullptr) {
    return headers;
  }
  auto type = arkJS.getType(headersValue);
  if (type != napi_object) {
    return headers;
  }
  auto props = arkJS.getObjectProperties(headersValue);
  for (auto& [key, value] : props) {
    if (arkJS.getType(value) != napi_string) {
      continue;
    }
    headers[arkJS.getString(key)] = arkJS.getString(value);
  }
  return headers;
}

std::string limitRequestBodySize(std::string body) {
  constexpr std::size_t kMaxBodySize = 1024 * 1024; // 1MB
  auto bodyLength = body.size();
  auto bytesToRead = std::min(bodyLength, kMaxBodySize);

  body.resize(bytesToRead);

  if (bytesToRead < bodyLength) {
    body += "\n... [truncated, showing " + std::to_string(bytesToRead) +
        " of " + std::to_string(bodyLength) + " bytes]";
  }

  return body;
}

std::mutex& responseBuffersMutex() {
  static std::mutex instance;
  return instance;
}

std::unordered_map<std::string, std::string>& responseBuffers() {
  static std::unordered_map<std::string, std::string> instance;
  return instance;
}

} // namespace

namespace network = facebook::react;

napi_value inspectorReporterIsDebuggingEnabled(
    napi_env env,
    napi_callback_info /*info*/) {
  return ArkJS(env).createBoolean(
      network::NetworkReporter::getInstance().isDebuggingEnabled());
}

napi_value inspectorReporterReportRequestStart(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 6);

  network::RequestInfo requestInfo;
  requestInfo.url = arkJS.getString(args[1]);
  requestInfo.httpMethod = arkJS.getString(args[2]);
  requestInfo.headers = convertHeadersObject(arkJS, args[3]);
  if (arkJS.getType(args[4]) == napi_string) {
    requestInfo.httpBody = limitRequestBodySize(arkJS.getString(args[4]));
  } else {
    requestInfo.httpBody = std::string{};
  }

  auto encodedDataLength = static_cast<int>(arkJS.getDouble(args[5]));
  auto requestId = arkJS.getString(args[0]);

  network::NetworkReporter::getInstance().reportRequestStart(
      requestId, requestInfo, encodedDataLength, std::nullopt);
  return arkJS.getUndefined();
}

napi_value inspectorReporterReportConnectionTiming(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 2);

  network::NetworkReporter::getInstance().reportConnectionTiming(
      arkJS.getString(args[0]), convertHeadersObject(arkJS, args[1]));
  return arkJS.getUndefined();
}

napi_value inspectorReporterReportResponseStart(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 5);

  network::ResponseInfo responseInfo;
  responseInfo.url = arkJS.getString(args[1]);
  responseInfo.statusCode = static_cast<uint16_t>(arkJS.getInteger(args[2]));
  responseInfo.headers = convertHeadersObject(arkJS, args[3]);

  auto encodedDataLength = static_cast<int>(arkJS.getDouble(args[4]));
  auto requestId = arkJS.getString(args[0]);

  network::NetworkReporter::getInstance().reportResponseStart(
      requestId, responseInfo, encodedDataLength);
  return arkJS.getUndefined();
}

napi_value inspectorReporterReportDataReceived(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 2);

  auto dataLength = static_cast<int>(arkJS.getDouble(args[1]));

  network::NetworkReporter::getInstance().reportDataReceived(
      arkJS.getString(args[0]), dataLength, std::nullopt);
  return arkJS.getUndefined();
}

napi_value inspectorReporterReportResponseEnd(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 2);

  auto requestId = arkJS.getString(args[0]);
  auto encodedDataLength = static_cast<int>(arkJS.getDouble(args[1]));

  network::NetworkReporter::getInstance().reportResponseEnd(
      requestId, encodedDataLength);

  std::string buffered;
  {
    std::lock_guard<std::mutex> lock(responseBuffersMutex());
    auto& buffers = responseBuffers();
    auto it = buffers.find(requestId);
    if (it != buffers.end()) {
      buffered = std::move(it->second);
      buffers.erase(it);
    }
  }
  if (!buffered.empty()) {
    network::NetworkReporter::getInstance().storeResponseBody(
        requestId, buffered, false);
  }

  return arkJS.getUndefined();
}

napi_value inspectorReporterReportRequestFailed(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 2);

  auto requestId = arkJS.getString(args[0]);
  network::NetworkReporter::getInstance().reportRequestFailed(
      requestId, arkJS.getBoolean(args[1]));

  {
    std::lock_guard<std::mutex> lock(responseBuffersMutex());
    responseBuffers().erase(requestId);
  }

  return arkJS.getUndefined();
}

napi_value inspectorReporterMaybeStoreResponseBody(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 3);

  auto& reporter = network::NetworkReporter::getInstance();
  if (reporter.isDebuggingEnabled()) {
    reporter.storeResponseBody(
        arkJS.getString(args[0]),
        arkJS.getString(args[1]),
        arkJS.getBoolean(args[2]));
  }
  return arkJS.getUndefined();
}

napi_value inspectorReporterMaybeStoreResponseBodyIncremental(
    napi_env env,
    napi_callback_info info) {
  ArkJS arkJS(env);
  auto args = arkJS.getCallbackArgs(info, 2);

  auto& reporter = network::NetworkReporter::getInstance();
  if (reporter.isDebuggingEnabled()) {
    auto requestId = arkJS.getString(args[0]);
    auto chunk = arkJS.getString(args[1]);
    std::lock_guard<std::mutex> lock(responseBuffersMutex());
    responseBuffers()[requestId] += chunk;
  }
  return arkJS.getUndefined();
}

} // namespace rnoh
