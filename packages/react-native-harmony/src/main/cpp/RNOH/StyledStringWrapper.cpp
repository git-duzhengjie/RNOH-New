/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "StyledStringWrapper.h"
#include <cmath>
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_point.h>
#include <native_drawing/drawing_types.h>
#include "TextConversions.h"
#include "TypographyStyleWrapper.h"
#include "react/renderer/graphics/Color.h"
#include "react/renderer/graphics/Size.h"

namespace rnoh {

facebook::react::Float normalizeFontSize(facebook::react::Float fontSize) {
  if (fontSize > 0) {
    return fontSize;
  }
  // set fontSize to default for negative values(same as iOS)
  return DEFAULT_FONT_SIZE;
}

double getOHDrawingFontSize(
    const facebook::react::TextAttributes& textAttributes,
    facebook::react::Float pointScaleFactor,
    facebook::react::Float maxFontSizeMultiplier) {
  if (textAttributes.allowFontScaling.has_value() &&
      !textAttributes.allowFontScaling.value()) {
    return normalizeFontSize(textAttributes.fontSize) * pointScaleFactor;
  }
  // Check fragment-level maxFontSizeMultiplier first (e.g. nested Text with
  // maxFontSizeMultiplier=0 means no limit), fall back to paragraph-level
  bool hasFragmentMaxMultiplier =
      !std::isnan(textAttributes.maxFontSizeMultiplier);
  facebook::react::Float effectiveMaxMultiplier = hasFragmentMaxMultiplier
      ? textAttributes.maxFontSizeMultiplier
      : maxFontSizeMultiplier;
  // maxFontSizeMultiplier < 1 (including 0) means no limit on font scaling
  if (!std::isnan(effectiveMaxMultiplier) && effectiveMaxMultiplier >= 1) {
    return normalizeFontSize(textAttributes.fontSize) * pointScaleFactor *
        std::min(textAttributes.fontSizeMultiplier, effectiveMaxMultiplier);
  }
  return normalizeFontSize(textAttributes.fontSize) * pointScaleFactor *
      textAttributes.fontSizeMultiplier;
}

bool getOHDrawingHalfLeading(
    const facebook::react::TextAttributes& textAttributes) {
  return textAttributes.lineHeight > normalizeFontSize(textAttributes.fontSize);
}

double getOHDrawingFontHeight(
    const facebook::react::TextAttributes& textAttributes) {
  // fontSize * fontHeight = lineHeight, no direct setting supported
  // lineHeight so do it in this weird way
  return textAttributes.lineHeight / normalizeFontSize(textAttributes.fontSize);
}

OH_Drawing_FontWeight mapValueToFontWeight(int value) {
  switch (value) {
    case 100:
      return OH_Drawing_FontWeight::FONT_WEIGHT_100;
    case 200:
      return OH_Drawing_FontWeight::FONT_WEIGHT_200;
    case 300:
      return OH_Drawing_FontWeight::FONT_WEIGHT_300;
    case 400:
      return OH_Drawing_FontWeight::FONT_WEIGHT_400;
    case 500:
      return OH_Drawing_FontWeight::FONT_WEIGHT_500;
    case 600:
      return OH_Drawing_FontWeight::FONT_WEIGHT_600;
    case 700:
      return OH_Drawing_FontWeight::FONT_WEIGHT_700;
    case 800:
      return OH_Drawing_FontWeight::FONT_WEIGHT_800;
    case 900:
      return OH_Drawing_FontWeight::FONT_WEIGHT_900;
    default:
      return OH_Drawing_FontWeight::FONT_WEIGHT_400;
  }
}
size_t utf16Length(const std::string& str) {
  size_t len = 0;
  const unsigned char* currentByte =
      reinterpret_cast<const unsigned char*>(str.data());
  const unsigned char* endOfBytes = currentByte + str.size();
  // Judge the length of UTF-8 characters based on the number of bits higher
  // than the first byte, take out the significant bits, and record the number
  // of continuation bytes that need to be read
  while (currentByte < endOfBytes) {
    uint32_t codePoint = 0;
    int continuationBytes = 0;
    if (*currentByte < 0x80) {
      codePoint = *currentByte++;
    } else if ((*currentByte >> 5) == 0x6) {
      codePoint = *currentByte & 0x1F;
      continuationBytes = 1;
      ++currentByte;
    } else if ((*currentByte >> 4) == 0xE) {
      codePoint = *currentByte & 0x0F;
      continuationBytes = 2;
      ++currentByte;
    } else if ((*currentByte >> 3) == 0x1E) {
      codePoint = *currentByte & 0x07;
      continuationBytes = 3;
      ++currentByte;
    } else {
      ++currentByte;
      continue;
    }
    // The remaining bytes are taken and the length is calculated
    while (continuationBytes-- && currentByte < endOfBytes &&
           ((*currentByte & 0xC0) == 0x80)) {
      codePoint = (codePoint << 6) | (*currentByte++ & 0x3F);
    }
    // The number of code elements is calculated according to the hexadecimal
    // format
    if (codePoint < 0x10000) {
      len += 1;
    } else {
      len += 2;
    }
  }
  return len;
}

std::string stringCapitalize(const std::string& strInput) {
  if (strInput.empty()) {
    return strInput;
  }

  std::string strRes;
  std::string split = " ";
  std::vector<std::string> subStringVector;
  subStringVector.clear();

  std::string strSrc = strInput + split;
  auto pos = strSrc.find(split);
  auto step = split.size();

  while (pos != std::string::npos) {
    std::string strTemp = strSrc.substr(0, pos);
    subStringVector.push_back(strTemp);

    strSrc = strSrc.substr(pos + step, strSrc.size());
    pos = strSrc.find(split);
  }

  for (auto subString : subStringVector) {
    if (std::isalpha(subString[0]) != 0) {
      std::transform(
          subString.begin(), subString.end(), subString.begin(), ::tolower);
      subString[0] = std::toupper(static_cast<unsigned char>(subString[0]));
    }
    if (!strRes.empty()) {
      strRes += split;
    }
    strRes += subString;
  }
  return strRes;
}

void textCaseTransform(
    std::string& textContent,
    facebook::react::TextTransform type) {
  switch (type) {
    case facebook::react::TextTransform::Uppercase: {
      transform(
          textContent.begin(),
          textContent.end(),
          textContent.begin(),
          ::toupper);
      break;
    }
    case facebook::react::TextTransform::Lowercase: {
      transform(
          textContent.begin(),
          textContent.end(),
          textContent.begin(),
          ::tolower);
      break;
    }
    case facebook::react::TextTransform::Capitalize: {
      textContent = stringCapitalize(textContent);
      break;
    }
    default:
      break;
  }
}

StyledStringWrapper::Shared StyledStringWrapper::createStyledString(
    facebook::react::AttributedString const& attributedString,
    facebook::react::ParagraphAttributes const& paragraphAttributes,
    facebook::react::TextLayoutContext const& layoutContext,
    FontRegistry const& fontRegistry) {
  TypographyStyleWrapper typographyStyle(paragraphAttributes);

  if (!attributedString.getFragments().empty()) {
    typographyStyle.apply(attributedString.getFragments()[0].textAttributes);
  }

  // Allocate on the heap (via make_shared) so that the caller can later call
  // get() (which seals this Sealable object) and still copy/share the pointer
  // without ever move/copy-constructing the sealed StyledStringWrapper value
  // (which would trip Sealable::ensureUnsealed in debug builds).
  auto styledStringWrapper = std::make_shared<StyledStringWrapper>(
      typographyStyle.get(),
      fontRegistry.getFontCollection(),
      fontRegistry.getThemeFontFamily(),
      layoutContext.pointScaleFactor,
      paragraphAttributes.maxFontSizeMultiplier);
  for (auto const& fragment : attributedString.getFragments()) {
    styledStringWrapper->addFragment(fragment);
  }
  return styledStringWrapper;
}

void StyledStringWrapper::addText(
    const std::string& text,
    const facebook::react::TextAttributes& textAttributes) {
  // No ThreadGuard here — see note in addFragment(); this is called from the
  // parallel FFRT text pipeline, serialized via TextStorageBuilder::m_mutex.
  ensureUnsealed();
  UniqueTextStyle textStyle(
      OH_Drawing_CreateTextStyle(), OH_Drawing_DestroyTextStyle);

  OH_Drawing_SetTextStyleFontSize(
      textStyle.get(),
      getOHDrawingFontSize(textAttributes, m_scale, m_maxFontScale));

  // halfLeading
  if (!std::isnan(textAttributes.lineHeight)) {
    OH_Drawing_SetTextStyleHalfLeading(
        textStyle.get(), getOHDrawingHalfLeading(textAttributes));
  }

  // fontStyle
  if (textAttributes.fontStyle.has_value()) {
    OH_Drawing_SetTextStyleFontStyle(
        textStyle.get(), (int)textAttributes.fontStyle.value());
  }

  // fontColor
  if (textAttributes.foregroundColor) {
    OH_Drawing_SetTextStyleColor(
        textStyle.get(), (uint32_t)(*textAttributes.foregroundColor));
  }

  // textDecoration
  int32_t textDecorationType = TEXT_DECORATION_NONE;
  uint32_t textDecorationColor = 0xFF000000;
  int32_t textDecorationStyle = TEXT_DECORATION_STYLE_SOLID;
  if (textAttributes.textDecorationLineType.has_value()) {
    textDecorationType = (int32_t)textAttributes.textDecorationLineType.value();
    if (textAttributes.textDecorationColor) {
      textDecorationColor = (uint32_t)(*textAttributes.textDecorationColor);
    } else if (textAttributes.foregroundColor) {
      textDecorationColor = (uint32_t)(*textAttributes.foregroundColor);
    }
    if (textDecorationType ==
            (int32_t)facebook::react::TextDecorationLineType::Strikethrough ||
        textDecorationType ==
            (int32_t)facebook::react::TextDecorationLineType::
                UnderlineStrikethrough) {
      textDecorationType = TEXT_DECORATION_LINE_THROUGH;
    }
  }
  if (textAttributes.textDecorationStyle.has_value()) {
    textDecorationStyle = (int32_t)textAttributes.textDecorationStyle.value();
  }
  OH_Drawing_SetTextStyleDecoration(textStyle.get(), textDecorationType);
  OH_Drawing_SetTextStyleDecorationColor(textStyle.get(), textDecorationColor);
  OH_Drawing_SetTextStyleDecorationStyle(textStyle.get(), textDecorationStyle);

  // backgroundColor
  if (textAttributes.backgroundColor) {
    std::unique_ptr<OH_Drawing_Brush, decltype(&OH_Drawing_BrushDestroy)> brush(
        OH_Drawing_BrushCreate(), OH_Drawing_BrushDestroy);
    OH_Drawing_BrushSetColor(
        brush.get(), (uint32_t)(*textAttributes.backgroundColor));
    OH_Drawing_SetTextStyleBackgroundBrush(textStyle.get(), brush.get());
  }

  // shadow
  if (facebook::react::isColorMeaningful(textAttributes.textShadowColor)) {
    std::unique_ptr<
        OH_Drawing_TextShadow,
        decltype(&OH_Drawing_DestroyTextShadow)>
        shadow(OH_Drawing_CreateTextShadow(), OH_Drawing_DestroyTextShadow);
    auto color = *textAttributes.textShadowColor;
    auto radius = textAttributes.textShadowRadius;
    auto offset =
        textAttributes.textShadowOffset.value_or(facebook::react::Size{0., 0.});
    std::unique_ptr<OH_Drawing_Point, decltype(&OH_Drawing_PointDestroy)>
        offsetPoint(
            OH_Drawing_PointCreate(
                offset.width * m_scale, offset.height * m_scale),
            OH_Drawing_PointDestroy);

    OH_Drawing_SetTextShadow(shadow.get(), color, offsetPoint.get(), radius);
    OH_Drawing_TextStyleAddShadow(textStyle.get(), shadow.get());
  }

  if (!std::isnan(textAttributes.letterSpacing)) {
    OH_Drawing_SetTextStyleLetterSpacing(
        textStyle.get(), m_scale * textAttributes.letterSpacing);
  }
  if (!std::isnan(textAttributes.lineHeight) && textAttributes.lineHeight > 0) {
    OH_Drawing_SetTextStyleFontHeight(
        textStyle.get(), getOHDrawingFontHeight(textAttributes));
  }
  if (textAttributes.fontWeight.has_value()) {
    OH_Drawing_SetTextStyleFontWeight(
        textStyle.get(),
        mapValueToFontWeight(int(textAttributes.fontWeight.value())));
  }

  // fontFamily
  std::vector<const char*> fontFamilies;
  if (!m_themeFontFamilyName.empty()) {
    fontFamilies.emplace_back(m_themeFontFamilyName.c_str());
  }
  if (!textAttributes.fontFamily.empty()) {
    fontFamilies.emplace_back(textAttributes.fontFamily.c_str());
  }
  if (!fontFamilies.empty()) {
    OH_Drawing_SetTextStyleFontFamilies(
        textStyle.get(), fontFamilies.size(), fontFamilies.data());
  }

  if (textAttributes.fontVariant.has_value()) {
    auto fontVariant = int(textAttributes.fontVariant.value());

    OH_Drawing_TextStyleAddFontFeature(
        textStyle.get(),
        "smcp",
        static_cast<int>((fontVariant & (int)FontVariant::SmallCaps) != 0));

    OH_Drawing_TextStyleAddFontFeature(
        textStyle.get(),
        "onum",
        static_cast<int>((fontVariant & (int)FontVariant::OldstyleNums) != 0));
    OH_Drawing_TextStyleAddFontFeature(
        textStyle.get(),
        "lnum",
        static_cast<int>((fontVariant & (int)FontVariant::LiningNums) != 0));
    OH_Drawing_TextStyleAddFontFeature(
        textStyle.get(),
        "tnum",
        static_cast<int>((fontVariant & (int)FontVariant::TabularNums) != 0));
    OH_Drawing_TextStyleAddFontFeature(
        textStyle.get(),
        "pnum",
        static_cast<int>(
            (fontVariant & (int)FontVariant::ProportionalNums) != 0));
  }

  auto fragmentContent = text;
  if (textAttributes.textTransform.has_value()) {
    textCaseTransform(fragmentContent, textAttributes.textTransform.value());
  }

  // push text and corresponding textStyle to handler
  OH_ArkUI_StyledString_PushTextStyle(m_styledString.get(), textStyle.get());
  OH_ArkUI_StyledString_AddText(m_styledString.get(), fragmentContent.c_str());
  m_fragmentLengths.emplace_back(utf16Length(text));
}
void StyledStringWrapper::addAttachment(const facebook::react::Size& size) {
  // No ThreadGuard here — see note in addFragment(); serialized via
  // TextStorageBuilder::m_mutex in the parallel FFRT text pipeline.
  ensureUnsealed();
  // using placeholder span for inline views
  OH_Drawing_PlaceholderSpan inlineView = {
      size.width * m_scale,
      size.height * m_scale,
      // align to the line's baseline
      ALIGNMENT_ABOVE_BASELINE,
      TEXT_BASELINE_ALPHABETIC,
      0,
  };
  // push placeholder to handler
  OH_ArkUI_StyledString_AddPlaceholder(m_styledString.get(), &inlineView);
  m_attachmentCount++;
}
}; // namespace rnoh
