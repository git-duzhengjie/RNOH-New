/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

//
// Created on 2024/3/19.
//
// Node APIs are not fully supported. To solve the compilation error of the
// interface cannot be found, please include "napi/native_api.h".

#include "TextConversions.h"
#include <cmath>

#include "react/renderer/components/text/RawTextShadowNode.h"
#include "react/renderer/components/text/TextShadowNode.h"
#include "react/renderer/components/view/conversions.h"

using namespace rnoh;

int32_t TextConversions::getArkUIFontWeight(int32_t fontWeight) {
  int32_t data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL;
  switch (fontWeight) {
    case (int)facebook::react::FontWeight::Weight100:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W100;
      break;
    case (int)facebook::react::FontWeight::Weight200:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W200;
      break;
    case (int)facebook::react::FontWeight::Weight300:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W300;
      break;
    case (int)facebook::react::FontWeight::Weight400:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W400;
      break;
    case (int)facebook::react::FontWeight::Weight500:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W500;
      break;
    case (int)facebook::react::FontWeight::Weight600:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W600;
      break;
    case (int)facebook::react::FontWeight::Weight700:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W700;
      break;
    case (int)facebook::react::FontWeight::Weight800:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W800;
      break;
    case (int)facebook::react::FontWeight::Weight900:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_W900;
      break;
    default:
      data = ArkUI_FontWeight::ARKUI_FONT_WEIGHT_NORMAL;
  }
  return data;
}

int32_t TextConversions::getArkUITextCase(
    const facebook::react::TextTransform& textTransform) {
  int textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL;
  switch (textTransform) {
    case facebook::react::TextTransform::None:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL;
      break;
    case facebook::react::TextTransform::Uppercase:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_UPPER;
      break;
    case facebook::react::TextTransform::Lowercase:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_LOWER;
      break;
    default:
      textCase = ArkUI_TextCase::ARKUI_TEXT_CASE_NORMAL;
      break;
  }
  return textCase;
}

int32_t TextConversions::getArkUIEllipsizeMode(
    const facebook::react::EllipsizeMode& ellipsizeMode) {
  int arkEllipsizeMode = -1;
  switch (ellipsizeMode) {
    case facebook::react::EllipsizeMode::Head:
      arkEllipsizeMode = ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_START;
      break;
    case facebook::react::EllipsizeMode::Tail:
      arkEllipsizeMode = ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_END;
      break;
    case facebook::react::EllipsizeMode::Middle:
      arkEllipsizeMode = ArkUI_EllipsisMode::ARKUI_ELLIPSIS_MODE_CENTER;
      break;
    default:
      arkEllipsizeMode = -1;
      break;
  }
  return arkEllipsizeMode;
}

std::pair<std::optional<float>, std::optional<float>> calcHorizontalPadding(
    const facebook::yoga::Style& yogaStyle,
    bool isRTL) {
  std::optional<float> left = std::nullopt;
  std::optional<float> right = std::nullopt;
  if (yogaStyle.padding(facebook::yoga::Edge::Left).isDefined()) {
    left = yogaStyle.padding(facebook::yoga::Edge::Left).value().unwrap();
  }
  if (yogaStyle.padding(facebook::yoga::Edge::Right).isDefined()) {
    right = yogaStyle.padding(facebook::yoga::Edge::Right).value().unwrap();
  }
  if (yogaStyle.padding(facebook::yoga::Edge::Start).isDefined()) {
    float paddingStart =
        yogaStyle.padding(facebook::yoga::Edge::Start).value().unwrap();
    isRTL ? (right = paddingStart) : (left = paddingStart);
  }
  if (yogaStyle.padding(facebook::yoga::Edge::End).isDefined()) {
    float paddingEnd =
        yogaStyle.padding(facebook::yoga::Edge::End).value().unwrap();
    isRTL ? (left = paddingEnd) : (right = paddingEnd);
  }
  if (yogaStyle.padding(facebook::yoga::Edge::Horizontal).isDefined()) {
    float paddingHorizontal =
        yogaStyle.padding(facebook::yoga::Edge::Horizontal).value().unwrap();
    left = paddingHorizontal;
    right = paddingHorizontal;
  }
  return std::make_pair(left, right);
};

