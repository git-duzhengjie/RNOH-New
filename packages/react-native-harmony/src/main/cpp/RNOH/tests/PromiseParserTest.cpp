/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <folly/dynamic.h>
#include <string>
#include <vector>

// PromiseParserTest is currently disabled.
// The production functions are static/anonymous-namespace in ArkTSTurboModule.cpp.
// Cannot be linked without compiling ArkTSTurboModule.cpp, which has
// deep platform deps (TaskExecutor, ArkJS, napi_env).
//
// To re-enable: compile ArkTSTurboModule.cpp with stubs + link ${FOLLY_LIB}
// + forward-declare extern preparePromiseRejectionResult.
