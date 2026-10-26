/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Constructor methodMap smoke tests for StatusBarTurboModule.
//
// Scope (honest): these verify the codegen'd methodMap_ registration — method
// names, argCount, and that invoker function pointers are non-null. They do
// NOT invoke the invokers, so they do NOT verify sync (ARK_METHOD_METADATA ->
// ArkTSTurboModule::call) vs schedule (ARK_SCHEDULE_METHOD_METADATA ->
// scheduleCall) routing. Invoker sync/schedule verification needs a JSI
// runtime + a recording TaskExecutor mock + a non-empty NapiRef and is tracked
// as a follow-up; until then this is NOT a Tier 4b behavior test.
//
// Harness: a host stub TaskExecutor (TurboModuleHostMocks.cpp) is linked to
// satisfy ~ArkTSTurboModule's non-virtual taskExecutor->runTask dependency;
// napi_* are no-op link stubs (NapiStubs.cpp) never invoked here.

// Harness: host stub TaskExecutor (stubs/TurboModuleHostMocks.cpp) + napi_* link
// stubs (stubs/NapiStubs.cpp) + folly stubs (stubs/FollyStubs.cpp), never
// invoked here. NapiRef's private ctor is accessed via #define private public
// (test-only, no production change to ArkJS.h).

// Include all standard library headers BEFORE #define private public to avoid
// affecting their internal access specifiers.
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

// Now make NapiRef's private members accessible (test-only, no production change).
#define private public
#include "RNOHCorePackage/TurboModules/StatusBarTurboModule.h"
#include "RNOH/ArkTSTurboModule.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#undef private

#include "stubs/TurboModuleHostMocks.h"
#include "MockJSIRuntime.h"

using namespace rnoh;
using facebook::react::TurboModule;
using MockJSIRuntime = facebook::jsi::MockJSIRuntime;

namespace {
class TestableStatusBarTurboModule : public StatusBarTurboModule {
 public:
  using StatusBarTurboModule::StatusBarTurboModule;
  const std::unordered_map<std::string, TurboModule::MethodMetadata>&
  methodMap() const {
    return methodMap_;
  }
};

ArkTSTurboModule::Context makeStubContext() {
  ArkTSTurboModule::Context ctx;
  ctx.env = reinterpret_cast<napi_env>(0x1);
  // Non-empty NapiRef so call()/scheduleCall() pass the
  // "if (!arkTSTurboModuleInstanceRef)" guard and reach the TaskExecutor
  // (whose stub records runSyncTask/runTask without running the lambda).
  // NapiRef(napi_env, napi_ref) is private (friend ArkJS only), so construct
  // via default + set the public members directly.
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

TEST(StatusBarTurboModule, Constructor_RegistersAllExpectedMethods) {
  TestableStatusBarTurboModule tm(makeStubContext(), "StatusBar");
  const auto& mm = tm.methodMap();
  ASSERT_EQ(mm.count("getConstants"), 1u);
  EXPECT_EQ(mm.at("getConstants").argCount, 0u);
  ASSERT_EQ(mm.count("setColor"), 1u);
  EXPECT_EQ(mm.at("setColor").argCount, 1u);
  ASSERT_EQ(mm.count("setHidden"), 1u);
  EXPECT_EQ(mm.at("setHidden").argCount, 1u);
  ASSERT_EQ(mm.count("setStyle"), 1u);
  EXPECT_EQ(mm.at("setStyle").argCount, 1u);
  ASSERT_EQ(mm.count("setTranslucent"), 1u);
  EXPECT_EQ(mm.at("setTranslucent").argCount, 1u);
}

TEST(StatusBarTurboModule, Constructor_RegistersExactlyFiveMethods) {
  TestableStatusBarTurboModule tm(makeStubContext(), "StatusBar");
  EXPECT_EQ(tm.methodMap().size(), 5u);
}

TEST(StatusBarTurboModule, Constructor_AllInvokersAreNonNull) {
  TestableStatusBarTurboModule tm(makeStubContext(), "StatusBar");
  for (const auto& [name, meta] : tm.methodMap()) {
    EXPECT_NE(meta.invoker, nullptr) << "invoker null for " << name;
  }
}

// Invoker routing (review §5): verify sync (ARK_METHOD_METADATA -> call ->
// runSyncTask) vs schedule (ARK_SCHEDULE_METHOD_METADATA -> scheduleCall ->
// runTask). The stub TaskExecutor records calls without running the posted
// lambda (which would need a real NAPI runtime), so we assert which method
// was reached. Invokers are called with count=0 so no jsi::Value is accessed.

TEST(StatusBarTurboModule, Invoker_GetConstants_RoutesSyncCall) {
  TestableStatusBarTurboModule tm(makeStubContext(), "StatusBar");
  auto* invoker = tm.methodMap().at("getConstants").invoker;
  ASSERT_NE(invoker, nullptr);
  rnoh::testing::resetTurboModuleMockState();
  MockJSIRuntime rt;
  invoker(rt, tm, nullptr, 0);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 0);
}

TEST(StatusBarTurboModule, Invoker_Setters_RouteScheduleCall) {
  // Disable lambda execution for this test — setters route via scheduleCall
  // which posts a lambda that calls ArkJS/napi. With stub NAPI returning
  // nullptr for napi_call_function, the lambda would crash on the nullptr.
  // We only verify routing (runTask count), not lambda execution.
  rnoh::testing::turboModuleMockState().executeLambdas = false;
  TestableStatusBarTurboModule tm{makeStubContext(), "StatusBar"};
  MockJSIRuntime rt;
  for (const auto* name : {"setColor", "setHidden", "setStyle", "setTranslucent"}) {
    auto it = tm.methodMap().find(name);
    ASSERT_NE(it, tm.methodMap().end()) << "missing " << name;
    ASSERT_NE(it->second.invoker, nullptr);
    rnoh::testing::resetTurboModuleMockState();
    rnoh::testing::turboModuleMockState().executeLambdas = false;
    it->second.invoker(rt, tm, nullptr, 0);
    EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 1)
        << name << " should route via schedule (runTask)";
    EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 0)
        << name << " must not route via sync (runSyncTask)";
  }
  rnoh::testing::turboModuleMockState().executeLambdas = true;
}

// Empty-NapiRef guard: when arkTSTurboModuleInstanceRef is empty, call() ->
// callSync() hits "if (!arkTSTurboModuleInstanceRef)" -> LOG(FATAL) -> abort.
// This test is disabled because LOG(FATAL) abort() kills the process under
// coverage instrumentation, preventing profraw flushing.
// To manually verify, run: ./rnoh_turbomodule_test --gtest_filter="*EmptyNapiRef*"