std::pair<std::optional<float>, std::optional<float>> calcVerticalPadding(
    const facebook::yoga::Style& yogaStyle) {
  std::optional<float> top = std::nullopt;
  std::optional<float> bottom = std::nullopt;
  if (yogaStyle.padding(facebook::yoga::Edge::Vertical).isDefined()) {
    float paddingVertical =
        yogaStyle.padding(facebook::yoga::Edge::Vertical).value().unwrap();
    top = paddingVertical;
    bottom = paddingVertical;
  }
  if (yogaStyle.padding(facebook::yoga::Edge::Top).isDefined()) {
    top = yogaStyle.padding(facebook::yoga::Edge::Top).value().unwrap();
  }
  if (yogaStyle.padding(facebook::yoga::Edge::Bottom).isDefined()) {
    bottom = yogaStyle.padding(facebook::yoga::Edge::Bottom).value().unwrap();
  }
  return std::make_pair(top, bottom);
};

TextPaddingInfo TextConversions::getArkUIPadding(
    std::shared_ptr<const facebook::react::ParagraphProps> props) {
  TextPaddingInfo info;
  bool isRTL = false;
  if (facebook::react::optionalFloatFromYogaValue(
          props->yogaStyle.padding(facebook::yoga::Edge::All))
          .has_value()) {
    float padding = facebook::react::optionalFloatFromYogaValue(
                        props->yogaStyle.padding(facebook::yoga::Edge::All))
                        .value();
    info.top = info.right = info.bottom = info.left = padding;
  }
  std::pair<std::optional<float>, std::optional<float>> horizonPadding =
      calcHorizontalPadding(props->yogaStyle, isRTL);
  std::pair<std::optional<float>, std::optional<float>> verticalPadding =
      calcVerticalPadding(props->yogaStyle);
  info.left = horizonPadding.first.has_value() ? horizonPadding.first.value()
                                               : info.left;
  info.right = horizonPadding.second.has_value() ? horizonPadding.second.value()
                                                 : info.right;
  info.top = verticalPadding.first.has_value() ? verticalPadding.first.value()
                                               : info.top;
  info.bottom = verticalPadding.second.has_value()
      ? verticalPadding.second.value()
      : info.bottom;
  return info;
}

int32_t TextConversions::getArkUITextAlignment(
    const facebook::react::TextAlignment& textAlign) {
  int32_t align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_START;
  switch (textAlign) {
    case facebook::react::TextAlignment::Natural:
    case facebook::react::TextAlignment::Left:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_START;
      break;
    case facebook::react::TextAlignment::Right:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_END;
      break;
    case facebook::react::TextAlignment::Center:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_CENTER;
      break;
    case facebook::react::TextAlignment::Justified:
      align = ArkUI_TextAlignment::ARKUI_TEXT_ALIGNMENT_JUSTIFY;
      break;
    default:
      break;
  }
  return align;
}

std::optional<float> TextConversions::getMinFontSize(
    std::shared_ptr<const facebook::react::ParagraphProps> props) {
  // RN 0.88: adjustsFontSizeToFit / minimumFontScale 属于 ParagraphProps 的
  // paragraphAttributes 成员（Props::rawProps 仅 Android 可用）
  if (!props->paragraphAttributes.adjustsFontSizeToFit ||
      std::isnan(props->paragraphAttributes.minimumFontScale)) {
    return std::nullopt;
  }
  double minimumFontScale = props->paragraphAttributes.minimumFontScale;
  float fontSize = std::isnan(props->textAttributes.fontSize)
      ? DEFAULT_FONT_SIZE
      : (float)props->textAttributes.fontSize;
  if (fabs((float)minimumFontScale) < EPSILON_THRESHOLD ||
      fabs(fontSize) < EPSILON_THRESHOLD) {
    return std::nullopt;
  }
  return fontSize * minimumFontScale;
}

