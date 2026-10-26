/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "MockJSIRuntime.h"
#include "PullToRefreshViewJSIBinder.h"

using namespace rnoh;
namespace jsi = facebook::jsi;
using MockJSIRuntime = facebook::jsi::MockJSIRuntime;

// PullToRefreshViewJSIBinder overrides createNativeProps/createDirectEventTypes
// in its private section, so the test subclass exposes the inherited protected
// names from ViewComponentJSIBinder; virtual dispatch routes to the overrides.
class PullToRefreshViewJSIBinderUnderTest : public PullToRefreshViewJSIBinder {
 public:
  using ViewComponentJSIBinder::createNativeProps;
  using ViewComponentJSIBinder::createBubblingEventTypes;
  using ViewComponentJSIBinder::createDirectEventTypes;
};

class PullToRefreshViewJSIBinderTest : public ::testing::Test {
 protected:
  MockJSIRuntime rt;
  PullToRefreshViewJSIBinderUnderTest binder;

  bool hasProp(const jsi::Object& obj, const std::string& name) {
    return obj.hasProperty(rt, jsi::String::createFromAscii(rt, name));
  }

  std::string stringProp(const jsi::Object& obj, const std::string& name) {
    auto v = obj.getProperty(rt, jsi::String::createFromAscii(rt, name));
    return v.isString() ? v.getString(rt).utf8(rt) : "";
  }
};

// ─────────────────────────────────────────────────────────────────────────
// createNativeProps — PullToRefresh additions
// ─────────────────────────────────────────────────────────────────────────

TEST_F(PullToRefreshViewJSIBinderTest, CreateNativeProps_IncludesRefreshControlSpecificProps) {
  auto nativeProps = binder.createNativeProps(rt);

  EXPECT_TRUE(hasProp(nativeProps, "refreshing"));
  EXPECT_TRUE(hasProp(nativeProps, "progressViewOffset"));
  EXPECT_TRUE(hasProp(nativeProps, "progressBackgroundColor"));
  EXPECT_TRUE(hasProp(nativeProps, "tintColor"));
  EXPECT_TRUE(hasProp(nativeProps, "title"));
  EXPECT_TRUE(hasProp(nativeProps, "titleColor"));
  EXPECT_TRUE(hasProp(nativeProps, "enabled"));
  EXPECT_TRUE(hasProp(nativeProps, "size"));
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateNativeProps_RefreshControlPropTypesAreCorrect) {
  auto nativeProps = binder.createNativeProps(rt);

  EXPECT_EQ(stringProp(nativeProps, "refreshing"), "boolean");
  EXPECT_EQ(stringProp(nativeProps, "progressViewOffset"), "number");
  EXPECT_EQ(stringProp(nativeProps, "progressBackgroundColor"), "Color");
  EXPECT_EQ(stringProp(nativeProps, "tintColor"), "Color");
  EXPECT_EQ(stringProp(nativeProps, "title"), "string");
  EXPECT_EQ(stringProp(nativeProps, "titleColor"), "Color");
  EXPECT_EQ(stringProp(nativeProps, "enabled"), "boolean");
  EXPECT_EQ(stringProp(nativeProps, "size"), "string");
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateNativeProps_InheritsViewBaseProps) {
  auto nativeProps = binder.createNativeProps(rt);

  EXPECT_TRUE(hasProp(nativeProps, "removeClippedSubviews"));
  EXPECT_TRUE(hasProp(nativeProps, "nativeID"));
  EXPECT_TRUE(hasProp(nativeProps, "testID"));
  EXPECT_EQ(stringProp(nativeProps, "pointerEvents"), "string");
  EXPECT_EQ(stringProp(nativeProps, "backgroundColor"), "Color");
  EXPECT_EQ(stringProp(nativeProps, "opacity"), "number");
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateNativeProps_InheritsViewEventCallbacks) {
  auto nativeProps = binder.createNativeProps(rt);

  EXPECT_TRUE(hasProp(nativeProps, "onTouchStart"));
  EXPECT_TRUE(hasProp(nativeProps, "onResponderGrant"));
  EXPECT_TRUE(hasProp(nativeProps, "onPointerOver"));
}

// ─────────────────────────────────────────────────────────────────────────
// createDirectEventTypes — topRefresh + inherited
// ─────────────────────────────────────────────────────────────────────────

