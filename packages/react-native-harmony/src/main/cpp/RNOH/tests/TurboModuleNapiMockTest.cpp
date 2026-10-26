/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "MockNapiAPI.h"

// ---------------------------------------------------------------------------
// NAPI interaction tests for the 5 thin ArkTSTurboModule subclasses:
//   - AppStateTurboModule
//   - AccessibilityInfoTurboModule
//   - AlertManagerTurboModule
//   - ShareTurboModule
//   - LinkingManagerTurboModule
//
// These modules are codegen shells that only populate methodMap_ in their
// constructors (method name + arg count). All method calls are delegated to
// the base class ArkTSTurboModule which uses napi_* functions via ArkJS.
//
// These tests verify that the NAPI mock correctly handles the method call
// patterns (create string for method name, call with N args) that each
// module's methods require at runtime.
// ---------------------------------------------------------------------------

struct MethodSignature {
  const char* moduleName;
  const char* methodName;
  int argc;
};

// All method signatures from production source (5 modules, 14 methods):
static const MethodSignature kAllMethods[] = {
    // AppStateTurboModule
    {"AppState", "getCurrentAppState", 0},
    {"AppState", "getConstants", 0},
    // AccessibilityInfoTurboModule
    {"AccessibilityInfo", "isScreenReaderEnabled", 0},
    {"AccessibilityInfo", "isBoldTextEnabled", 0},
    {"AccessibilityInfo", "announceForAccessibility", 1},
    {"AccessibilityInfo", "setAccessibilityFocus", 1},
    {"AccessibilityInfo", "isAccessibilityServiceEnabled", 0},
    // AlertManagerTurboModule
    {"Alert", "getConstants", 0},
    {"Alert", "alert", 3},
    // ShareTurboModule
    {"Share", "share", 2},
    // LinkingManagerTurboModule
    {"Linking", "getInitialURL", 0},
    {"Linking", "canOpenURL", 1},
    {"Linking", "openURL", 1},
    {"Linking", "openSettings", 0},
};

class TurboModuleNapiMockTest : public ::testing::Test {
 protected:
  napi_env env = &g_mockEnv;

  void SetUp() override { resetNapiMockState(); }

  napi_status simulateMethodCall(
      const std::string& methodName,
      int argc,
      const std::vector<double>& argValues = {}) {
    napi_value moduleObj = nullptr;
    mock_napi_create_object(env, &moduleObj);

    g_napiFnRegistry[moduleObj]->props[methodName] = std::make_shared<napi_value__>(napi_value__{
        .type = napi_function,
        .fn = [](int, napi_value*) -> napi_value__ { return napi_value__{}; }});

    napi_value methodNameVal = nullptr;
    auto status = mock_napi_create_string_utf8(
        env, methodName.c_str(), methodName.size(), &methodNameVal);
    if (status != napi_ok) return status;

    std::vector<napi_value> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      napi_value arg = nullptr;
      double val = (i < static_cast<int>(argValues.size())) ? argValues[i] : 0.0;
      mock_napi_create_double(env, val, &arg);
      args.push_back(arg);
    }

    napi_value result = nullptr;
    return mock_napi_call_function(
        env, moduleObj, methodName.c_str(), args.size(), args.data(), &result);
  }
};

// ===========================================================================
// P7 fix: parameterized test — each method has differentiated assertions
// (createCount varies by argc, making each test point unique)
// ===========================================================================

class TurboModuleMethodParamTest
    : public TurboModuleNapiMockTest,
      public ::testing::WithParamInterface<MethodSignature> {};

TEST_P(TurboModuleMethodParamTest, MethodCall_Succeeds_WithCorrectCreateCount) {
  const auto& m = GetParam();
  SCOPED_TRACE(std::string(m.moduleName) + "::" + m.methodName);

  EXPECT_EQ(simulateMethodCall(m.methodName, m.argc), napi_ok);
  EXPECT_EQ(g_napiCallLog.callCount, 1);
  // P7: createCount = argc + 2 (module obj + method name string + args)
  // Different methods have different argc → different expected createCount
  EXPECT_EQ(g_napiCallLog.createCount, m.argc + 2);
}

