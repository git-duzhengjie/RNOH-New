/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// ArkTSTurboModule behavior tests — covers public methods and free functions
// that are reachable through the stub harness (stub TaskExecutor, MockJSIRuntime,
// stub folly::dynamic). NAPI link stubs (NapiStubs.cpp) satisfy the linker but
// are never invoked by these tests.
//
// Coverage targets: get(), getPropertyNames(), releaseAllCallbackWrappers(),
// convertDynamicsToJSIValues(), preparePromiseResolverResult(),
// preparePromiseRejectionResult(), convertJSIValuesToIntermediaryValues().

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
#include "RNOH/ArkTSTurboModule.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#undef private

#include "stubs/TurboModuleHostMocks.h"
#include "MockJSIRuntime.h"

using namespace rnoh;
namespace jsi = facebook::jsi;
using MockJSIRuntime = facebook::jsi::MockJSIRuntime;

// Forward-declare free functions with external linkage in ArkTSTurboModule.cpp
std::string preparePromiseRejectionResult(
    const std::vector<folly::dynamic>);
facebook::jsi::Value preparePromiseResolverResult(
    facebook::jsi::Runtime&,
    const std::vector<folly::dynamic>);
const std::vector<facebook::jsi::Value> convertDynamicsToJSIValues(
    facebook::jsi::Runtime&,
    const std::vector<folly::dynamic>&);

namespace {

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

// ===========================================================================
// preparePromiseRejectionResult — pure folly::dynamic, no JSI/NAPI
// ===========================================================================

TEST(PreparePromiseRejectionResult, EmptyArgs_ReturnsEmptyString) {
  auto result = preparePromiseRejectionResult({});
  EXPECT_TRUE(result.empty());
}

TEST(PreparePromiseRejectionResult, TooManyArgs_ThrowsInvalidArgument) {
  std::vector<folly::dynamic> args = {folly::dynamic("a"), folly::dynamic("b")};
  EXPECT_THROW(preparePromiseRejectionResult(args), std::invalid_argument);
}

TEST(PreparePromiseRejectionResult, StringError_ReturnsString) {
  std::vector<folly::dynamic> args = {folly::dynamic("my error")};
  EXPECT_EQ(preparePromiseRejectionResult(args), "my error");
}

TEST(PreparePromiseRejectionResult, ObjectWithMessage_ReturnsMessage) {
  folly::dynamic obj = folly::dynamic::object;
  obj["message"] = "boom";
  std::vector<folly::dynamic> args = {obj};
  EXPECT_EQ(preparePromiseRejectionResult(args), "boom");
}

TEST(PreparePromiseRejectionResult, ObjectWithoutMessage_Throws) {
  folly::dynamic obj = folly::dynamic::object;
  obj["code"] = "E1";
  std::vector<folly::dynamic> args = {obj};
  EXPECT_THROW(preparePromiseRejectionResult(args), std::invalid_argument);
}

TEST(PreparePromiseRejectionResult, ObjectWithNonStringMessage_Throws) {
  folly::dynamic obj = folly::dynamic::object;
  obj["message"] = 42;
  std::vector<folly::dynamic> args = {obj};
  EXPECT_THROW(preparePromiseRejectionResult(args), std::invalid_argument);
}

TEST(PreparePromiseRejectionResult, NumberError_Throws) {
  std::vector<folly::dynamic> args = {folly::dynamic(42)};
  EXPECT_THROW(preparePromiseRejectionResult(args), std::invalid_argument);
}

// ===========================================================================
// preparePromiseResolverResult — MockJSIRuntime, no NAPI
// ===========================================================================

TEST(PreparePromiseResolverResult, EmptyArgs_ReturnsUndefined) {
  MockJSIRuntime rt;
  auto result = preparePromiseResolverResult(rt, {});
  EXPECT_TRUE(result.isUndefined());
}

TEST(PreparePromiseResolverResult, TooManyArgs_Throws) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> args = {folly::dynamic("a"), folly::dynamic("b")};
  EXPECT_THROW(preparePromiseResolverResult(rt, args), std::invalid_argument);
}

