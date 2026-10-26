/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include "MockNapiAPI.h"

class NapiSharedMockTest : public ::testing::Test {
 protected:
  napi_env env = &g_mockEnv;

  void SetUp() override {
    resetNapiMockState();
  }
};

TEST_F(NapiSharedMockTest, CreateInt32_GetValueInt32_RoundTrips) {
  napi_value val = nullptr;
  ASSERT_EQ(mock_napi_create_int32(env, 42, &val), napi_ok);
  ASSERT_NE(val, nullptr);

  int32_t result = 0;
  ASSERT_EQ(mock_napi_get_value_int32(env, val, &result), napi_ok);
  EXPECT_EQ(result, 42);
}

TEST_F(NapiSharedMockTest, CreateInt32_NegativeValue_RoundTrips) {
  napi_value val = nullptr;
  ASSERT_EQ(mock_napi_create_int32(env, -1, &val), napi_ok);

  int32_t result = 0;
  ASSERT_EQ(mock_napi_get_value_int32(env, val, &result), napi_ok);
  EXPECT_EQ(result, -1);
}

TEST_F(NapiSharedMockTest, CreateInt32_TypeofReturnsNumber) {
  napi_value val = nullptr;
  mock_napi_create_int32(env, 100, &val);

  napi_valuetype type = napi_undefined;
  mock_napi_typeof(env, val, &type);
  EXPECT_EQ(type, napi_number);
}

TEST_F(NapiSharedMockTest, CreateInt32_LargeValue_RoundTrips) {
  napi_value val = nullptr;
  int32_t large = 2147483647;
  ASSERT_EQ(mock_napi_create_int32(env, large, &val), napi_ok);

  int32_t result = 0;
  ASSERT_EQ(mock_napi_get_value_int32(env, val, &result), napi_ok);
  EXPECT_EQ(result, large);
}

TEST_F(NapiSharedMockTest, CreateInt32_IncrementsCreateCount) {
  napi_value v1 = nullptr, v2 = nullptr, v3 = nullptr;
  mock_napi_create_int32(env, 1, &v1);
  mock_napi_create_int32(env, 2, &v2);
  mock_napi_create_int32(env, 3, &v3);
  EXPECT_EQ(g_napiCallLog.createCount, 3);
}

TEST_F(NapiSharedMockTest, CreateInt32_GetValueDouble_Interoperable) {
  napi_value val = nullptr;
  mock_napi_create_int32(env, 55, &val);

  double dresult = 0;
  ASSERT_EQ(mock_napi_get_value_double(env, val, &dresult), napi_ok);
  EXPECT_DOUBLE_EQ(dresult, 55.0);
}

TEST_F(NapiSharedMockTest, CreateDouble_GetValueInt32_Truncates) {
  napi_value val = nullptr;
  mock_napi_create_double(env, 3.14, &val);

  int32_t result = 0;
  ASSERT_EQ(mock_napi_get_value_int32(env, val, &result), napi_ok);
  EXPECT_EQ(result, 3);
}

TEST_F(NapiSharedMockTest, ResetState_ClearsCallLog) {
  napi_value val = nullptr;
  mock_napi_create_int32(env, 1, &val);
  ASSERT_EQ(g_napiCallLog.createCount, 1);

  resetNapiMockState();
  EXPECT_EQ(g_napiCallLog.createCount, 0);
}

TEST_F(NapiSharedMockTest, Reference_LifecycleWithInt32Value) {
  napi_value val = nullptr;
  mock_napi_create_int32(env, 777, &val);

  napi_ref ref = nullptr;
  ASSERT_EQ(mock_napi_create_reference(env, val, 1, &ref), napi_ok);
  ASSERT_NE(ref, nullptr);
  EXPECT_EQ(g_napiCallLog.refCreateCount, 1);

  napi_value retrieved = nullptr;
  ASSERT_EQ(mock_napi_get_reference_value(env, ref, &retrieved), napi_ok);
  ASSERT_NE(retrieved, nullptr);

  int32_t result = 0;
  mock_napi_get_value_int32(env, retrieved, &result);
  EXPECT_EQ(result, 777);

  ASSERT_EQ(mock_napi_delete_reference(env, ref), napi_ok);
  EXPECT_EQ(g_napiCallLog.refDeleteCount, 1);
}

TEST_F(NapiSharedMockTest, FullPattern_CreateObjectCallFunctionWithInt32Args) {
  napi_value module = nullptr;
  mock_napi_create_object(env, &module);

  g_napiFnRegistry[module]->props["add"] = std::make_shared<napi_value__>(napi_value__{
      .type = napi_function,
      .fn = [](int argc, napi_value* argv) -> napi_value__ {
        int32_t sum = 0;
        for (int i = 0; i < argc; i++) {
          int32_t val = 0;
          mock_napi_get_value_int32(&g_mockEnv, argv[i], &val);
          sum += val;
        }
        return napi_value__{
            .type = napi_number,
            .intNumber = sum,
            .number = static_cast<double>(sum)};
      }});

  napi_value arg1 = nullptr, arg2 = nullptr;
  mock_napi_create_int32(env, 15, &arg1);
  mock_napi_create_int32(env, 27, &arg2);

  napi_value result = nullptr;
  napi_value args[] = {arg1, arg2};
  ASSERT_EQ(
      mock_napi_call_function(env, module, "add", 2, args, &result), napi_ok);

  int32_t sum = 0;
  mock_napi_get_value_int32(env, result, &sum);
  EXPECT_EQ(sum, 42);
  EXPECT_EQ(g_napiCallLog.callCount, 1);
}
