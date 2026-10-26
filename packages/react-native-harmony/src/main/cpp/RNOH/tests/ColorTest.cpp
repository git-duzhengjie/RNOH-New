/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOH/Color.h"
#include <gtest/gtest.h>

#include <sstream>
#include <string>

using namespace rnoh;
using facebook::react::ColorComponents;
using facebook::react::SharedColor;
using facebook::react::colorFromComponents;

// RNOH Color wraps facebook::react::ColorComponents and uses the cxx platform
// Color (int32_t) packed as ARGB: (alpha << 24) | (red << 16) | (green << 8) |
// blue. A SharedColor is "meaningful" when its alpha byte is non-zero.
// `from(ColorValue)` scales RGB by 255 and leaves alpha as-is; `from(SharedColor)`
// does NOT scale, so the two paths store different ranges for the same color.
// Expected values below are hand-computed from that packing, not produced by
// the production code under test.

TEST(Color, Constructor_TypicalValues_StoresComponents) {
  Color c(10, 20, 30, 0.5);
  EXPECT_EQ(c.toRGBA(), "rgba(10,20,30,0.5)");
}

TEST(Color, Constructor_ZeroValues_StoresZeroComponents) {
  Color c(0, 0, 0, 0);
  EXPECT_EQ(c.toRGBA(), "rgba(0,0,0,0)");
}

// The constructor does NOT clamp; clamping only happens in asColorValue().
TEST(Color, Constructor_NegativeComponents_StoresAsIs) {
  Color c(-10, -5, -1, -0.25);
  EXPECT_EQ(c.toRGBA(), "rgba(-10,-5,-1,-0.25)");
}

TEST(Color, FromColorValue_OpaqueRed_ScalesToByteRange) {
  // ARGB: alpha=0xFF, red=0xFF, green=0x00, blue=0x00
  ColorValue opaqueRed = 0xFFFF0000u;
  Color c = Color::from(opaqueRed);
  EXPECT_EQ(c.toRGBA(), "rgba(255,0,0,1)");
}

TEST(Color, FromColorValue_Zero_ReturnsTransparentBlack) {
  Color c = Color::from(static_cast<ColorValue>(0u));
  EXPECT_EQ(c.toRGBA(), "rgba(0,0,0,0)");
}

TEST(Color, FromSharedColor_Meaningful_KeepsNormalizedRange) {
  // colorFromComponents takes 0..1 floats; from(SharedColor) stores them as-is.
  SharedColor sc = colorFromComponents(
      {.red = 1.0f, .green = 0.0f, .blue = 0.0f, .alpha = 1.0f});
  Color c = Color::from(sc);
  EXPECT_EQ(c.toRGBA(), "rgba(1,0,0,1)");
}

TEST(Color, FromSharedColor_NotMeaningful_ReturnsTransparent) {
  // Default-constructed SharedColor is UndefinedColor (0) → alpha 0 → not
  // meaningful → from() returns the transparent sentinel.
  SharedColor sc;
  Color c = Color::from(sc);
  EXPECT_EQ(c.toRGBA(), "rgba(0,0,0,0)");
}

TEST(Color, AsColorValue_Normal_PacksToARGB) {
  Color c(255, 0, 0, 1.0);
  EXPECT_EQ(c.asColorValue(), 0xFFFF0000u);
}

TEST(Color, AsColorValue_ClampsOverflow) {
  Color c(300, 300, 300, 2.0);
  EXPECT_EQ(c.asColorValue(), 0xFFFFFFFFu);
}

TEST(Color, AsColorValue_ClampsNegative) {
  Color c(-10, -10, -10, -0.5);
  EXPECT_EQ(c.asColorValue(), 0x00000000u);
}

TEST(Color, AsColorValue_BlueOnly_PacksCorrectly) {
  Color c(0, 0, 255, 1.0);
  EXPECT_EQ(c.asColorValue(), 0xFF0000FFu);
}

TEST(Color, OperatorPlus_AddsComponentsPerChannel) {
  Color a(255, 0, 0, 1.0);
  Color b(0, 255, 0, 1.0);
  Color sum = a + b;
  EXPECT_EQ(sum.toRGBA(), "rgba(255,255,0,2)");
}

TEST(Color, OperatorMultiply_ScalesAllComponents) {
  Color c(10, 20, 30, 0.5);
  Color scaled = c * 2.0f;
  EXPECT_EQ(scaled.toRGBA(), "rgba(20,40,60,1)");
}

TEST(Color, StreamOperator_FormatsAsRGBA) {
  Color c(1, 2, 3, 0.25);
  std::ostringstream os;
  os << c;
  EXPECT_EQ(os.str(), "rgba(1,2,3,0.25)");
}

// --- Channel-isolation + boundary coverage (review §13). ---

TEST(Color, AsColorValue_GreenOnly_PacksCorrectly) {
  Color c(0, 255, 0, 1.0);
  EXPECT_EQ(c.asColorValue(), 0xFF00FF00u);
}

TEST(Color, AsColorValue_DistinctChannels_PacksInARGBOrder) {
  // red=10, green=20, blue=30, alpha=0.5 -> clamped/rounded: 10,20,30,128
  Color c(10, 20, 30, 0.5);
  EXPECT_EQ(c.asColorValue(), 0x800A141Eu);
}

TEST(Color, AsColorValue_HalfAlpha_PacksExpectedAlpha) {
  Color c(0, 0, 0, 0.5);
  EXPECT_EQ(c.asColorValue(), 0x80000000u);
}

TEST(Color, FromSharedColor_DistinctChannels_TruncatedToInt_NoScaling) {
  // from(SharedColor) does NOT scale (unlike from(ColorValue) which ×255).
  // Float components 0-1 are truncated to int 0 by Color(int,...).
  // This verifies the no-scale + truncate quirk: 0.5/0.25/0.75 → 0,0,0.
  SharedColor sc = colorFromComponents(
      {.red = 0.5f, .green = 0.25f, .blue = 0.75f, .alpha = 1.0f});
  Color c = Color::from(sc);
  EXPECT_EQ(c.toRGBA(), "rgba(0,0,0,1)");
}

TEST(Color, OperatorPlus_AllChannelsNonZero_AddsIndependently) {
  Color a(10, 20, 30, 0.5);
  Color b(1, 2, 3, 0.25);
  Color sum = a + b;
  EXPECT_EQ(sum.toRGBA(), "rgba(11,22,33,0.75)");
}

TEST(Color, OperatorMultiply_Zero_ReturnsZeroComponents) {
  Color c(10, 20, 30, 0.5);
  Color scaled = c * 0.0f;
  EXPECT_EQ(scaled.toRGBA(), "rgba(0,0,0,0)");
}

TEST(Color, OperatorMultiply_Negative_PreservesSign) {
  Color c(10, 20, 30, 0.5);
  Color scaled = c * -1.0f;
  EXPECT_EQ(scaled.toRGBA(), "rgba(-10,-20,-30,-0.5)");
}

TEST(Color, OperatorMultiply_Fraction_ScalesAllChannels) {
  Color c(10, 20, 30, 1.0);
  Color scaled = c * 0.5f;
  EXPECT_EQ(scaled.toRGBA(), "rgba(5,10,15,0.5)");
}