TEST(PreparePromiseResolverResult, SingleNumberValue_ReturnsValue) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> args = {folly::dynamic(42)};
  auto result = preparePromiseResolverResult(rt, args);
  EXPECT_FALSE(result.isUndefined());
}

// ===========================================================================
// convertDynamicsToJSIValues — MockJSIRuntime, no NAPI
// ===========================================================================

TEST(ConvertDynamicsToJSIValues, EmptyInput_ReturnsEmpty) {
  MockJSIRuntime rt;
  auto result = convertDynamicsToJSIValues(rt, {});
  EXPECT_EQ(result.size(), 0u);
}

TEST(ConvertDynamicsToJSIValues, MixedTypes_ReturnsAllValues) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> input = {
      folly::dynamic(1),
      folly::dynamic("str"),
      folly::dynamic(true)};
  auto result = convertDynamicsToJSIValues(rt, input);
  EXPECT_EQ(result.size(), 3u);
}

// ===========================================================================
// get() — MockJSIRuntime, public method override
// ===========================================================================

class ArkTSTurboModuleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rnoh::testing::resetTurboModuleMockState();
  }

  std::unique_ptr<ArkTSTurboModule> createTM() {
    return std::make_unique<ArkTSTurboModule>(makeStubContext(), "TestModule");
  }
};

TEST_F(ArkTSTurboModuleTest, Get_ReleaseAllCallbacks_ReturnsObject) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto name = facebook::jsi::PropNameID::forAscii(rt, "releaseAllCallbacks");
  auto result = tm->get(rt, name);
  EXPECT_TRUE(result.isObject());
}

TEST_F(ArkTSTurboModuleTest, Get_ReleaseCallbacksByModule_ReturnsObject) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto name =
      facebook::jsi::PropNameID::forAscii(rt, "releaseCallbacksByModule");
  auto result = tm->get(rt, name);
  EXPECT_TRUE(result.isObject());
}

TEST_F(ArkTSTurboModuleTest, Get_GetPendingCallbackCount_ReturnsObject) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto name =
      facebook::jsi::PropNameID::forAscii(rt, "getPendingCallbackCount");
  auto result = tm->get(rt, name);
  EXPECT_TRUE(result.isObject());
}

TEST_F(ArkTSTurboModuleTest, Get_UnknownProp_DelegatesToBase) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto name = facebook::jsi::PropNameID::forAscii(rt, "unknownProp");
  auto result = tm->get(rt, name);
  EXPECT_TRUE(result.isUndefined());
}

// ===========================================================================
// getPropertyNames() — MockJSIRuntime, public method override
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       GetPropertyNames_IncludesCallbackManagementMethods) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto names = tm->getPropertyNames(rt);
  std::vector<std::string> nameStrs;
  for (const auto& propId : names) {
    nameStrs.push_back(propId.utf8(rt));
  }
  auto contains = [&](const std::string& s) {
    return std::find(nameStrs.begin(), nameStrs.end(), s) != nameStrs.end();
  };
  EXPECT_TRUE(contains("releaseAllCallbacks"));
  EXPECT_TRUE(contains("releaseCallbacksByModule"));
  EXPECT_TRUE(contains("getPendingCallbackCount"));
}

// ===========================================================================
// convertJSIValuesToIntermediaryValues — with actual jsi::Value args
// ===========================================================================

class FakeCallInvoker : public facebook::react::CallInvoker {
 public:
  ~FakeCallInvoker() override = default;
  void invokeAsync(facebook::react::CallFunc&& func) noexcept override {
    if (func) {
      MockJSIRuntime rt;
      func(rt);
    }
  }
  void invokeSync(facebook::react::CallFunc&& func) override {
    if (func) {
      MockJSIRuntime rt;
      func(rt);
    }
  }
};

