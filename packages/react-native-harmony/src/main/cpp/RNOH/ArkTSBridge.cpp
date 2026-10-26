/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ArkTSBridge.h"

#include <boost/exception/diagnostic_information.hpp>
#include <cxxreact/TraceSection.h>
#include <glog/logging.h>
#include <jsi/jsi.h>
#include <react/debug/flags.h>
#include "RNOH/Assert.h"
#include "RNOH/RNOHError.h"

namespace rnoh {

auto physicalPixelsFromNapiValue(napi_env env, napi_value value)
    -> PhysicalPixels {
  ArkJS arkJS(env);
  auto top = arkJS.hasProperty(value, "top")
      ? static_cast<float>(
            arkJS.getDouble(arkJS.getObjectProperty(value, "top")))
      : 0.0f;
  auto left = arkJS.hasProperty(value, "left")
      ? static_cast<float>(
            arkJS.getDouble(arkJS.getObjectProperty(value, "left")))
      : 0.0f;
  auto decorHeight = arkJS.hasProperty(value, "decorHeight")
      ? static_cast<float>(
            arkJS.getDouble(arkJS.getObjectProperty(value, "decorHeight")))
      : 0.0f;
  return {
      top,
      left,
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "width"))),
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "height"))),
      decorHeight,
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "scale"))),
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "fontScale"))),
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "densityDpi"))),
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "xDpi"))),
      static_cast<float>(
          arkJS.getDouble(arkJS.getObjectProperty(value, "yDpi"))),
  };
}

auto displayMetricsFromNapiValue(napi_env env, napi_value value)
    -> DisplayMetrics {
  ArkJS arkJS(env);
  return {
      physicalPixelsFromNapiValue(
          env, arkJS.getObjectProperty(value, "windowPhysicalPixels")),
      physicalPixelsFromNapiValue(
          env, arkJS.getObjectProperty(value, "screenPhysicalPixels")),
  };
}

ArkTSBridge::ArkTSBridge(
    napi_env env,
    NapiRef napiBridgeRef,
    bool isDebugModeEnabled)
    : m_arkJS(ArkJS(env)),
      m_arkTSBridgeRef(std::move(napiBridgeRef)),
      m_isDebugModeEnabled(isDebugModeEnabled) {
  LOG(INFO) << "ArkTSBridge::ArkTSBridge";
}

ArkTSBridge::~ArkTSBridge() noexcept {
  LOG(INFO) << "ArkTSBridge::~ArkTSBridge";
  m_threadGuard.assertThread();
}

void ArkTSBridge::handleError(std::exception_ptr ex) {
  m_threadGuard.assertThread();
  try {
    LOG(ERROR) << boost::diagnostic_information(ex);
    std::rethrow_exception(ex);
  } catch (const RNOHError& e) {
    m_arkJS.getObject(m_arkTSBridgeRef)
        .call("handleError", {m_arkJS.createFromRNOHError(e)});
  } catch (const facebook::jsi::JSError& e) {
    m_arkJS.getObject(m_arkTSBridgeRef)
        .call("handleError", {m_arkJS.createFromJSError(e)});
  } catch (const std::exception& e) {
    m_arkJS.getObject(m_arkTSBridgeRef)
        .call("handleError", {m_arkJS.createFromException(e)});
  } catch (...) {
    m_arkJS.getObject(m_arkTSBridgeRef)
        .call(
            "handleError",
            {m_arkJS.createString(std::string_view("Unknown error"))});
  }
}

void ArkTSBridge::handleError(
    const facebook::react::JsErrorHandler::ProcessedError& error) {
  m_threadGuard.assertThread();
  m_arkJS.getObject(m_arkTSBridgeRef)
      .call(
          "handleError",
          {m_arkJS.createFromParsedError(error),
           m_arkJS.createBoolean(m_isDebugModeEnabled)});
}

auto ArkTSBridge::getDisplayMetrics() -> DisplayMetrics {
  m_threadGuard.assertThread();
  facebook::react::TraceSection s("#RNOH::ArkTSBridge::getDisplayMetrics");
  auto napiBridgeObject = m_arkJS.getReferenceValue(m_arkTSBridgeRef);
  auto methodImpl =
      m_arkJS.getObjectProperty(napiBridgeObject, "getDisplayMetrics");
  auto napiResult = m_arkJS.call<0>(methodImpl, {});
  DisplayMetrics displayMetrics =
      displayMetricsFromNapiValue(m_arkJS.getEnv(), napiResult);
  auto scaleRatioDpiX = displayMetrics.windowPhysicalPixels.scale /
      displayMetrics.windowPhysicalPixels.xDpi;
  auto scaleRatioDpiY = displayMetrics.windowPhysicalPixels.scale /
      displayMetrics.windowPhysicalPixels.xDpi;
  setScaleRatioDpi(scaleRatioDpiX, scaleRatioDpiY);
  return displayMetrics;
}

auto ArkTSBridge::getScaleRatioDpiX() const -> float {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_scaleRatioDpiX;
}

auto ArkTSBridge::getScaleRatioDpiY() const -> float {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_scaleRatioDpiY;
}

void ArkTSBridge::setScaleRatioDpi(float scaleRatioDpiX, float scaleRatioDpiY) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_scaleRatioDpiX = scaleRatioDpiX;
  m_scaleRatioDpiY = scaleRatioDpiY;
}

auto ArkTSBridge::getMetadata(std::string const& name) -> std::string {
  m_threadGuard.assertThread();
  auto napiBridgeObject = m_arkJS.getReferenceValue(m_arkTSBridgeRef);
  auto methodImpl = m_arkJS.getObjectProperty(napiBridgeObject, "getMetadata");
  auto metadataName = m_arkJS.createString(name);
  auto napiResult = m_arkJS.call<1>(methodImpl, {metadataName});
  return m_arkJS.getString(napiResult);
}

} // namespace rnoh
