/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Host implementation of `OH_GetSdkApiVersion()` used by the Animated test
 * target. Returns API level 20 so that `RNOH/ApiVersionCheck.h` always
 * reports the modern code path during tests.
 */

#include "deviceinfo.h"

extern "C" int OH_GetSdkApiVersion(void) {
  return 20;
}
