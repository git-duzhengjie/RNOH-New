/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <memory>
#include "ArkUITypography.h"
#include "FontRegistry.h"
#include "StyledStringWrapper.h"

namespace rnoh {

/*
 * Maximum size of the textStorageCache.
 */
constexpr auto textStorageThreadSafeCacheSizeCap = size_t{2048};

/**
 * @threadSafe
 */
class TextStorage {
  using AttributedString = facebook::react::AttributedString;
  using ParagraphAttributes = facebook::react::ParagraphAttributes;
  using TextLayoutContext = facebook::react::TextLayoutContext;
  using LayoutConstraints = facebook::react::LayoutConstraints;

 public:
  using Shared = std::shared_ptr<TextStorage>;

  const std::shared_ptr<const StyledStringWrapper> styledString;
  const ArkUITypography arkUITypography;
  const facebook::react::Float pointScaleFactor;
  const facebook::react::TextMeasurement measurement;
  const px leftPadding;

  static TextStorage::Shared create(
      AttributedString const& attributedString,
      ParagraphAttributes const& paragraphAttributes,
      TextLayoutContext const& layoutContext,
      LayoutConstraints const& layoutConstraints,
      FontRegistry const& fontRegistry);

  static px getLeftPadding(
      const ArkUITypography& arkUITypography,
      OH_Drawing_TextAlign textAlign) {
    switch (textAlign) {
      case OH_Drawing_TextAlign::TEXT_ALIGN_CENTER:
        return (OH_Drawing_TypographyGetMaxWidth(arkUITypography.get()) -
                OH_Drawing_TypographyGetLongestLine(arkUITypography.get())) /
            2.0;
      case OH_Drawing_TextAlign::TEXT_ALIGN_RIGHT:
        return (
            OH_Drawing_TypographyGetMaxWidth(arkUITypography.get()) -
            OH_Drawing_TypographyGetLongestLine(arkUITypography.get()));
      default:
        return 0;
    }
  }

  TextStorage(
      std::shared_ptr<const StyledStringWrapper> styledString,
      ArkUITypography&& arkUITypography)
      : styledString(std::move(styledString)),
        arkUITypography(std::move(arkUITypography)),
        pointScaleFactor(this->styledString->m_scale),
        measurement(this->arkUITypography.getMeasurement()),
        leftPadding(getLeftPadding(
            this->arkUITypography,
            this->styledString->m_textAlign)) {}

 private:
  static TextStorage::Shared findFitFontSize(
      int maxFontSize,
      facebook::react::AttributedString const& attributedString,
      facebook::react::ParagraphAttributes const& paragraphAttributes,
      facebook::react::TextLayoutContext const& layoutContext,
      facebook::react::LayoutConstraints const& layoutConstraints,
      FontRegistry const& fontRegistry);
};

} // namespace rnoh
