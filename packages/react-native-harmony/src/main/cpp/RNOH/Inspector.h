/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <jsinspector-modern/InspectorInterfaces.h>
#include <napi/native_api.h>
#include <string>

namespace rnoh {

napi_value getInspectorPackagerConnection(
    napi_env env,
    napi_callback_info info);

napi_value getInspectorFlagIsFuseboxEnabled(
    napi_env env,
    napi_callback_info info);

} // namespace rnoh