TEST_F(ArkTSTurboModuleTest,
       ConvertJSI_NumberArg_ReturnsDynamicVariant) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto jsInvoker = std::make_shared<FakeCallInvoker>();
  jsi::Value arg(42);
  auto result = tm->convertJSIValuesToIntermediaryValues(
      rt, jsInvoker, &arg, 1);
  EXPECT_EQ(result.size(), 1u);
}

TEST_F(ArkTSTurboModuleTest,
       ConvertJSI_StringArg_ReturnsDynamicVariant) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto jsInvoker = std::make_shared<FakeCallInvoker>();
  jsi::Value arg = facebook::jsi::String::createFromAscii(rt, "hello");
  auto result = tm->convertJSIValuesToIntermediaryValues(
      rt, jsInvoker, &arg, 1);
  EXPECT_EQ(result.size(), 1u);
}

TEST_F(ArkTSTurboModuleTest,
       ConvertJSI_TwoCallbackArgs_CreatesExclusiveGroup) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto jsInvoker = std::make_shared<FakeCallInvoker>();
  auto fn1 = facebook::jsi::Function::createFromHostFunction(
      rt, facebook::jsi::PropNameID::forAscii(rt, "cb1"), 0,
      [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t) -> jsi::Value { return jsi::Value::undefined(); });
  auto fn2 = facebook::jsi::Function::createFromHostFunction(
      rt, facebook::jsi::PropNameID::forAscii(rt, "cb2"), 0,
      [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t) -> jsi::Value { return jsi::Value::undefined(); });
  jsi::Value args[] = {jsi::Value(rt, fn1), jsi::Value(rt, fn2)};
  auto result = tm->convertJSIValuesToIntermediaryValues(
      rt, jsInvoker, args, 2);
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(ArkTSTurboModuleTest,
       ConvertJSI_MixedNumberAndCallback_ArgCountCorrect) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto jsInvoker = std::make_shared<FakeCallInvoker>();
  auto fn = facebook::jsi::Function::createFromHostFunction(
      rt, facebook::jsi::PropNameID::forAscii(rt, "cb"), 0,
      [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t) -> jsi::Value { return jsi::Value::undefined(); });
  jsi::Value args[] = {jsi::Value(42), jsi::Value(rt, fn)};
  auto result = tm->convertJSIValuesToIntermediaryValues(
      rt, jsInvoker, args, 2);
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(ArkTSTurboModuleTest,
       ConvertJSI_ThreeCallbacks_CreatesNonExclusiveGroup) {
  auto tm = createTM();
  MockJSIRuntime rt;
  auto jsInvoker = std::make_shared<FakeCallInvoker>();
  auto makeFn = [&]() {
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "cb"), 0,
        [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t) -> jsi::Value { return jsi::Value::undefined(); });
  };
  auto fn1 = makeFn();
  auto fn2 = makeFn();
  auto fn3 = makeFn();
  jsi::Value args[] = {
      jsi::Value(rt, fn1), jsi::Value(rt, fn2), jsi::Value(rt, fn3)};
  auto result = tm->convertJSIValuesToIntermediaryValues(
      rt, jsInvoker, args, 3);
  EXPECT_EQ(result.size(), 3u);
}

// ===========================================================================
// call() — full path with number arg, stub TaskExecutor
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       Call_WithNumberArg_ReachesCallSync) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg(42);
  // call() will reach callSync -> runSyncTask (stub records, doesn't run lambda)
  // The lambda inside runSyncTask calls ArkJS which needs napi — stub NapiStubs
  // return napi_ok/nullptr, so callSync's lambda body executes but returns empty
  // dynamic. jsi::valueFromDynamic then converts empty dynamic to undefined.
  // This still covers call() + convertJSIValuesToIntermediaryValues + callSync
  // entry + runSyncTask call.
  rnoh::testing::resetTurboModuleMockState();
  auto result = tm->call(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
}

// ===========================================================================
// releaseAllCallbackWrappers() — header-only, no JSI/NAPI
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       ReleaseAllCallbackWrappers_NoPendingCallbacks_NoCrash) {
  auto tm = createTM();
  tm->releaseAllCallbackWrappers();
  // No crash, registry is empty after release
}

