/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests for TouchTarget::computeChildPoint and the 4x4 matrix
// inversion (gluInvertMatrix) it relies on. computeChildPoint is the only
// non-pure-virtual method on TouchTarget; it is NOT mocked on MockTouchTarget
// so the real implementation in TouchTarget.cpp is exercised, with the
// abstract accessors (getLayoutMetrics/getTransform/getCurrentOffset/
// getTouchTargetTag) driven by gmock. The global `dispatcher` offset is
// preset via TouchEventDispatcher::updateOffset.

#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <memory>
#include "RNOH/TouchTarget.h"
#include "RNOH/arkui/TouchEventDispatcher.h"
#include "RNOH/tests/MockTouchTarget.h"

using namespace rnoh;
using facebook::react::Float;
using facebook::react::LayoutMetrics;
using facebook::react::Point;
using facebook::react::Rect;
using facebook::react::Size;
using facebook::react::Transform;
using ::testing::Return;

namespace {
constexpr int32_t kParentTag = 42;

LayoutMetrics makeLayout(Float x, Float y, Float w, Float h) {
  LayoutMetrics lm;
  lm.frame = Rect{Point{x, y}, Size{w, h}};
  return lm;
}
} // namespace

class TouchTargetComputeChildPointTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clear any offset registered for our parent tag so tests are isolated.
    dispatcher.removeOffset(kParentTag);
    // Common expectations: parent tag is fixed; identity transform by default.
    ON_CALL(parent_, getTouchTargetTag())
        .WillByDefault(Return(kParentTag));
  }

  MockTouchTarget parent_;
  std::shared_ptr<MockTouchTarget> child_ =
      std::make_shared<MockTouchTarget>();
};

// point {30,40}, child at origin {0,0} size {100,100}, identity transform,
// no scroll/current offsets => parent coords equal child local coords.
TEST_F(TouchTargetComputeChildPointTest,
       IdentityTransform_NoOffset_ReturnsPoint) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform()).WillByDefault(Return(Transform{}));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 30.0f);
  EXPECT_FLOAT_EQ(result.y, 40.0f);
}

// Scroll offset of 10 shifts the y-axis: scrolledPoint.y = point.y - 10.
// With identity transform, origin {0,0} and currentOffset {0,0}, the
// origin/center terms cancel so localPoint == scrolledPoint == {30, 30}.
TEST_F(TouchTargetComputeChildPointTest,
       IdentityTransform_WithScrollOffset_AppliesToY) {
  dispatcher.updateOffset(kParentTag, 10.0f);
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform()).WillByDefault(Return(Transform{}));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 30.0f);
  EXPECT_FLOAT_EQ(result.y, 30.0f);
}

// Parent currentOffset {5,5} combines with the (identity) transform.
TEST_F(TouchTargetComputeChildPointTest,
       IdentityTransform_WithCurrentOffset_Combines) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform()).WillByDefault(Return(Transform{}));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{5, 5}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 35.0f);
  EXPECT_FLOAT_EQ(result.y, 45.0f);
}

// Both scroll offset (10) and currentOffset {5,5} applied together.
TEST_F(TouchTargetComputeChildPointTest,
       IdentityTransform_WithScrollAndCurrentOffset) {
  dispatcher.updateOffset(kParentTag, 10.0f);
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform()).WillByDefault(Return(Transform{}));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{5, 5}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 35.0f);
  EXPECT_FLOAT_EQ(result.y, 35.0f);
}

// A singular (all-zero) matrix is not invertible: gluInvertMatrix returns
// false, invertTransform returns nullopt, computeChildPoint returns infinity.
TEST_F(TouchTargetComputeChildPointTest,
       NonInvertibleTransform_ReturnsInfinity) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  Transform singular;
  singular.matrix.fill(0.0f);
  ON_CALL(*child_, getTransform()).WillByDefault(Return(singular));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_TRUE(std::isinf(result.x));
  EXPECT_TRUE(std::isinf(result.y));
}

// Child frame origin {10,20} shifts the transformation origin; point equal to
// the origin maps back to the child center.
TEST_F(TouchTargetComputeChildPointTest,
       ChildOriginOffset_AccountedInTransformationOrigin) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(10, 20, 100, 100)));
  ON_CALL(*child_, getTransform()).WillByDefault(Return(Transform{}));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({60, 70}, child_);
  EXPECT_FLOAT_EQ(result.x, 50.0f);
  EXPECT_FLOAT_EQ(result.y, 50.0f);
}

// Scale(2,2) transform: the inverse halves the offset-from-center.
// ({30,40}-{50,50})*invScale = {-20,-10}*{0.5} = {-10,-5}; +center = {40,45}.
TEST_F(TouchTargetComputeChildPointTest,
       ScaleTransform_HalvesOffsetFromCenter) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform())
      .WillByDefault(Return(Transform::Scale(2, 2, 1)));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 40.0f);
  EXPECT_FLOAT_EQ(result.y, 45.0f);
}

// Translate(10,20) transform: the inverse subtracts (10,20).
// ({30,40}-{50,50})*invTrans = {-20,-10} - {10,20} = {-30,-30}; +center={20,20}.
TEST_F(TouchTargetComputeChildPointTest, TranslateTransform_InversesOffset) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform())
      .WillByDefault(Return(Transform::Translate(10, 20, 0)));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 20.0f);
  EXPECT_FLOAT_EQ(result.y, 20.0f);
}

// Scale(2,2) + scroll offset 10: scrolledPoint.y=30, then halved.
// ({30,30}-{50,50})*invScale = {-20,-20}*{0.5} = {-10,-10}; +center={40,40}.
TEST_F(TouchTargetComputeChildPointTest, ScaleTransform_WithScrollOffset) {
  dispatcher.updateOffset(kParentTag, 10.0f);
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform())
      .WillByDefault(Return(Transform::Scale(2, 2, 1)));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{0, 0}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 40.0f);
  EXPECT_FLOAT_EQ(result.y, 40.0f);
}

// Translate(10,20) + currentOffset {5,5}: combines offset then inverts.
// ({30,40}+{5,5}-{50,50})*invTrans = {-15,-5}-{10,20}={-25,-25}; +center={25,25}.
TEST_F(TouchTargetComputeChildPointTest,
       TranslateTransform_WithCurrentOffset) {
  ON_CALL(*child_, getLayoutMetrics())
      .WillByDefault(Return(makeLayout(0, 0, 100, 100)));
  ON_CALL(*child_, getTransform())
      .WillByDefault(Return(Transform::Translate(10, 20, 0)));
  ON_CALL(parent_, getCurrentOffset()).WillByDefault(Return(Point{5, 5}));

  auto result = parent_.computeChildPoint({30, 40}, child_);
  EXPECT_FLOAT_EQ(result.x, 25.0f);
  EXPECT_FLOAT_EQ(result.y, 25.0f);
}
