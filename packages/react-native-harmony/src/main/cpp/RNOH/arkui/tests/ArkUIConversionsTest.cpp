/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: ArkUI 节点基础设施 — arkui/conversions.h
// Tier: 2 (ArkUI enums + RN core, 零平台依赖)
// Functions: convertReactBorderStyleToArk, convertTextAlign, convertFontWeight
//
// Includes the real RNOH/arkui/conversions.h production header (inline
// functions are the real production symbols). Deep dependencies are satisfied
// by stubs under tests/mock/stubs/ so the header compiles on host.
// =============================================================================

#include <gtest/gtest.h>
#include "RNOH/arkui/conversions.h"

using namespace rnoh;

// ===========================================================================
// Tests
// ===========================================================================

// --- convertReactBorderStyleToArk ---

TEST(ArkUIConversions, ConvertReactBorderStyleToArk_Solid_ReturnsSolid) {
  EXPECT_EQ(
      convertReactBorderStyleToArk(facebook::react::BorderStyle::Solid),
      ArkUI_BorderStyle::ARKUI_BORDER_STYLE_SOLID);
}
TEST(ArkUIConversions, ConvertReactBorderStyleToArk_Dotted_ReturnsDotted) {
  EXPECT_EQ(
      convertReactBorderStyleToArk(facebook::react::BorderStyle::Dotted),
      ArkUI_BorderStyle::ARKUI_BORDER_STYLE_DOTTED);
}
TEST(ArkUIConversions, ConvertReactBorderStyleToArk_Dashed_ReturnsDashed) {
  EXPECT_EQ(
      convertReactBorderStyleToArk(facebook::react::BorderStyle::Dashed),
      ArkUI_BorderStyle::ARKUI_BORDER_STYLE_DASHED);
}

// --- convertTextAlign ---

TEST(ArkUIConversions, ConvertTextAlign_Natural_ReturnsStart) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Natural),
      ARKUI_TEXT_ALIGNMENT_START);
}
TEST(ArkUIConversions, ConvertTextAlign_Left_ReturnsStart) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Left),
      ARKUI_TEXT_ALIGNMENT_START);
}
TEST(ArkUIConversions, ConvertTextAlign_Right_ReturnsEnd) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Right),
      ARKUI_TEXT_ALIGNMENT_END);
}
TEST(ArkUIConversions, ConvertTextAlign_Center_ReturnsCenter) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Center),
      ARKUI_TEXT_ALIGNMENT_CENTER);
}
TEST(ArkUIConversions, ConvertTextAlign_Justified_ReturnsJustify) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Justified),
      ARKUI_TEXT_ALIGNMENT_JUSTIFY);
}

// --- convertFontWeight (bucketized, different from TextConversions' exact
// switch) ---

TEST(ArkUIConversions, ConvertFontWeight_Weight100_ReturnsW100) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight100),
      ARKUI_FONT_WEIGHT_W100);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight200_ReturnsW200) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight200),
      ARKUI_FONT_WEIGHT_W200);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight300_ReturnsW300) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight300),
      ARKUI_FONT_WEIGHT_W300);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight400_ReturnsW400) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight400),
      ARKUI_FONT_WEIGHT_W400);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight500_ReturnsW500) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight500),
      ARKUI_FONT_WEIGHT_W500);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight600_ReturnsW600) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight600),
      ARKUI_FONT_WEIGHT_W600);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight700_ReturnsW700) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight700),
      ARKUI_FONT_WEIGHT_W700);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight800_ReturnsW800) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight800),
      ARKUI_FONT_WEIGHT_W800);
}
TEST(ArkUIConversions, ConvertFontWeight_Weight900_ReturnsW900) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight900),
      ARKUI_FONT_WEIGHT_W900);
}
