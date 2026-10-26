/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <functional>

namespace rnoh::testing {

// Observable state for the stub TaskExecutor (TurboModuleHostMocks.cpp).
// executeLambdas defaults to false (lambdas NOT executed) to avoid crashes
// from stub NAPI returning nullptr. Set to true per-test when the lambda
// body is safe (e.g., callAsyncNative empty-ref path calls onError directly
// without touching NAPI).
struct TurboModuleMockState {
  long runTaskCount = 0;
  long runSyncTaskCount = 0;
  bool executeLambdas = false;
};

inline TurboModuleMockState& turboModuleMockState() {
  static TurboModuleMockState state;
  return state;
}

inline void resetTurboModuleMockState() {
  turboModuleMockState() = TurboModuleMockState{};
}

} // namespace rnoh::testing
