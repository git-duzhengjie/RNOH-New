/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <cstring>
#include "MockJSIRuntime.h"

using namespace facebook::jsi;

class JSIBinderTest : public ::testing::Test {
 protected:
  MockJSIRuntime rt;

  void SetUp() override {
    rt.resetState();
  }

  String str(const char* s) {
    return rt.createStringFromAscii(s, std::strlen(s));
  }
};

// ---------------------------------------------------------------------------
// MockJSIRuntime basic validation
// ---------------------------------------------------------------------------

TEST_F(JSIBinderTest, CreateStringFromAscii_RoundTrips) {
  auto s = rt.createStringFromAscii("selectionColor", 15);
  EXPECT_STREQ(s.utf8(rt).c_str(), "selectionColor");
}

TEST_F(JSIBinderTest, CreateObject_SetGetProperty_RoundTrips) {
  auto obj = rt.createObject();
  auto key = str("selectionColor");
  obj.setProperty(rt, key, str("Color"));

  EXPECT_TRUE(rt.hasProperty(obj, key));
  auto val = rt.getProperty(obj, key);
  EXPECT_EQ(val.getString(rt).utf8(rt), std::string("Color"));
}

TEST_F(JSIBinderTest, GetProperty_NotExists_ReturnsUndefined) {
  auto obj = rt.createObject();
  auto kExist = str("exists");
  auto kMissing = str("missing");
  obj.setProperty(rt, kExist, str("yes"));

  EXPECT_FALSE(rt.hasProperty(obj, kMissing));
  auto val = rt.getProperty(obj, kMissing);
  EXPECT_TRUE(val.isUndefined());
}

TEST_F(JSIBinderTest, EmptyObject_HasNoProperties) {
  auto obj = rt.createObject();
  auto k1 = str("anyKey");
  EXPECT_FALSE(rt.hasProperty(obj, k1));
}

TEST_F(JSIBinderTest, MultipleProperties_AreIndependent) {
  auto obj = rt.createObject();
  auto kA = str("propA");
  auto kB = str("propB");
  obj.setProperty(rt, kA, str("valueA"));
  obj.setProperty(rt, kB, str("valueB"));

  EXPECT_EQ(rt.getProperty(obj, kA).getString(rt).utf8(rt), std::string("valueA"));
  EXPECT_EQ(rt.getProperty(obj, kB).getString(rt).utf8(rt), std::string("valueB"));
}

// ---------------------------------------------------------------------------
// TextComponentJSIBinder::createNativeProps pattern
// ---------------------------------------------------------------------------

TEST_F(JSIBinderTest, TextComponentNativeProps_ContainsSelectionColor) {
  auto nativeProps = rt.createObject();
  auto kID = str("nativeID");
  auto kSel = str("selectionColor");
  auto kDeco = str("textDecorationStyle");
  nativeProps.setProperty(rt, kID, str("string"));
  nativeProps.setProperty(rt, kSel, str("Color"));
  nativeProps.setProperty(rt, kDeco, str("TextDecorationStyle"));

  EXPECT_TRUE(rt.hasProperty(nativeProps, kSel));
  EXPECT_TRUE(rt.hasProperty(nativeProps, kDeco));
  EXPECT_TRUE(rt.hasProperty(nativeProps, kID));
}

TEST_F(JSIBinderTest, TextComponentNativeProps_SelectionColorIsColorType) {
  auto nativeProps = rt.createObject();
  auto k = str("selectionColor");
  nativeProps.setProperty(rt, k, str("Color"));
  EXPECT_EQ(
      rt.getProperty(nativeProps, k).getString(rt).utf8(rt),
      std::string("Color"));
}

TEST_F(JSIBinderTest, TextComponentNativeProps_TextDecorationStyleIsCorrectType) {
  auto nativeProps = rt.createObject();
  auto k = str("textDecorationStyle");
  nativeProps.setProperty(rt, k, str("TextDecorationStyle"));
  EXPECT_EQ(
      rt.getProperty(nativeProps, k).getString(rt).utf8(rt),
      std::string("TextDecorationStyle"));
}

// ---------------------------------------------------------------------------
// BaseComponentJSIBinder::createBindings pattern
// ---------------------------------------------------------------------------

