/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Test stub for HarmonyOS <deviceinfo.h>. The production header declares
 * `OH_GetSdkApiVersion()` (and other device-info queries) which is only
 * available when linking against the HarmonyOS NDK. RNOH core pulls it in
 * transitively through "RNOH/ApiVersionCheck.h", so a stub is required to
 * compile AnimatedNodesManager on the host.
 *
 * Only the symbol referenced by the production code (`OH_GetSdkApiVersion`)
 * is declared here. The implementation is provided by
 * `deviceinfo_stub.cpp` next to this header.
 */

#ifndef DEVICEINFO_CSDK_H_STUB
#define DEVICEINFO_CSDK_H_STUB

#ifdef __cplusplus
extern "C" {
#endif

int OH_GetSdkApiVersion(void);

#ifdef __cplusplus
}
#endif

#endif
