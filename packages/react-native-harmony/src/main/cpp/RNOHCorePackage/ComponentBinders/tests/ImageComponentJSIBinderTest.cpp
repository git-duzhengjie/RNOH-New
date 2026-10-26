/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <RNOHCorePackage/ComponentBinders/ImageComponentJSIBinder.h>
#include <gtest/gtest.h>
#include <optional>
#include <MockJSIRuntime.h>

using namespace rnoh;
using namespace facebook::jsi;

class ImageComponentJSIBinderTest : public ::testing::Test {
 protected:
  MockJSIRuntime rt;
  std::unique_ptr<ImageComponentJSIBinder> binder;
  std::optional<Object> bindings;
  std::optional<Object> nativeProps;
  std::optional<Object> directEvents;
  std::optional<Object> bubblingEvents;

  void SetUp() override {
    binder = std::make_unique<ImageComponentJSIBinder>();
    bindings.emplace(binder->createBindings(rt));
    nativeProps.emplace(bindings->getProperty(rt, "NativeProps").getObject(rt));
    directEvents.emplace(
        bindings->getProperty(rt, "directEventTypes").getObject(rt));
    bubblingEvents.emplace(
        bindings->getProperty(rt, "bubblingEventTypes").getObject(rt));
  }
};

// ---------------------------------------------------------------------------
// Bindings structure
// ---------------------------------------------------------------------------

TEST_F(
    ImageComponentJSIBinderTest,
    CreateBindings_ReturnsObjectWithNativeProps) {
  EXPECT_TRUE(bindings->hasProperty(rt, "NativeProps"));
}

TEST_F(
    ImageComponentJSIBinderTest,
    CreateBindings_ReturnsObjectWithDirectEventTypes) {
  EXPECT_TRUE(bindings->hasProperty(rt, "directEventTypes"));
}

TEST_F(
    ImageComponentJSIBinderTest,
    CreateBindings_ReturnsObjectWithBubblingEventTypes) {
  EXPECT_TRUE(bindings->hasProperty(rt, "bubblingEventTypes"));
}

TEST_F(ImageComponentJSIBinderTest, CreateBindings_ReturnsObjectWithConstants) {
  EXPECT_TRUE(bindings->hasProperty(rt, "Constants"));
}

TEST_F(ImageComponentJSIBinderTest, CreateBindings_ReturnsObjectWithCommands) {
  EXPECT_TRUE(bindings->hasProperty(rt, "Commands"));
}

// ---------------------------------------------------------------------------
// Image-specific native props
// ---------------------------------------------------------------------------

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasSourceProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "source"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasAltProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "alt"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasResizeModeProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "resizeMode"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasResizeMethodProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "resizeMethod"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasBlurRadiusProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "blurRadius"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasDefaultSourceProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "defaultSource"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasLoadingIndicatorSourceProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "loadingIndicatorSource"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasFadeDurationProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "fadeDuration"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasCapInsetsProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "capInsets"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasTintColorProp) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "tintColor"));
}

// ---------------------------------------------------------------------------
// Inherited View props (from ViewComponentJSIBinder + BaseComponentJSIBinder)
// ---------------------------------------------------------------------------

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasInheritedNativeId) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "nativeID"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasInheritedTestID) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "testID"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasInheritedBackgroundColor) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "backgroundColor"));
}

TEST_F(
    ImageComponentJSIBinderTest,
    NativeProps_HasInheritedRemoveClippedSubviews) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "removeClippedSubviews"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasInheritedOnTouchStart) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "onTouchStart"));
}

TEST_F(
    ImageComponentJSIBinderTest,
    NativeProps_HasInheritedAccessibilityLabel) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "accessibilityLabel"));
}

TEST_F(ImageComponentJSIBinderTest, NativeProps_HasInheritedOpacity) {
  EXPECT_TRUE(nativeProps->hasProperty(rt, "opacity"));
}

// ---------------------------------------------------------------------------
// Image-specific direct events
// ---------------------------------------------------------------------------

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasTopLoadStart) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topLoadStart"));
}

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasTopLoad) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topLoad"));
}

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasTopError) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topError"));
}

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasTopLoadEnd) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topLoadEnd"));
}

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasTopProgress) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topProgress"));
}

// ---------------------------------------------------------------------------
// Inherited direct events
// ---------------------------------------------------------------------------

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasInheritedTopLayout) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topLayout"));
}

TEST_F(ImageComponentJSIBinderTest, DirectEventTypes_HasInheritedTopClick) {
  EXPECT_TRUE(directEvents->hasProperty(rt, "topClick"));
}

// ---------------------------------------------------------------------------
// Inherited bubbling events
// ---------------------------------------------------------------------------

TEST_F(ImageComponentJSIBinderTest, BubblingEventTypes_HasTopTouchStart) {
  EXPECT_TRUE(bubblingEvents->hasProperty(rt, "topTouchStart"));
}

TEST_F(ImageComponentJSIBinderTest, BubblingEventTypes_HasTopTouchEnd) {
  EXPECT_TRUE(bubblingEvents->hasProperty(rt, "topTouchEnd"));
}

TEST_F(ImageComponentJSIBinderTest, BubblingEventTypes_HasTopFocus) {
  EXPECT_TRUE(bubblingEvents->hasProperty(rt, "topFocus"));
}
