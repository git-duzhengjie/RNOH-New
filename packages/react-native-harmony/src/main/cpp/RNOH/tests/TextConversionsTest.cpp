/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: Text Component — TextConversions
// Tier: 2 (Folly + ArkUI enums + RN primitives)
//
// Links the real RNOH/TextConversions.cpp production source. The deep RN core
// dependency chain (ParagraphProps, ShadowNode) is satisfied by stub headers
// under tests/text_conversions_test/react/... so the production .cpp compiles
// on host without the full fbjni/JNI tree. Coverage lands on
// RNOH/TextConversions.cpp.
// =============================================================================

#include "TextConversions.h"
#include <limits>
#include <gtest/gtest.h>
#include "react/renderer/components/text/RawTextShadowNode.h"
#include "react/renderer/components/text/TextShadowNode.h"

// ===========================================================================
// Tests — link against real TextConversions.cpp
// ===========================================================================
TEST(TextConversions, GetArkUIFontWeight_Weight100_ReturnsW100) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight100),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W100);
}
TEST(TextConversions, GetArkUIFontWeight_Weight400_ReturnsW400) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight400),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W400);
}
TEST(TextConversions, GetArkUIFontWeight_Weight700_ReturnsW700) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight700),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W700);
}
TEST(TextConversions, GetArkUIFontWeight_Weight900_ReturnsW900) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight900),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W900);
}
TEST(TextConversions, GetArkUIFontWeight_UnrecognizedValue_ReturnsNormal) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(0),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL);
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(123),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL);
}

TEST(TextConversions, GetArkUIFontWeight_Weight200_ReturnsW200) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight200),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W200);
}
TEST(TextConversions, GetArkUIFontWeight_Weight300_ReturnsW300) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight300),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W300);
}
TEST(TextConversions, GetArkUIFontWeight_Weight500_ReturnsW500) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight500),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W500);
}
TEST(TextConversions, GetArkUIFontWeight_Weight600_ReturnsW600) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight600),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W600);
}
TEST(TextConversions, GetArkUIFontWeight_Weight800_ReturnsW800) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(
          (int)facebook::react::FontWeight::Weight800),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W800);
}
TEST(TextConversions, GetArkUIFontWeight_IntMax_ReturnsNormal) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(INT32_MAX),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL);
}
TEST(TextConversions, GetArkUIFontWeight_IntMin_ReturnsNormal) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(INT32_MIN),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL);
}
TEST(TextConversions, GetArkUIFontWeight_NegativeValue_ReturnsNormal) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIFontWeight(-1),
      (int32_t)ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL);
}

TEST(TextConversions, GetArkUITextCase_None_ReturnsNormal) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextCase(
          facebook::react::TextTransform::None),
      (int32_t)ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL);
}
TEST(TextConversions, GetArkUITextCase_Uppercase_ReturnsUpper) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextCase(
          facebook::react::TextTransform::Uppercase),
      (int32_t)ArkUI_TextCase::ARKUI_TEXT_CASE_UPPER);
}
TEST(TextConversions, GetArkUITextCase_Lowercase_ReturnsLower) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextCase(
          facebook::react::TextTransform::Lowercase),
      (int32_t)ArkUI_TextCase::ARKUI_TEXT_CASE_LOWER);
}
TEST(TextConversions, GetArkUITextCase_Capitalize_ReturnsNormal) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextCase(
          facebook::react::TextTransform::Capitalize),
      (int32_t)ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL);
}

TEST(TextConversions, GetArkUIEllipsizeMode_Head_ReturnsStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIEllipsizeMode(
          facebook::react::EllipsizeMode::Head),
      (int32_t)ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_START);
}
TEST(TextConversions, GetArkUIEllipsizeMode_Tail_ReturnsEnd) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIEllipsizeMode(
          facebook::react::EllipsizeMode::Tail),
      (int32_t)ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_END);
}
TEST(TextConversions, GetArkUIEllipsizeMode_Middle_ReturnsCenter) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIEllipsizeMode(
          facebook::react::EllipsizeMode::Middle),
      (int32_t)ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_CENTER);
}
TEST(TextConversions, GetArkUIEllipsizeMode_Clip_ReturnsNegativeOne) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIEllipsizeMode(
          facebook::react::EllipsizeMode::Clip),
      -1);
}

