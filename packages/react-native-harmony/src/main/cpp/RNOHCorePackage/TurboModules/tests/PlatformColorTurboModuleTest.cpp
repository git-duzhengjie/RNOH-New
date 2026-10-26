/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Constructor methodMap smoke tests for PlatformColorTurboModule.
//
// Scope (honest): verifies the codegen'd methodMap_ registration — method
// name, argCount, invoker non-null. Does NOT invoke the invoker, so sync vs
// schedule routing is NOT verified (see StatusBarTurboModuleTest.cpp header
// for the follow-up note). NOT a Tier 4b behavior test.
//
// Harness: host stub TaskExecutor (TurboModuleHostMocks.cpp) + napi_* link
// stubs (NapiStubs.cpp), never invoked here.

// Harness: host stub TaskExecutor (stubs/TurboModuleHostMocks.cpp) + napi_* link
// stubs (stubs/NapiStubs.cpp) + folly stubs (stubs/FollyStubs.cpp).
// NapiRef's private ctor accessed via #define private public (no production change).

#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <exception>
#include <array>
#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>

#define private public
#include "RNOHCorePackage/TurboModules/PlatformColorTurboModule.h"
#include "RNOH/ArkTSTurboModule.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#undef private

#include "stubs/TurboModuleHostMocks.h"
#include "MockJSIRuntime.h"

using namespace rnoh;
using facebook::react::TurboModule;
using MockJSIRuntime = facebook::jsi::MockJSIRuntime;

namespace {
class TestablePlatformColorTurboModule : public PlatformColorTurboModule {
 public:
  using PlatformColorTurboModule::PlatformColorTurboModule;
  const std::unordered_map<std::string, TurboModule::MethodMetadata>&
  methodMap() const {
    return methodMap_;
  }
};

ArkTSTurboModule::Context makeStubContext() {
  ArkTSTurboModule::Context ctx;
  ctx.env = reinterpret_cast<napi_env>(0x1);
  NapiRef ref;
  ref.m_env = ctx.env;
  ref.m_ref = std::shared_ptr<napi_ref__>(
      reinterpret_cast<napi_ref__*>(0x1), [](napi_ref__*){});
  ctx.arkTSTurboModuleInstanceRef = std::move(ref);
  ctx.turboModuleThread = TaskThread::MAIN;
  ctx.taskExecutor = std::make_shared<TaskExecutor>(
      ctx.env, std::unique_ptr<AbstractTaskRunner>(nullptr));
  return ctx;
}
} // namespace

TEST(PlatformColorTurboModule, Constructor_RegistersGetSystemColor) {
  TestablePlatformColorTurboModule tm(makeStubContext(), "PlatformColor");
  const auto& mm = tm.methodMap();
  ASSERT_EQ(mm.count("getSystemColor"), 1u);
  EXPECT_EQ(mm.at("getSystemColor").argCount, 1u);
  EXPECT_EQ(mm.size(), 1u);
}

TEST(PlatformColorTurboModule, Constructor_InvokerIsNonNull) {
  TestablePlatformColorTurboModule tm(makeStubContext(), "PlatformColor");
  EXPECT_NE(tm.methodMap().at("getSystemColor").invoker, nullptr);
}

// Invoker routing (review §5): getSystemColor uses ARK_METHOD_METADATA (sync
// -> call -> runSyncTask). Called with count=0 so no jsi::Value is accessed.
TEST(PlatformColorTurboModule, Invoker_GetSystemColor_RoutesSyncCall) {
  TestablePlatformColorTurboModule tm(makeStubContext(), "PlatformColor");
  auto* invoker = tm.methodMap().at("getSystemColor").invoker;
  ASSERT_NE(invoker, nullptr);
  rnoh::testing::resetTurboModuleMockState();
  MockJSIRuntime rt;
  invoker(rt, tm, nullptr, 0);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 0);
}