TEST_F(PullToRefreshViewJSIBinderTest, CreateDirectEventTypes_IncludesTopRefresh) {
  auto events = binder.createDirectEventTypes(rt);

  ASSERT_TRUE(hasProp(events, "topRefresh"));
  auto topRefresh = events.getProperty(rt, "topRefresh").getObject(rt);
  ASSERT_TRUE(topRefresh.hasProperty(rt, "registrationName"));
  EXPECT_EQ(
      topRefresh.getProperty(rt, "registrationName").getString(rt).utf8(rt),
      "onRefresh");
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateDirectEventTypes_InheritsViewDirectEvents) {
  auto events = binder.createDirectEventTypes(rt);

  ASSERT_TRUE(hasProp(events, "topLayout"));
  ASSERT_TRUE(hasProp(events, "topClick"));
  ASSERT_TRUE(hasProp(events, "topPointerEnter"));
  ASSERT_TRUE(hasProp(events, "topPointerLeave"));
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateDirectEventTypes_TopClickMapsToOnClick) {
  auto events = binder.createDirectEventTypes(rt);

  auto topClick = events.getProperty(rt, "topClick").getObject(rt);
  EXPECT_EQ(
      topClick.getProperty(rt, "registrationName").getString(rt).utf8(rt),
      "onClick");
}

// ─────────────────────────────────────────────────────────────────────────
// createBubblingEventTypes — inherited from View (not overridden)
// ─────────────────────────────────────────────────────────────────────────

TEST_F(PullToRefreshViewJSIBinderTest, CreateBubblingEventTypes_InheritsTouchAndFocusEvents) {
  auto events = binder.createBubblingEventTypes(rt);

  ASSERT_TRUE(hasProp(events, "topTouchStart"));
  ASSERT_TRUE(hasProp(events, "topTouchMove"));
  ASSERT_TRUE(hasProp(events, "topTouchEnd"));
  ASSERT_TRUE(hasProp(events, "topTouchCancel"));
  ASSERT_TRUE(hasProp(events, "topFocus"));
  ASSERT_TRUE(hasProp(events, "topBlur"));
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateBubblingEventTypes_TopTouchStartHasPhasedNames) {
  auto events = binder.createBubblingEventTypes(rt);

  auto topTouchStart =
      events.getProperty(rt, "topTouchStart").getObject(rt);
  ASSERT_TRUE(topTouchStart.hasProperty(rt, "phasedRegistrationNames"));
  auto phased =
      topTouchStart.getProperty(rt, "phasedRegistrationNames").getObject(rt);
  EXPECT_EQ(
      phased.getProperty(rt, "bubbled").getString(rt).utf8(rt),
      "onTouchStart");
}

// ─────────────────────────────────────────────────────────────────────────
// createBindings (public API integration)
// ─────────────────────────────────────────────────────────────────────────

TEST_F(PullToRefreshViewJSIBinderTest, CreateBindings_ReturnsObjectWithAllSubConfigs) {
  auto config = binder.createBindings(rt);

  ASSERT_TRUE(hasProp(config, "NativeProps"));
  ASSERT_TRUE(hasProp(config, "Constants"));
  ASSERT_TRUE(hasProp(config, "Commands"));
  ASSERT_TRUE(hasProp(config, "bubblingEventTypes"));
  ASSERT_TRUE(hasProp(config, "directEventTypes"));
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateBindings_NativePropsContainsRefreshControlProps) {
  auto config = binder.createBindings(rt);
  auto nativeProps = config.getProperty(rt, "NativeProps").getObject(rt);

  ASSERT_TRUE(hasProp(nativeProps, "refreshing"));
  ASSERT_TRUE(hasProp(nativeProps, "tintColor"));
  EXPECT_EQ(
      nativeProps.getProperty(rt, "refreshing").getString(rt).utf8(rt),
      "boolean");
}

TEST_F(PullToRefreshViewJSIBinderTest, CreateBindings_DirectEventTypesContainsTopRefresh) {
  auto config = binder.createBindings(rt);
  auto events = config.getProperty(rt, "directEventTypes").getObject(rt);

  ASSERT_TRUE(hasProp(events, "topRefresh"));
}