TEST(TextConversions, GetArkUITextAlignment_Natural_ReturnsStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextAlignment(
          facebook::react::TextAlignment::Natural),
      (int32_t)ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_START);
}
TEST(TextConversions, GetArkUITextAlignment_Left_ReturnsStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextAlignment(
          facebook::react::TextAlignment::Left),
      (int32_t)ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_START);
}
TEST(TextConversions, GetArkUITextAlignment_Right_ReturnsEnd) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextAlignment(
          facebook::react::TextAlignment::Right),
      (int32_t)ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_END);
}
TEST(TextConversions, GetArkUITextAlignment_Center_ReturnsCenter) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextAlignment(
          facebook::react::TextAlignment::Center),
      (int32_t)ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_CENTER);
}
TEST(TextConversions, GetArkUITextAlignment_Justified_ReturnsJustify) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUITextAlignment(
          facebook::react::TextAlignment::Justified),
      (int32_t)ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_JUSTIFY);
}

TEST(TextConversions, GetArkUIWordBreakStrategy_Simple_ReturnsBreakAll) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIWordBreakStrategy(
          facebook::react::TextBreakStrategy::Simple),
      ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_ALL);
}
TEST(TextConversions, GetArkUIWordBreakStrategy_HighQuality_ReturnsBreakWord) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIWordBreakStrategy(
          facebook::react::TextBreakStrategy::HighQuality),
      ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_WORD);
}
TEST(TextConversions, GetArkUIWordBreakStrategy_Balanced_ReturnsBreakWord) {
  EXPECT_EQ(
      rnoh::TextConversions::getArkUIWordBreakStrategy(
          facebook::react::TextBreakStrategy::Balanced),
      ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_WORD);
}

TEST(TextConversions, GetArkUITextDataDetectorEnable_All_ReturnsTrue) {
  EXPECT_TRUE(rnoh::TextConversions::getArkUITextDataDetectorEnable("all"));
}
TEST(TextConversions, GetArkUITextDataDetectorEnable_PhoneNumber_ReturnsTrue) {
  EXPECT_TRUE(
      rnoh::TextConversions::getArkUITextDataDetectorEnable("phoneNumber"));
}
TEST(TextConversions, GetArkUITextDataDetectorEnable_Link_ReturnsTrue) {
  EXPECT_TRUE(rnoh::TextConversions::getArkUITextDataDetectorEnable("link"));
}
TEST(TextConversions, GetArkUITextDataDetectorEnable_Email_ReturnsTrue) {
  EXPECT_TRUE(rnoh::TextConversions::getArkUITextDataDetectorEnable("email"));
}
TEST(TextConversions, GetArkUITextDataDetectorEnable_Address_ReturnsTrue) {
  EXPECT_TRUE(rnoh::TextConversions::getArkUITextDataDetectorEnable("address"));
}
TEST(
    TextConversions,
    GetArkUITextDataDetectorEnable_UnknownString_ReturnsFalse) {
  EXPECT_FALSE(
      rnoh::TextConversions::getArkUITextDataDetectorEnable("unknown"));
}
TEST(TextConversions, GetArkUITextDataDetectorEnable_EmptyString_ReturnsFalse) {
  EXPECT_FALSE(rnoh::TextConversions::getArkUITextDataDetectorEnable(""));
}

TEST(
    TextConversions,
    GetArkUITextDataDetectorTypes_PhoneNumber_ReturnsPhoneNumber) {
  auto r = rnoh::TextConversions::getArkUITextDataDetectorTypes("phoneNumber");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value(), ARKUI_TEXT_DATA_DETECTOR_TYPE_PHONE_NUMBER);
}
TEST(TextConversions, GetArkUITextDataDetectorTypes_Link_ReturnsUrl) {
  auto r = rnoh::TextConversions::getArkUITextDataDetectorTypes("link");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value(), ARKUI_TEXT_DATA_DETECTOR_TYPE_URL);
}
TEST(TextConversions, GetArkUITextDataDetectorTypes_Email_ReturnsEmail) {
  auto r = rnoh::TextConversions::getArkUITextDataDetectorTypes("email");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value(), ARKUI_TEXT_DATA_DETECTOR_TYPE_EMAIL);
}
TEST(TextConversions, GetArkUITextDataDetectorTypes_Address_ReturnsAddress) {
  auto r = rnoh::TextConversions::getArkUITextDataDetectorTypes("address");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r.value(), ARKUI_TEXT_DATA_DETECTOR_TYPE_ADDRESS);
}
TEST(TextConversions, GetArkUITextDataDetectorTypes_All_ReturnsNullopt) {
  EXPECT_FALSE(
      rnoh::TextConversions::getArkUITextDataDetectorTypes("all").has_value());
}
TEST(
    TextConversions,
    GetArkUITextDataDetectorTypes_UnknownString_ReturnsNullopt) {
  EXPECT_FALSE(
      rnoh::TextConversions::getArkUITextDataDetectorTypes("xyz").has_value());
}