std::optional<float> TextConversions::getMaxFontSize(
    std::shared_ptr<const facebook::react::ParagraphProps> props) {
  float fontSize = std::isnan(props->textAttributes.fontSize)
      ? DEFAULT_FONT_SIZE
      : (float)props->textAttributes.fontSize;
  if (fabs(fontSize) < EPSILON_THRESHOLD) {
    return std::nullopt;
  }
  return fontSize;
}

ArkUI_Alignment TextConversions::getAlignment(
    const facebook::react::TextAlignment& alignHorizon,
    const std::string& alignVertical) {
  const static std::map<
      std::string,
      std::map<facebook::react::TextAlignment, ArkUI_Alignment>>
      convertMap{
          {"auto",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
           }},
          {"top",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START},
           }},
          {"center",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_CENTER},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_START},
           }},
          {"bottom",
           std::map<facebook::react::TextAlignment, ArkUI_Alignment>{
               {facebook::react::TextAlignment::Natural,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START},
               {facebook::react::TextAlignment::Left,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START},
               {facebook::react::TextAlignment::Right,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_END},
               {facebook::react::TextAlignment::Center,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM},
               {facebook::react::TextAlignment::Justified,
                ArkUI_Alignment::ARKUI_ALIGNMENT_BOTTOM_START},
           }}};
  if (convertMap.find(alignVertical) == convertMap.end()) {
    return ArkUI_Alignment::ARKUI_ALIGNMENT_TOP_START;
  }
  return convertMap.at(alignVertical).at(alignHorizon);
}

ArkUI_WordBreak TextConversions::getArkUIWordBreakStrategy(
    facebook::react::TextBreakStrategy textBreakStrategy) {
  switch (textBreakStrategy) {
    case facebook::react::TextBreakStrategy::Simple:
      return ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_ALL;
    case facebook::react::TextBreakStrategy::HighQuality:
    case facebook::react::TextBreakStrategy::Balanced:
      return ArkUI_WordBreak::ARKUI_WORD_BREAK_BREAK_WORD;
  }
}

bool TextConversions::getArkUITextDataDetectorEnable(
    folly::dynamic dataDetectorType) {
  return dataDetectorType == "all" || dataDetectorType == "phoneNumber" ||
      dataDetectorType == "link" || dataDetectorType == "email" ||
      dataDetectorType == "address";
}

std::optional<ArkUI_TextDataDetectorType>
TextConversions::getArkUITextDataDetectorTypes(
    folly::dynamic dataDetectorType) {
  if (dataDetectorType == "phoneNumber") {
    return ARKUI_TEXT_DATA_DETECTOR_TYPE_PHONE_NUMBER;
  } else if (dataDetectorType == "link") {
    return ARKUI_TEXT_DATA_DETECTOR_TYPE_URL;
  } else if (dataDetectorType == "email") {
    return ARKUI_TEXT_DATA_DETECTOR_TYPE_EMAIL;
  } else if (dataDetectorType == "address") {
    return ARKUI_TEXT_DATA_DETECTOR_TYPE_ADDRESS;
  } else {
    return std::nullopt;
  }
}

void TextConversions::getAttributedStringFragmentsWithoutFrames(
    const facebook::react::ShadowNode& shadowNode,
    const facebook::react::TextAttributes& baseTextAttributes,
    const facebook::react::ShadowNode& parentShadowNode,
    facebook::react::AttributedString::Fragments& outFragments) {
  if (auto rawTextShadowNode =
          dynamic_cast<const facebook::react::RawTextShadowNode*>(
              &shadowNode)) {
    outFragments.push_back(
        {.string = rawTextShadowNode->getConcreteProps().text,
         .textAttributes = baseTextAttributes});
    return;
  }

  if (auto textShadowNode =
          dynamic_cast<const facebook::react::TextShadowNode*>(&shadowNode)) {
    auto localTextAttributes = baseTextAttributes;
    localTextAttributes.apply(
        textShadowNode->getConcreteProps().textAttributes);
    for (const auto& childNode : textShadowNode->getChildren()) {
      getAttributedStringFragmentsWithoutFrames(
          *childNode, localTextAttributes, *textShadowNode, outFragments);
    }
    return;
  }

  // Any *other* kind of ShadowNode
  throw PreMeasureTerminated();
}
