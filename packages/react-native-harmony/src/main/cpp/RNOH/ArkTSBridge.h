/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jserrorhandler/JsErrorHandler.h>
#include "ArkJS.h"
#include "DisplayMetricsManager.h"
#include "ThreadGuard.h"

namespace rnoh {

class ArkTSErrorHandler {
 public:
  virtual void handleError(std::exception_ptr ex) = 0;

  virtual void handleError(
      const facebook::react::JsErrorHandler::ProcessedError&) = 0;
};

/**
 * @internal
 * @thread: MAIN
 */
class ArkTSBridge final : public DisplayMetricsManager,
                          public ArkTSErrorHandler {
 public:
  using Shared = std::shared_ptr<ArkTSBridge>;

  ArkTSBridge(napi_env env, NapiRef napiBridgeRef, bool isDebugModeEnabled);

  ArkTSBridge(ArkTSBridge const&) = delete;
  ArkTSBridge& operator=(ArkTSBridge const&) = delete;

  ~ArkTSBridge() noexcept;

  void handleError(std::exception_ptr ex) override;

  void handleError(
      const facebook::react::JsErrorHandler::ProcessedError&) override;

  DisplayMetrics getDisplayMetrics() override;

  float getScaleRatioDpiX() const override;

  float getScaleRatioDpiY() const override;

  void setScaleRatioDpi(float scaleRatioDpiX, float scaleRatioDpiY);

  std::string getMetadata(std::string const& name);

 protected:
  ArkJS m_arkJS;
  NapiRef m_arkTSBridgeRef;
  ThreadGuard m_threadGuard;

 private:
  bool m_isDebugModeEnabled;
};
} // namespace rnoh