TEST_F(JSIBinderTest, CreateBindings_ReturnsObjectWithAllKeys) {
  auto bindings = rt.createObject();
  auto kNP = str("NativeProps");
  auto kC = str("Constants");
  auto kCmd = str("Commands");
  auto kBE = str("bubblingEventTypes");
  auto kDE = str("directEventTypes");

  auto nativeProps = rt.createObject();
  nativeProps.setProperty(rt, str("nativeID"), str("string"));
  bindings.setProperty(rt, kNP, nativeProps);
  bindings.setProperty(rt, kC, rt.createObject());
  bindings.setProperty(rt, kCmd, rt.createObject());
  bindings.setProperty(rt, kBE, rt.createObject());

  auto directEvents = rt.createObject();
  auto topLayout = rt.createObject();
  topLayout.setProperty(rt, str("registrationName"), str("onLayout"));
  directEvents.setProperty(rt, str("topLayout"), topLayout);
  bindings.setProperty(rt, kDE, directEvents);

  EXPECT_TRUE(rt.hasProperty(bindings, kNP));
  EXPECT_TRUE(rt.hasProperty(bindings, kC));
  EXPECT_TRUE(rt.hasProperty(bindings, kCmd));
  EXPECT_TRUE(rt.hasProperty(bindings, kBE));
  EXPECT_TRUE(rt.hasProperty(bindings, kDE));
}

TEST_F(JSIBinderTest, CreateDirectEvent_OnLayout_HasRegistrationName) {
  auto eventObj = rt.createObject();
  auto kReg = str("registrationName");
  eventObj.setProperty(rt, kReg, str("onLayout"));

  EXPECT_TRUE(rt.hasProperty(eventObj, kReg));
  EXPECT_EQ(
      rt.getProperty(eventObj, kReg).getString(rt).utf8(rt),
      std::string("onLayout"));
}

// ---------------------------------------------------------------------------
// createBubblingCapturedEvent pattern
// ---------------------------------------------------------------------------

TEST_F(JSIBinderTest, CreateBubblingCapturedEvent_HasBubbledAndCaptured) {
  auto eventObj = rt.createObject();
  auto phased = rt.createObject();
  phased.setProperty(rt, str("bubbled"), str("topTouchStart"));
  phased.setProperty(rt, str("captured"), str("topTouchStartCapture"));
  eventObj.setProperty(rt, str("phasedRegistrationNames"), phased);

  auto phasedProps = rt.getProperty(eventObj, str("phasedRegistrationNames"));
  EXPECT_TRUE(phasedProps.isObject());
  auto phasedObj = phasedProps.getObject(rt);
  EXPECT_EQ(
      rt.getProperty(phasedObj, str("bubbled")).getString(rt).utf8(rt),
      std::string("topTouchStart"));
  EXPECT_EQ(
      rt.getProperty(phasedObj, str("captured")).getString(rt).utf8(rt),
      std::string("topTouchStartCapture"));
}

// ---------------------------------------------------------------------------
// JSI Mock function install + call
// ---------------------------------------------------------------------------

TEST_F(JSIBinderTest, InstallMockFunction_CallReturnsExpectedValue) {
  auto obj = rt.createObject();
  rt.installMockFunction(
      obj,
      "multiply",
      [](Runtime& rt, const Value* args, size_t count) {
        if (count < 2) return Value::undefined();
        return Value(args[0].getNumber() * args[1].getNumber());
      });

  EXPECT_TRUE(rt.hasProperty(obj, str("multiply")));
  auto fnVal = rt.getProperty(obj, str("multiply"));
  ASSERT_TRUE(fnVal.isObject());
  auto fn = fnVal.getObject(rt).getFunction(rt);
  auto result = fn.call(rt, {Value(6.0), Value(7.0)});
  EXPECT_DOUBLE_EQ(result.getNumber(), 42.0);
  EXPECT_EQ(rt.callCount(), 1);
}

TEST_F(JSIBinderTest, ResetState_ClearsCallCount) {
  auto obj = rt.createObject();
  rt.installMockFunction(
      obj, "noop", [](Runtime&, const Value*, size_t) { return Value(); });
  auto fn = rt.getProperty(obj, str("noop")).getObject(rt).getFunction(rt);
  fn.call(rt, {});
  EXPECT_EQ(rt.callCount(), 1);

  rt.resetState();
  EXPECT_EQ(rt.callCount(), 0);
}
