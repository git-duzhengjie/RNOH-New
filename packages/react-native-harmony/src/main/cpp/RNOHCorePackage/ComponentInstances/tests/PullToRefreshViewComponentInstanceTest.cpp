/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// PullToRefreshViewComponentInstance — host testability note
// (RefreshControl implementation; intentionally no tests)
// =============================================================================
//
// PullToRefreshViewComponentInstance
// (RNOHCorePackage/ComponentInstances/PullToRefreshViewComponentInstance.h/.cpp)
// is the native backing of the RefreshControl component. Per
// gtest-module-catalog.md it is classified as "vtable Mock" (it drives several
// ArkUI nodes: RefreshNode, ColumnNode, StackNode, LoadingProgressNode,
// TextNode).
//
// Why there are no host unit tests here:
//   - Every method either constructs/mutates ArkUI nodes (RefreshNode,
//     ColumnNode, StackNode, LoadingProgressNode, TextNode — via the ArkUI C
//     API vtable) or depends on CppComponentInstance<PullToRefreshView
//     ShadowNode> infrastructure (m_props, m_eventEmitter, m_deps,
//     m_layoutMetrics) and on RNInstanceInternal/TaskExecutor
//     (onRefresh posts a JS-thread task) and on the related
//     ScrollViewComponentInstance (getRelatedScrollView,
//     onRefreshNodeOffsetChange, getCurrentOffset, onFinalizeUpdates).
//   - Constructing an instance calls RefreshNode/ColumnNode/... constructors,
//     which call NativeNodeApi / OH_ArkUI_* (setRefreshContent, insertChild,
//     setVisibility, setBorderRadius, ...). That requires the ArkUI C API
//     vtable mock + the device SDK, which are not available in the host test
//     environment. gtest-guide.md §5.4 documents this path, but it needs the
//     SDK installed and is therefore deferred to a CI environment.
//   - Exercising the real methods by #include-ing the production header is not
//     feasible on the host for the same reason (the ArkUI C API surface is
//     referenced throughout), and replicating the logic here would test a copy
//     rather than the production code (silently passing on regression), so no
//     copy is made.
//
// Methods deferred to a vtable-mock CI build (gtest-guide.md §5.4):
//   constructor, createRefreshIndicatorTitle / createOrUpdateRefreshIndicator
//   Title / updateRefreshIndicatorTitleNode, updateRefreshPullDownRatio,
//   onChildInserted/Removed, getLocalRootArkUINode, onPropsChanged, onRefresh,
//   onRefreshStateChanged, getRelatedScrollView, onRefreshNodeOffsetChange,
//   getCurrentOffset, onNativeResponderBlockChange, onFinalizeUpdates,
//   isRefreshControlComponentInstance.
//
// The pure, host-testable surface is the JSIBinder, covered by
// PullToRefreshViewJSIBinderTest.cpp. This file intentionally declares no
// tests.
