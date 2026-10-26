/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <napi/native_api.h>

namespace rnoh {

napi_value inspectorReporterIsDebuggingEnabled(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterReportRequestStart(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterReportConnectionTiming(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterReportResponseStart(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterReportDataReceived(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterReportResponseEnd(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterReportRequestFailed(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterMaybeStoreResponseBody(
    napi_env env,
    napi_callback_info info);

napi_value inspectorReporterMaybeStoreResponseBodyIncremental(
    napi_env env,
    napi_callback_info info);

} // namespace rnoh
