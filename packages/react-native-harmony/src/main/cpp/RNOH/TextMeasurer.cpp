/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextMeasurer.h"
#include <cxxreact/SystraceSection.h>
#include <react/renderer/attributedstring/PlaceholderAttributedString.h>
#include <codecvt>
#include "TextStorage.h"

namespace rnoh {

using TextMeasurement = facebook::react::TextMeasurement;
using AttributedString = facebook::react::AttributedString;
using ParagraphAttributes = facebook::react::ParagraphAttributes;
using LayoutConstraints = facebook::react::LayoutConstraints;

facebook::react::TextMeasurement TextMeasurer::measure(
    const facebook::react::AttributedString& attributedString,
    const facebook::react::ParagraphAttributes& paragraphAttributes,
    const facebook::react::TextLayoutContext& layoutContext,
    const facebook::react::LayoutConstraints& layoutConstraints) const {
  facebook::react::SystraceSection s("#RNOH::TextMeasurer::measure");
  auto attributedStringWithPlaceholder =
      facebook::react::ensurePlaceholderIfEmpty_DO_NOT_USE(attributedString);
  TextStorage::Shared textStorage = TextStorageProvider::get(
      attributedStringWithPlaceholder,
      paragraphAttributes,
      layoutContext,
      layoutConstraints,
      [](TextStorage::Shared textStorage) { return true; });
  auto clampedSize =
      facebook::react::LayoutConstraints{
          layoutConstraints.minimumSize,
          {layoutConstraints.maximumSize.width,
           std::numeric_limits<facebook::react::Float>::infinity()},
          layoutConstraints.layoutDirection}
          .clamp(textStorage->measurement.size);
  if (std::fabs(textStorage->leftPadding) < 1) {
    ZeroLeftPaddingCacheKey key{
        attributedStringWithPlaceholder,
        paragraphAttributes,
        layoutContext,
        static_cast<int>(ceil(
            textStorage->measurement.size.width *
            layoutContext.pointScaleFactor))};
    m_zeroLeftPaddingCache.get(key, [&textStorage]() { return textStorage; });
  }
  return {clampedSize, textStorage->measurement.attachments};
}

TextStorage::Shared TextMeasurer::getHostTextStorage(
    const facebook::react::AttributedString& attributedString,
    const facebook::react::ParagraphAttributes& paragraphAttributes,
    const facebook::react::TextLayoutContext& layoutContext,
    const facebook::react::Size& size) const {
  static constexpr std::array<int, 4> errors = {-1, 0, +1, -2};
  auto attributedStringWithPlaceholder =
      facebook::react::ensurePlaceholderIfEmpty_DO_NOT_USE(attributedString);
  auto ceiledPixelWidth =
      static_cast<int>(ceil(size.width * layoutContext.pointScaleFactor));
  for (auto error : errors) {
    auto cached = m_zeroLeftPaddingCache.get(
        {attributedStringWithPlaceholder,
         paragraphAttributes,
         layoutContext,
         ceiledPixelWidth + error});
    if (cached.has_value() && cached.value()) {
      return cached.value();
    }
  }
  TextStorage::Shared textStorage = TextStorageProvider::get(
      attributedStringWithPlaceholder,
      paragraphAttributes,
      layoutContext,
      {size, size},
      [](TextStorage::Shared textStorage) {
        return std::fabs(textStorage->leftPadding) < 1;
      });
  if (textStorage->arkUITypography.getLineCount() > 1 && size.width > 0) {
    facebook::react::LayoutConstraints unboundedConstraints = {
        {0, 0},
        {std::numeric_limits<facebook::react::Float>::infinity(),
         std::numeric_limits<facebook::react::Float>::infinity()},
        facebook::react::LayoutDirection::LeftToRight};
    auto naturalTextStorage = TextStorage::create(
        attributedStringWithPlaceholder,
        paragraphAttributes,
        layoutContext,
        unboundedConstraints,
        *m_fontRegistry);
    if (naturalTextStorage &&
        naturalTextStorage->arkUITypography.getLineCount() <
            textStorage->arkUITypography.getLineCount() &&
        naturalTextStorage->measurement.size.width <= size.width + 0.5) {
      auto intrinsicWidth = naturalTextStorage->measurement.size.width;
      facebook::react::LayoutConstraints intrinsicConstraints = {
          {0, 0},
          {intrinsicWidth, intrinsicWidth},
          facebook::react::LayoutDirection::LeftToRight};
      auto refitTextStorage = TextStorage::create(
          attributedStringWithPlaceholder,
          paragraphAttributes,
          layoutContext,
          intrinsicConstraints,
          *m_fontRegistry);
      if (refitTextStorage &&
          refitTextStorage->arkUITypography.getLineCount() <=
              naturalTextStorage->arkUITypography.getLineCount()) {
        textStorage = refitTextStorage;
      } else {
        textStorage = naturalTextStorage;
      }
    }
  }
  return textStorage;
}

facebook::react::LinesMeasurements TextMeasurer::measureLines(
    const facebook::react::AttributedStringBox& attributedStringBox,
    const facebook::react::ParagraphAttributes& paragraphAttributes,
    const facebook::react::Size& size) const {
  facebook::react::SystraceSection s("#RNOH::TextMeasurer::measureLines");
  const auto& attributedString = attributedStringBox.getValue();
  auto paragraphAttributesWithoutMaxLines = paragraphAttributes;
  paragraphAttributesWithoutMaxLines.maximumNumberOfLines = {};
  auto textStorage = getHostTextStorage(
      attributedString, paragraphAttributesWithoutMaxLines, {m_scale.load()},
      size);
  auto& typography = textStorage->arkUITypography;
  bool hasAttachmentCharacter = false;
  for (auto const& fragment : attributedString.getFragments()) {
    if (fragment.isAttachment()) {
      hasAttachmentCharacter = true;
      break;
    }
  }
  std::string textStr;
  size_t totalLength = 0;
  for (auto const& fragment : attributedString.getFragments()) {
    totalLength += fragment.string.size();
  }
  textStr.reserve(totalLength);
  for (auto const& fragment : attributedString.getFragments()) {
    textStr += fragment.string;
  }
  std::u16string u16Text =
      std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
          .from_bytes(textStr);
  return typography.getLinesMeasurements(u16Text, hasAttachmentCharacter);
};

} // namespace rnoh