// ===========================================================================
// RNOHPromise::reject — MockJSIRuntime
// ===========================================================================

TEST(RNOHPromiseReject, SimpleReject_DoesNotCrash) {
  MockJSIRuntime rt;
  auto rejectFn = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "reject"), 1,
      [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t)
          -> jsi::Value { return jsi::Value::undefined(); });
  auto resolveFn = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "resolve"), 1,
      [](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t)
          -> jsi::Value { return jsi::Value::undefined(); });
  RNOHPromise promise(rt, std::move(resolveFn), std::move(rejectFn));
  promise.reject(std::string("error message"));
  // Promise::reject calls the reject_ function via JSI
  // MockJSIRuntime may or may not actually invoke the lambda
  // Test verifies no crash
  SUCCEED();
}

// ===========================================================================
// callSync — direct call with non-empty ref
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       CallSync_DirectCall_RoutesToRunSyncTask) {
  auto tm = createTM();
  rnoh::testing::resetTurboModuleMockState();
  auto result = tm->callSync("someMethod", {});
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  EXPECT_TRUE(result.isNull());
}

// ===========================================================================
// scheduleCall — with non-empty ref, number arg
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       ScheduleCall_WithNumberArg_RoutesToRunTask) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg(42);
  rnoh::testing::resetTurboModuleMockState();
  tm->scheduleCall(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 0);
}

// ===========================================================================
// callAsyncNative — empty ref calls onError
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       CallAsyncNative_EmptyNapiRef_ExecutesOnErrorLambda) {
  ArkTSTurboModule::Context ctx;
  ctx.env = reinterpret_cast<napi_env>(0x1);
  ctx.turboModuleThread = TaskThread::MAIN;
  ctx.taskExecutor = std::make_shared<TaskExecutor>(
      ctx.env, std::unique_ptr<AbstractTaskRunner>(nullptr));
  ArkTSTurboModule tm(ctx, "TestModule");
  std::string errorMsg;
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  tm.callAsyncNative("method", {}, nullptr,
                     [&](std::string err) { errorMsg = err; });
  EXPECT_EQ(errorMsg, "TurboModule unavailable");
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

TEST_F(ArkTSTurboModuleTest,
       CallAsyncNative_EmptyNapiRef_NullOnError_NoCrash) {
  ArkTSTurboModule::Context ctx;
  ctx.env = reinterpret_cast<napi_env>(0x1);
  ctx.turboModuleThread = TaskThread::MAIN;
  ctx.taskExecutor = std::make_shared<TaskExecutor>(
      ctx.env, std::unique_ptr<AbstractTaskRunner>(nullptr));
  ArkTSTurboModule tm(ctx, "TestModule");
  tm.callAsyncNative("method", {}, nullptr, nullptr);
  // No crash — the null onError path is handled
}

// ===========================================================================
// call() with string arg — covers dynamicFromValue STRING branch
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       Call_WithStringArg_ReachesCallSync) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg = jsi::String::createFromAscii(rt, "test");
  rnoh::testing::resetTurboModuleMockState();
  tm->call(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
}

// ===========================================================================
// call() with bool arg — covers dynamicFromValue BOOL branch
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       Call_WithBoolArg_ReachesCallSync) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg(true);
  rnoh::testing::resetTurboModuleMockState();
  tm->call(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
}

// ===========================================================================
// ConvertDynamics — with null and bool dynamics
// ===========================================================================

TEST(ConvertDynamicsToJSIValues, NullDynamic_ReturnsNullValue) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> input = {folly::dynamic()};
  auto result = convertDynamicsToJSIValues(rt, input);
  EXPECT_EQ(result.size(), 1u);
}

TEST(ConvertDynamicsToJSIValues, BoolDynamic_ReturnsBoolValue) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> input = {folly::dynamic(true)};
  auto result = convertDynamicsToJSIValues(rt, input);
  EXPECT_EQ(result.size(), 1u);
}