TEST(TextConversions, GetAlignment_AutoNatural_ReturnsTopStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Natural, "auto"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START);
}
TEST(TextConversions, GetAlignment_CenterCenter_ReturnsCenter) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Center, "center"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_CENTER);
}
TEST(TextConversions, GetAlignment_BottomRight_ReturnsBottomEnd) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Right, "bottom"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_END);
}
TEST(TextConversions, GetAlignment_UnknownVertical_ReturnsTopStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Center, "unknown"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START);
}

// --- auto vertical: 5 combinations ---
TEST(TextConversions, GetAlignment_AutoLeft_ReturnsTopStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Left, "auto"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START);
}
TEST(TextConversions, GetAlignment_AutoRight_ReturnsTopEnd) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Right, "auto"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_END);
}
TEST(TextConversions, GetAlignment_AutoCenter_ReturnsTop) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Center, "auto"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP);
}
TEST(TextConversions, GetAlignment_AutoJustified_ReturnsTopStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Justified, "auto"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START);
}

// --- center vertical: 5 combinations ---
TEST(TextConversions, GetAlignment_CenterNatural_ReturnsStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Natural, "center"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_START);
}
TEST(TextConversions, GetAlignment_CenterLeft_ReturnsStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Left, "center"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_START);
}
TEST(TextConversions, GetAlignment_CenterRight_ReturnsEnd) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Right, "center"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_END);
}
TEST(TextConversions, GetAlignment_CenterJustified_ReturnsStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Justified, "center"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_START);
}

// --- bottom vertical: 5 combinations ---
TEST(TextConversions, GetAlignment_BottomNatural_ReturnsBottomStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Natural, "bottom"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START);
}
TEST(TextConversions, GetAlignment_BottomLeft_ReturnsBottomStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Left, "bottom"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START);
}
TEST(TextConversions, GetAlignment_BottomCenter_ReturnsBottom) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Center, "bottom"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM);
}
TEST(TextConversions, GetAlignment_BottomJustified_ReturnsBottomStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Justified, "bottom"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START);
}

// --- top vertical: now in map (matches production code) ---
TEST(TextConversions, GetAlignment_TopRight_ReturnsTopEnd) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Right, "top"),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_END);
}
TEST(TextConversions, GetAlignment_EmptyVertical_ReturnsTopStart) {
  EXPECT_EQ(
      rnoh::TextConversions::getAlignment(
          facebook::react::TextAlignment::Center, ""),
      ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START);
}

// ===========================================================================
// Tests for methods requiring ParagraphProps (real .cpp linking stub)
// ===========================================================================

TEST(TextConversions, GetMaxFontSize_NormalFontSize_ReturnsFontSize) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->textAttributes.fontSize = 16.0;
  auto result = rnoh::TextConversions::getMaxFontSize(props);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result.value(), 16.0f);
}

TEST(TextConversions, GetMaxFontSize_NaN_ReturnsDefault) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->textAttributes.fontSize = std::numeric_limits<double>::quiet_NaN();
  auto result = rnoh::TextConversions::getMaxFontSize(props);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result.value(), 14.0f);
}

TEST(TextConversions, GetMaxFontSize_Zero_ReturnsNullopt) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->textAttributes.fontSize = 0.0;
  auto result = rnoh::TextConversions::getMaxFontSize(props);
  EXPECT_FALSE(result.has_value());
}

TEST(TextConversions, GetMinFontSize_NoRawProps_ReturnsNullopt) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  auto result = rnoh::TextConversions::getMinFontSize(props);
  EXPECT_FALSE(result.has_value());
}

TEST(TextConversions, GetMinFontSize_AdjustsFitTrue_ReturnsScaledSize) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->textAttributes.fontSize = 16.0;
  props->rawProps["adjustsFontSizeToFit"] = true;
  props->rawProps["minimumFontScale"] = 0.5;
  auto result = rnoh::TextConversions::getMinFontSize(props);
  ASSERT_TRUE(result.has_value());
  EXPECT_FLOAT_EQ(result.value(), 8.0f);
}

TEST(TextConversions, GetMinFontSize_AdjustsFitFalse_ReturnsNullopt) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->textAttributes.fontSize = 16.0;
  props->rawProps["adjustsFontSizeToFit"] = false;
  props->rawProps["minimumFontScale"] = 0.5;
  auto result = rnoh::TextConversions::getMinFontSize(props);
  EXPECT_FALSE(result.has_value());
}