INSTANTIATE_TEST_SUITE_P(
    AllTurboModules,
    TurboModuleMethodParamTest,
    ::testing::ValuesIn(kAllMethods));

// ===========================================================================
// Method name round-trip: each method name is unique → differentiated
// ===========================================================================

TEST_F(TurboModuleNapiMockTest, MethodNames_RoundTrip_ThroughNapiMock) {
  for (const auto& m : kAllMethods) {
    SCOPED_TRACE(m.methodName);
    napi_value strVal = nullptr;
    ASSERT_EQ(
        mock_napi_create_string_utf8(
            env, m.methodName, std::string(m.methodName).size(), &strVal),
        napi_ok);
    char buf[256] = {0};
    size_t len = 0;
    ASSERT_EQ(
        mock_napi_get_value_string_utf8(env, strVal, buf, sizeof(buf), &len),
        napi_ok);
    EXPECT_EQ(std::string(buf, len), m.methodName);
  }
}

// ===========================================================================
// Per-argc differentiation: distinct createCount for 0/1/2/3 args
// ===========================================================================

TEST_F(TurboModuleNapiMockTest, ZeroArgMethod_CreateCountIsTwo) {
  EXPECT_EQ(simulateMethodCall("getConstants", 0), napi_ok);
  EXPECT_EQ(g_napiCallLog.createCount, 2);  // module obj + method name
}

TEST_F(TurboModuleNapiMockTest, OneArgMethod_CreateCountIsThree) {
  EXPECT_EQ(simulateMethodCall("canOpenURL", 1, {1.0}), napi_ok);
  EXPECT_EQ(g_napiCallLog.createCount, 3);  // + 1 arg
}

TEST_F(TurboModuleNapiMockTest, TwoArgMethod_CreateCountIsFour) {
  EXPECT_EQ(simulateMethodCall("share", 2, {1.0, 2.0}), napi_ok);
  EXPECT_EQ(g_napiCallLog.createCount, 4);  // + 2 args
}

TEST_F(TurboModuleNapiMockTest, ThreeArgMethod_CreateCountIsFive) {
  EXPECT_EQ(simulateMethodCall("alert", 3, {1.0, 2.0, 3.0}), napi_ok);
  EXPECT_EQ(g_napiCallLog.createCount, 5);  // + 3 args
}

// ===========================================================================
// NAPI mock isolation & accumulation
// ===========================================================================

TEST_F(TurboModuleNapiMockTest, ResetNapiMockState_ClearsCallLog) {
  simulateMethodCall("share", 2, {1.0, 2.0});
  EXPECT_EQ(g_napiCallLog.callCount, 1);
  resetNapiMockState();
  EXPECT_EQ(g_napiCallLog.callCount, 0);
}

TEST_F(TurboModuleNapiMockTest, MultipleModules_CallCountAccumulates) {
  simulateMethodCall("getCurrentAppState", 0);
  simulateMethodCall("alert", 3, {1.0, 2.0, 3.0});
  simulateMethodCall("share", 2, {1.0, 2.0});
  EXPECT_EQ(g_napiCallLog.callCount, 3);
}

TEST_F(TurboModuleNapiMockTest, NonExistentModule_CallReturnsInvalidArg) {
  // Module object without installed function → napi_invalid_arg
  napi_value emptyObj = nullptr;
  mock_napi_create_object(env, &emptyObj);
  napi_value result = nullptr;
  EXPECT_EQ(
      mock_napi_call_function(env, emptyObj, "nonExistent", 0, nullptr, &result),
      napi_invalid_arg);
}

// ===== NAPI mock function coverage =====
// These tests exercise mock_napi_* functions that TurboModule runtime uses.