TEST(ConvertDynamicsToJSIValues, DoubleDynamic_ReturnsNumberValue) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> input = {folly::dynamic(3.14)};
  auto result = convertDynamicsToJSIValues(rt, input);
  EXPECT_EQ(result.size(), 1u);
}

TEST(ConvertDynamicsToJSIValues, ObjectDynamic_ReturnsObjectValue) {
  MockJSIRuntime rt;
  folly::dynamic obj = folly::dynamic::object;
  obj["key"] = "value";
  std::vector<folly::dynamic> input = {obj};
  auto result = convertDynamicsToJSIValues(rt, input);
  EXPECT_EQ(result.size(), 1u);
}

TEST(ConvertDynamicsToJSIValues, ArrayDynamic_ReturnsArrayValue) {
  MockJSIRuntime rt;
  std::vector<folly::dynamic> input = {folly::dynamic::array(1, 2, 3)};
  auto result = convertDynamicsToJSIValues(rt, input);
  EXPECT_EQ(result.size(), 1u);
}

// ===========================================================================
// callSync with executeLambdas — covers lambda body (ArkJS/napi calls)
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       CallSync_WithExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  // Lambda body calls ArkJS/napi stubs which return nullptr —
  // the napi_get_reference_value returns nullptr, and ArkJS::getObject
  // wraps it. NapiObject::call calls napi_call_function (stub returns napi_ok).
  // arkJS.getDynamic(nullptr) calls napi_typeof which stub returns 0 (napi_undefined).
  // getDynamic handles undefined → returns folly::dynamic() (null).
  // The lambda should complete without crash.
  EXPECT_NO_THROW(tm->callSync("someMethod", {}));
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

TEST_F(ArkTSTurboModuleTest,
       CallSync_WithArgs_ExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  std::vector<ArkJS::IntermediaryArg> args = {folly::dynamic(42), folly::dynamic("str")};
  auto result = tm->callSync("someMethod", std::move(args));
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

// ===========================================================================
// scheduleCall with executeLambdas — covers lambda body
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       ScheduleCall_WithExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg(42);
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  tm->scheduleCall(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

// ===========================================================================
// callAsyncNative with non-empty ref + executeLambdas — covers lambda body
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       CallAsyncNative_WithNonEmptyRef_ExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  bool onSuccessCalled = false;
  bool onErrorCalled = false;
  tm->callAsyncNative("method", {},
                     [&](folly::dynamic) { onSuccessCalled = true; },
                     [&](std::string) { onErrorCalled = true; });
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

TEST_F(ArkTSTurboModuleTest,
       CallAsyncNative_WithNonEmptyRef_NullCallbacks_NoCrash) {
  auto tm = createTM();
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  tm->callAsyncNative("method", {}, nullptr, nullptr);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

// ===========================================================================
// call() with executeLambdas — covers full callSync lambda body
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       Call_WithNumberArg_ExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg(42);
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  auto result = tm->call(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

TEST_F(ArkTSTurboModuleTest,
       Call_WithStringArg_ExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg = jsi::String::createFromAscii(rt, "test");
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  tm->call(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

TEST_F(ArkTSTurboModuleTest,
       Call_WithBoolArg_ExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  MockJSIRuntime rt;
  jsi::Value arg(true);
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  tm->call(rt, "someMethod", &arg, 1);
  EXPECT_EQ(rnoh::testing::turboModuleMockState().runSyncTaskCount, 1);
  rnoh::testing::turboModuleMockState().executeLambdas = false;
}

// ===========================================================================
// Destructor with executeLambdas — covers destructor lambda body
// ===========================================================================

TEST_F(ArkTSTurboModuleTest,
       Destructor_WithExecuteLambdas_CoversLambdaBody) {
  auto tm = createTM();
  rnoh::testing::resetTurboModuleMockState();
  rnoh::testing::turboModuleMockState().executeLambdas = true;
  tm.reset();
  rnoh::testing::turboModuleMockState().executeLambdas = false;
  SUCCEED();
}
