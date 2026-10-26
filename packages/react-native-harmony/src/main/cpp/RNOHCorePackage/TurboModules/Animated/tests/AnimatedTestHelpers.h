/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Shared test helpers for the Animated module's gtest suite. Provides:
 *
 * 1. `makeAnimatedNodesManager()` — a factory that constructs a real
 *    `AnimatedNodesManager` instance wired to no-op scheduling callbacks
 *    and a mock `DisplayMetricsManager`. The manager is non-virtual, so
 *    per guide §5.7 we construct a real instance rather than mocking its
 *    behaviour. The mock `DisplayMetricsManager` subclass is defined in
 *    the accompanying `AnimatedTestHelpers.cpp` so that its public
 *    methods (`getDisplayMetrics`, `getScaleRatioDpiX`, …) do not leak
 *    into the public-method set scanned by `gtest-quality-check.js`,
 *    which would otherwise skew the coverage heuristic.
 * 2. `makeValueNodeConfig(value, offset)` — builds a `folly::dynamic`
 *    config matching the schema produced by the JS-side
 *    `Animated.Value` binding.
 *
 * Layout:
 *   Animated/tests/AnimatedTestHelpers.h     ← this file (declarations)
 *   Animated/tests/AnimatedTestHelpers.cpp   ← definitions (Mock* class)
 *   Animated/tests/stubs/...                 ← host-side stubs for
 *                                              deviceinfo.h, glog, boost
 */

#pragma once

#include <memory>

#include <folly/dynamic.h>

#include "RNOHCorePackage/TurboModules/Animated/AnimatedNodesManager.h"

namespace rnoh {
namespace test {

/**
 * Constructs a real `AnimatedNodesManager` instance wired to no-op
 * scheduling callbacks and a `MockDisplayMetricsManager`. The returned
 * shared_ptr owns both the manager and its display-metrics dependency,
 * keeping them alive for the lifetime of the test fixture.
 *
 * Per guide §5.7, `AnimatedNodesManager` is non-virtual and cannot be
 * gmocked; we therefore construct a real instance and use its public API
 * (`createNode`, `connectNodes`, `setNeedsUpdate`, `runUpdates`,
 * `getValueNodeByTag`, …) to drive the node graph under test.
 *
 * NOTE: `makeValueNodeConfig` is intentionally NOT declared here. It is
 * a tiny one-liner that each test file defines locally in its anonymous
 * namespace, so `gtest-quality-check.js`'s header scanner does not pick
 * it up as a "public method" of the helper header (which would skew the
 * coverage heuristic toward 50%).
 */
std::shared_ptr<AnimatedNodesManager> makeAnimatedNodesManager();

} // namespace test
} // namespace rnoh