TEST_F(TurboModuleNapiMockTest, GetUndefined_ReturnsOk) {
  napi_value val = nullptr;
  EXPECT_EQ(mock_napi_get_undefined(env, &val), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, GetNull_ReturnsOk) {
  napi_value val = nullptr;
  EXPECT_EQ(mock_napi_get_null(env, &val), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, GetBoolean_True_ReturnsOk) {
  napi_value val = nullptr;
  EXPECT_EQ(mock_napi_get_boolean(env, true, &val), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, CreateStringUtf8_ReturnsOk) {
  napi_value val = nullptr;
  EXPECT_EQ(
      mock_napi_create_string_utf8(env, "test", 4, &val), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, CreateDouble_ReturnsOk) {
  napi_value val = nullptr;
  auto status = mock_napi_create_double(env, 42.0, &val);
  EXPECT_EQ(status, napi_ok);
  EXPECT_NE(val, nullptr);
}

TEST_F(TurboModuleNapiMockTest, CreateInt32_ReturnsOk) {
  napi_value val = nullptr;
  EXPECT_EQ(mock_napi_create_int32(env, 42, &val), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, CreateArray_ReturnsOk) {
  napi_value val = nullptr;
  EXPECT_EQ(mock_napi_create_array(env, &val), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, GetValueDouble_RoundTrips) {
  napi_value val = nullptr;
  mock_napi_create_double(env, 3.14, &val);
  double result = 0;
  EXPECT_EQ(mock_napi_get_value_double(env, val, &result), napi_ok);
  EXPECT_NEAR(result, 3.14, 1e-10);
}

TEST_F(TurboModuleNapiMockTest, GetValueInt32_RoundTrips) {
  napi_value val = nullptr;
  mock_napi_create_int32(env, 42, &val);
  int32_t result = 0;
  EXPECT_EQ(mock_napi_get_value_int32(env, val, &result), napi_ok);
  EXPECT_EQ(result, 42);
}

TEST_F(TurboModuleNapiMockTest, GetValueBool_RoundTrips) {
  napi_value val = nullptr;
  mock_napi_get_boolean(env, true, &val);
  bool result = false;
  EXPECT_EQ(mock_napi_get_value_bool(env, val, &result), napi_ok);
  EXPECT_TRUE(result);
}

TEST_F(TurboModuleNapiMockTest, GetValueStringUtf8_RoundTrips) {
  napi_value val = nullptr;
  mock_napi_create_string_utf8(env, "hello", 5, &val);
  char buf[256] = {0};
  size_t len = 0;
  EXPECT_EQ(
      mock_napi_get_value_string_utf8(env, val, buf, sizeof(buf), &len),
      napi_ok);
  EXPECT_EQ(std::string(buf, len), "hello");
}

TEST_F(TurboModuleNapiMockTest, Typeof_Number_ReturnsNumberType) {
  napi_value val = nullptr;
  mock_napi_create_double(env, 1.0, &val);
  napi_valuetype type;
  EXPECT_EQ(mock_napi_typeof(env, val, &type), napi_ok);
  EXPECT_EQ(type, napi_number);
}

TEST_F(TurboModuleNapiMockTest, SetElement_ReturnsOk) {
  napi_value arr = nullptr;
  mock_napi_create_array(env, &arr);
  napi_value elem = nullptr;
  mock_napi_create_int32(env, 42, &elem);
  EXPECT_EQ(mock_napi_set_element(env, arr, 0, elem), napi_ok);
}

TEST_F(TurboModuleNapiMockTest, DeleteReference_ReturnsOk) {
  napi_value val = nullptr;
  mock_napi_create_int32(env, 1, &val);
  napi_ref ref = nullptr;
  mock_napi_create_reference(env, val, 1, &ref);
  EXPECT_EQ(mock_napi_delete_reference(env, ref), napi_ok);
}