TEST(TextConversions, GetMinFontSize_ZeroFontScale_ReturnsNullopt) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->textAttributes.fontSize = 16.0;
  props->rawProps["adjustsFontSizeToFit"] = true;
  props->rawProps["minimumFontScale"] = 0.0;
  auto result = rnoh::TextConversions::getMinFontSize(props);
  EXPECT_FALSE(result.has_value());
}

TEST(TextConversions, GetArkUIPadding_NoPadding_ReturnsZeros) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  auto result = rnoh::TextConversions::getArkUIPadding(props);
  EXPECT_FLOAT_EQ(result.top, 0.0f);
  EXPECT_FLOAT_EQ(result.bottom, 0.0f);
  EXPECT_FLOAT_EQ(result.left, 0.0f);
  EXPECT_FLOAT_EQ(result.right, 0.0f);
}

TEST(TextConversions, GetArkUIPadding_AllEdgesSet_ReturnsUniformPadding) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->yogaStyle.setPadding(facebook::yoga::Edge::All, 10.0f);
  auto result = rnoh::TextConversions::getArkUIPadding(props);
  EXPECT_FLOAT_EQ(result.top, 10.0f);
  EXPECT_FLOAT_EQ(result.bottom, 10.0f);
  EXPECT_FLOAT_EQ(result.left, 10.0f);
  EXPECT_FLOAT_EQ(result.right, 10.0f);
}

TEST(TextConversions, GetArkUIPadding_LeftRightSet_ReturnsSpecificPadding) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->yogaStyle.setPadding(facebook::yoga::Edge::Left, 5.0f);
  props->yogaStyle.setPadding(facebook::yoga::Edge::Right, 7.0f);
  auto result = rnoh::TextConversions::getArkUIPadding(props);
  EXPECT_FLOAT_EQ(result.left, 5.0f);
  EXPECT_FLOAT_EQ(result.right, 7.0f);
}

TEST(TextConversions, GetArkUIPadding_TopBottomSet_ReturnsVerticalPadding) {
  auto props = std::make_shared<facebook::react::ParagraphProps>();
  props->yogaStyle.setPadding(facebook::yoga::Edge::Top, 3.0f);
  props->yogaStyle.setPadding(facebook::yoga::Edge::Bottom, 8.0f);
  auto result = rnoh::TextConversions::getArkUIPadding(props);
  EXPECT_FLOAT_EQ(result.top, 3.0f);
  EXPECT_FLOAT_EQ(result.bottom, 8.0f);
}

TEST(TextConversions, GetAttributedStringFragments_RawText_PushesFragment) {
  facebook::react::RawTextShadowNode rawNode;
  facebook::react::RawTextProps rawProps;
  rawProps.text = "hello";
  rawNode.setProps(rawProps);
  facebook::react::TextAttributes baseAttrs;
  facebook::react::AttributedString::Fragments fragments;
  rnoh::TextConversions::getAttributedStringFragmentsWithoutFrames(
      rawNode, baseAttrs, rawNode, fragments);
  ASSERT_EQ(fragments.size(), 1u);
  EXPECT_EQ(fragments[0].string, "hello");
}

TEST(
    TextConversions,
    GetAttributedStringFragments_TextNodeWithChildren_Recurses) {
  facebook::react::TextShadowNode textNode;
  facebook::react::TextProps textProps;
  textProps.textAttributes.fontSize = 16.0;
  textNode.setProps(textProps);

  auto rawChild = std::make_shared<facebook::react::RawTextShadowNode>();
  facebook::react::RawTextProps rawProps;
  rawProps.text = "world";
  rawChild->setProps(rawProps);
  textNode.addChild(rawChild);

  facebook::react::TextAttributes baseAttrs;
  facebook::react::AttributedString::Fragments fragments;
  rnoh::TextConversions::getAttributedStringFragmentsWithoutFrames(
      textNode, baseAttrs, textNode, fragments);
  ASSERT_EQ(fragments.size(), 1u);
  EXPECT_EQ(fragments[0].string, "world");
}

TEST(
    TextConversions,
    GetAttributedStringFragments_OtherNode_ThrowsPreMeasureTerminated) {
  class UnknownShadowNode : public facebook::react::ShadowNode {
   public:
    const std::vector<facebook::react::SharedShadowNode>& getChildren()
        const override {
      static std::vector<facebook::react::SharedShadowNode> empty;
      return empty;
    }
  };
  UnknownShadowNode node;
  facebook::react::TextAttributes baseAttrs;
  facebook::react::AttributedString::Fragments fragments;
  EXPECT_THROW(
      rnoh::TextConversions::getAttributedStringFragmentsWithoutFrames(
          node, baseAttrs, node, fragments),
      rnoh::PreMeasureTerminated);
}
