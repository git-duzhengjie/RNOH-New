/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextStorage.h"
#include <cmath>

#include <optional>

namespace rnoh {

namespace {
std::optional<facebook::react::TextAlignment> getTextAlignment(
    const facebook::react::AttributedString& attributedString) {
  if (!attributedString.getFragments().empty()) {
    const auto& fragment = attributedString.getFragments()[0];
    return fragment.textAttributes.alignment;
  }
  return {};
}
} // namespace

TextStorage::Shared TextStorage::create(
    facebook::react::AttributedString const& attributedString,
    facebook::react::ParagraphAttributes const& paragraphAttributes,
    facebook::react::TextLayoutContext const& layoutContext,
    facebook::react::LayoutConstraints const& layoutConstraints,
    FontRegistry const& fontRegistry) {
  if (paragraphAttributes.adjustsFontSizeToFit) {
    int maxFontSize = 0;
    for (const auto& fragment : attributedString.getFragments()) {
      maxFontSize =
          std::max(maxFontSize, (int)fragment.textAttributes.fontSize);
    }
    return findFitFontSize(
        maxFontSize,
        attributedString,
        paragraphAttributes,
        layoutContext,
        layoutConstraints,
        fontRegistry);
  }

  auto styledString = StyledStringWrapper::createStyledString(
      attributedString, paragraphAttributes, layoutContext, fontRegistry);
  auto textAlign = getTextAlignment(attributedString);
  auto typography = ArkUITypography(
      styledString->get(),
      styledString->m_attachmentCount,
      styledString->m_fragmentLengths,
      layoutConstraints.maximumSize.width,
      layoutContext.pointScaleFactor,
      textAlign);
  return std::make_shared<TextStorage>(
      std::move(styledString), std::move(typography));
}

auto TextStorage::findFitFontSize(
    int maxFontSize,
    facebook::react::AttributedString const& attributedString,
    facebook::react::ParagraphAttributes const& paragraphAttributes,
    facebook::react::TextLayoutContext const& layoutContext,
    facebook::react::LayoutConstraints const& layoutConstraints,
    FontRegistry const& fontRegistry) -> TextStorage::Shared {
  // Helper to (re)build a styledString + typography for a given attributed
  // string. The StyledStringWrapper is heap-allocated (Shared) so that after
  // get() seals it, we can still copy/share the pointer; we never move/copy the
  // sealed Sealable value itself.
  auto buildFor = [&](facebook::react::AttributedString const& str)
      -> std::pair<StyledStringWrapper::Shared, ArkUITypography> {
    auto styledString = StyledStringWrapper::createStyledString(
        str, paragraphAttributes, layoutContext, fontRegistry);
    auto textAlign = getTextAlignment(str);
    auto typography = ArkUITypography(
        styledString->get(),
        styledString->m_attachmentCount,
        styledString->m_fragmentLengths,
        layoutConstraints.maximumSize.width,
        layoutContext.pointScaleFactor,
        textAlign);
    return {styledString, std::move(typography)};
  };

  auto fits = [&](ArkUITypography const& typography) {
    return typography.getHeight() <= layoutConstraints.maximumSize.height &&
        (paragraphAttributes.maximumNumberOfLines == 0 ||
         !typography.didExceedMaxLines());
  };

  // check if already fit at the original size
  auto [initialStyledString, initialTypography] = buildFor(attributedString);
  if (fits(initialTypography)) {
    return std::make_shared<TextStorage>(
        std::move(initialStyledString), std::move(initialTypography));
  }

  // init params
  int minFontSize = 1;
  if (!std::isnan(paragraphAttributes.minimumFontScale)) {
    minFontSize = ceil(paragraphAttributes.minimumFontScale * maxFontSize);
  }
  int finalFontSize = minFontSize;
  int initFontSize = maxFontSize;

  // Holds the most recent successful fit. StyledStringWrapper::Shared is a
  // shared_ptr, so reassigning it never moves/copies the sealed Sealable value.
  StyledStringWrapper::Shared fittedStyledString;
  std::optional<ArkUITypography> fittedTypography;

  auto fittedAttributedString = attributedString;
  // find fit fontSize
  while (minFontSize <= maxFontSize) {
    int curFontSize = ceil((minFontSize + maxFontSize) * 1.0 / 2);
    float ratio = 1.0 * curFontSize / initFontSize;
    for (int i = 0; i < fittedAttributedString.getFragments().size(); ++i) {
      int newFontSize = ceil(
          attributedString.getFragments()[i].textAttributes.fontSize * ratio);
      fittedAttributedString.getFragments()[i].textAttributes.fontSize =
          newFontSize;
    }

    auto [styledString, typography] = buildFor(fittedAttributedString);
    if (fits(typography)) {
      // Keep the most recent successful fit; keep searching upward for a larger
      // fitting size.
      finalFontSize = curFontSize;
      fittedStyledString = std::move(styledString);
      fittedTypography.emplace(std::move(typography));
      minFontSize = curFontSize + 1;
    } else {
      maxFontSize = curFontSize - 1;
    }
  }

  // If no size ever fit, fall back to the smallest size we ever tried
  // (finalFontSize == minFontSize initial value), which is the closest we can
  // get to fitting. This matches the pre-refactor semantics.
  if (!fittedStyledString) {
    fittedAttributedString = attributedString;
    float ratio = 1.0 * finalFontSize / initFontSize;
    for (int i = 0; i < fittedAttributedString.getFragments().size(); ++i) {
      int newFontSize = ceil(
          attributedString.getFragments()[i].textAttributes.fontSize * ratio);
      fittedAttributedString.getFragments()[i].textAttributes.fontSize =
          newFontSize;
    }
    auto [styledString, typography] = buildFor(fittedAttributedString);
    fittedStyledString = std::move(styledString);
    fittedTypography.emplace(std::move(typography));
  }
  return std::make_shared<TextStorage>(
      std::move(fittedStyledString), std::move(*fittedTypography));
}

} // namespace rnoh
