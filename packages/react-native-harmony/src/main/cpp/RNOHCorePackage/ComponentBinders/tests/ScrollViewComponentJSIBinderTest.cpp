/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOHCorePackage/ComponentBinders/ScrollViewComponentJSIBinder.h"
#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <utility>
#include "MockJSIRuntime.h"

using facebook::jsi::MockJSIRuntime;
using facebook::jsi::Object;
using rnoh::ScrollViewComponentJSIBinder;

namespace {

using ExpectedProperties =
    std::initializer_list<std::pair<const char*, const char*>>;

::testing::AssertionResult stringPropertyMatches(
    MockJSIRuntime& runtime,
    const Object& object,
    const char* propertyName,
    const char* expectedValue) {
  auto value = object.getProperty(runtime, propertyName);
  if (!value.isString()) {
    return ::testing::AssertionFailure() << propertyName << " is not a string";
  }
  auto actualValue = value.getString(runtime).utf8(runtime);
  if (actualValue != expectedValue) {
    return ::testing::AssertionFailure()
        << propertyName << " is " << actualValue << ", expected "
        << expectedValue;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult stringPropertiesMatch(
    MockJSIRuntime& runtime,
    const Object& object,
    ExpectedProperties expectedProperties) {
  for (const auto& [name, value] : expectedProperties) {
    auto result = stringPropertyMatches(runtime, object, name, value);
    if (!result) {
      return result;
    }
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult directEventMatches(
    MockJSIRuntime& runtime,
    const Object& events,
    const char* eventName,
    const char* registrationName) {
  auto eventValue = events.getProperty(runtime, eventName);
  if (!eventValue.isObject()) {
    return ::testing::AssertionFailure() << eventName << " is not an object";
  }
  auto event = eventValue.getObject(runtime);
  return stringPropertyMatches(
      runtime, event, "registrationName", registrationName);
}

class ScrollViewComponentJSIBinderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    runtime_.resetState();
    bindings_ = binder_.createBindings(runtime_);
  }

  Object getBindingSection(const char* name) {
    auto sectionValue = bindings_.getProperty(runtime_, name);
    EXPECT_TRUE(sectionValue.isObject());
    return sectionValue.getObject(runtime_);
  }

  MockJSIRuntime runtime_;
  ScrollViewComponentJSIBinder binder_;
  Object bindings_{runtime_};
};

} // namespace

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateBindings_DefaultSections_ExposesCompleteViewConfigShape) {
  constexpr const char* sections[] = {
      "NativeProps",
      "Constants",
      "Commands",
      "bubblingEventTypes",
      "directEventTypes"};

  for (auto section : sections) {
    SCOPED_TRACE(section);
    EXPECT_TRUE(bindings_.getProperty(runtime_, section).isObject());
  }
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateNativeProps_BooleanProps_UseBooleanTypeDescriptor) {
  auto nativeProps = getBindingSection("NativeProps");

  EXPECT_TRUE(stringPropertiesMatch(
      runtime_,
      nativeProps,
      {{"scrollEnabled", "boolean"},
       {"scrollsToTop", "boolean"},
       {"showsHorizontalScrollIndicator", "boolean"},
       {"showsVerticalScrollIndicator", "boolean"},
       {"persistentScrollbar", "boolean"},
       {"horizontal", "boolean"},
       {"alwaysBounceHorizontal", "boolean"},
       {"alwaysBounceVertical", "boolean"},
       {"bounces", "boolean"},
       {"snapToStart", "boolean"},
       {"snapToEnd", "boolean"},
       {"pagingEnabled", "boolean"},
       {"inverted", "boolean"},
       {"scrollToOverflowEnabled", "boolean"}}));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateNativeProps_NumericProps_UseNumberTypeDescriptor) {
  auto nativeProps = getBindingSection("NativeProps");

  EXPECT_TRUE(stringPropertiesMatch(
      runtime_,
      nativeProps,
      {{"indicatorStyle", "number"},
       {"decelerationRate", "number"},
       {"scrollEventThrottle", "number"},
       {"snapToInterval", "number"},
       {"fadingEdgeLength", "number"},
       {"flingSpeedLimit", "number"}}));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateNativeProps_ComplexProps_UseExpectedTypeDescriptors) {
  auto nativeProps = getBindingSection("NativeProps");

  EXPECT_TRUE(stringPropertiesMatch(
      runtime_,
      nativeProps,
      {{"contentOffset", "Object"},
       {"snapToOffsets", "number[]"},
       {"snapToAlignment", "ScrollViewSnapToAlignment"},
       {"maintainVisibleContentPosition", "Object"}}));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateNativeProps_HarmonyProps_UseExpectedTypeDescriptors) {
  auto nativeProps = getBindingSection("NativeProps");

  EXPECT_TRUE(stringPropertiesMatch(
      runtime_,
      nativeProps,
      {{"nestedScrollEnabled", "boolean"},
       {"overScrollMode", "string"},
       {"endFillColor", "Color"},
       {"centerContent", "boolean"}}));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateNativeProps_InternalMarkerProps_AreEnabled) {
  auto nativeProps = getBindingSection("NativeProps");

  auto disableIntervalMomentum =
      nativeProps.getProperty(runtime_, "disableIntervalMomentum");
  EXPECT_TRUE(
      disableIntervalMomentum.isBool() && disableIntervalMomentum.getBool());

  auto keyboardAvoidingViewBottomHeight =
      nativeProps.getProperty(runtime_, "__keyboardAvoidingViewBottomHeight");
  EXPECT_TRUE(
      keyboardAvoidingViewBottomHeight.isBool() &&
      keyboardAvoidingViewBottomHeight.getBool());
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateDirectEventTypes_ScrollEvent_UsesExpectedRegistrationName) {
  auto events = getBindingSection("directEventTypes");

  EXPECT_TRUE(directEventMatches(runtime_, events, "topScroll", "onScroll"));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateDirectEventTypes_DragEvents_UseExpectedRegistrationNames) {
  auto events = getBindingSection("directEventTypes");

  EXPECT_TRUE(directEventMatches(
      runtime_, events, "topScrollBeginDrag", "onScrollBeginDrag"));
  EXPECT_TRUE(directEventMatches(
      runtime_, events, "topScrollEndDrag", "onScrollEndDrag"));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateDirectEventTypes_MomentumEvents_UseExpectedRegistrationNames) {
  auto events = getBindingSection("directEventTypes");

  EXPECT_TRUE(directEventMatches(
      runtime_, events, "topMomentumScrollBegin", "onMomentumScrollBegin"));
  EXPECT_TRUE(directEventMatches(
      runtime_, events, "topMomentumScrollEnd", "onMomentumScrollEnd"));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateDirectEventTypes_InheritedLayoutEvent_RemainsRegistered) {
  auto events = getBindingSection("directEventTypes");

  EXPECT_TRUE(directEventMatches(runtime_, events, "topLayout", "onLayout"));
}

TEST_F(
    ScrollViewComponentJSIBinderTest,
    CreateBubblingEventTypes_TouchEvents_AreNotRegistered) {
  auto events = getBindingSection("bubblingEventTypes");

  EXPECT_FALSE(events.hasProperty(runtime_, "topTouchStart"));
  EXPECT_FALSE(events.hasProperty(runtime_, "topTouchMove"));
  EXPECT_FALSE(events.hasProperty(runtime_, "topTouchEnd"));
  EXPECT_FALSE(events.hasProperty(runtime_, "topTouchCancel"));
}
