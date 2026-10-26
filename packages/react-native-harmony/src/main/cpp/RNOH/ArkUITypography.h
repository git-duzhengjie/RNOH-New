/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <arkui/styled_string.h>
#include <cmath>
#include <react/renderer/attributedstring/primitives.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <codecvt>
#include <memory>
#include <optional>
#include "Graphics.h"

namespace rnoh {

/**
 * @internal
 */
using UniqueTypographyStyle = std::unique_ptr<
    OH_Drawing_TypographyStyle,
    decltype(&OH_Drawing_DestroyTypographyStyle)>;

/**
 * @internal
 */
class ArkUITypography final {
 public:
  facebook::react::TextMeasurement::Attachments getAttachments() const {
    facebook::react::TextMeasurement::Attachments result;
    result.reserve(m_attachmentCount);
    std::shared_ptr<OH_Drawing_TextBox> placeholderRects(
        OH_Drawing_TypographyGetRectsForPlaceholders(m_typography.get()),
        OH_Drawing_TypographyDestroyTextBox);
    // calculate attachment sizes and positions
    for (auto i = 0; i < m_attachmentCount; i++) {
      float left = OH_Drawing_GetLeftFromTextBox(placeholderRects.get(), i);
      float top = OH_Drawing_GetTopFromTextBox(placeholderRects.get(), i);
      float right = OH_Drawing_GetRightFromTextBox(placeholderRects.get(), i);
      float bottom = OH_Drawing_GetBottomFromTextBox(placeholderRects.get(), i);

      facebook::react::TextMeasurement::Attachment attachment;
      bool isClipped = right - left == 0 && bottom - top == 0;
      attachment.isClipped = isClipped;
      attachment.frame.origin.x = (left + m_offset.x) / m_scale;
      attachment.frame.origin.y = (top + m_offset.y) / m_scale;
      attachment.frame.size.width = (right - left) / m_scale;
      attachment.frame.size.height = (bottom - top) / m_scale;
      result.push_back(std::move(attachment));
    }
    return result;
  }

  vp getHeight() const {
    return OH_Drawing_TypographyGetHeight(m_typography.get()) / m_scale;
  }

  vp getLongestLineWidth() const {
    return OH_Drawing_TypographyGetLongestLine(m_typography.get()) / m_scale;
  }

  size_t getLineCount() const {
    return OH_Drawing_TypographyGetLineCount(m_typography.get());
  }

  bool didExceedMaxLines() const {
    return OH_Drawing_TypographyDidExceedMaxLines(m_typography.get());
  }

  using Rects = std::vector<facebook::react::Rect>;

  std::vector<Rects> getRectsForFragments() const {
    std::vector<Rects> result;
    result.reserve(m_fragmentLengths.size());
    size_t fragmentBegin = 0;
    for (size_t i = 0; i < m_fragmentLengths.size(); i++) {
      auto fragmentEnd = fragmentBegin + m_fragmentLengths[i];
      std::shared_ptr<OH_Drawing_TextBox> textBoxes(
          OH_Drawing_TypographyGetRectsForRange(
              m_typography.get(),
              fragmentBegin,
              fragmentEnd,
              RECT_HEIGHT_STYLE_MAX,
              RECT_WIDTH_STYLE_MAX),
          OH_Drawing_TypographyDestroyTextBox);
      auto textBoxCount = OH_Drawing_GetSizeOfTextBox(textBoxes.get());
      Rects rects;
      rects.reserve(textBoxCount);
      for (size_t j = 0; j < textBoxCount; j++) {
        facebook::react::Rect rect;
        rect.origin.x =
            (OH_Drawing_GetLeftFromTextBox(textBoxes.get(), j) + m_offset.x) /
            m_scale;
        rect.origin.y =
            (OH_Drawing_GetTopFromTextBox(textBoxes.get(), j) + m_offset.y) /
            m_scale;
        rect.size.width = (OH_Drawing_GetRightFromTextBox(textBoxes.get(), j) -
                           OH_Drawing_GetLeftFromTextBox(textBoxes.get(), j)) /
            m_scale;
        rect.size.height =
            (OH_Drawing_GetBottomFromTextBox(textBoxes.get(), j) -
             OH_Drawing_GetTopFromTextBox(textBoxes.get(), j)) /
            m_scale;
        rects.emplace_back(std::move(rect));
      }
      result.emplace_back(std::move(rects));
      fragmentBegin = fragmentEnd;
    }
    return result;
  }

  facebook::react::TextMeasurement getMeasurement() const {
    return {
        {
            .width = getLongestLineWidth(),
            .height = getHeight(),
        },
        getAttachments()};
  }

  ArkUITypography(
      ArkUI_StyledString* styledString,
      size_t attachmentCount,
      std::vector<size_t> fragmentLengths,
      vp maxWidth,
      float scale,
      std::optional<facebook::react::TextAlignment> textAlign = {})
      : m_typography(
            OH_ArkUI_StyledString_CreateTypography(styledString),
            OH_Drawing_DestroyTypography),
        m_attachmentCount(attachmentCount),
        m_fragmentLengths(std::move(fragmentLengths)),
        m_scale(scale) {
    px scaledWidth = maxWidth * m_scale;
    if (std::isnan(scaledWidth) || scaledWidth <= 0) {
      scaledWidth = std::numeric_limits<decltype(scaledWidth)>::infinity();
    }
    OH_Drawing_TypographyLayout(m_typography.get(), scaledWidth);
    std::shared_ptr<OH_Drawing_LineMetrics> lineMetrics(
        OH_Drawing_TypographyGetLineMetrics(m_typography.get()),
        OH_Drawing_DestroyLineMetrics);
    if (lineMetrics) {
      m_offset.x = lineMetrics->x;
      m_offset.y = lineMetrics->y;
    }
    if (!textAlign.has_value() ||
        textAlign.value() != facebook::react::TextAlignment::Justified) {
      auto longestWidth =
          std::ceil(OH_Drawing_TypographyGetLongestLine(m_typography.get()));
      if (std::isfinite(scaledWidth) && scaledWidth > 0) {
        longestWidth = std::min(longestWidth, scaledWidth);
      }
      if (textAlign.has_value()) {
        switch (textAlign.value()) {
          case facebook::react::TextAlignment::Right:
            m_offset.x = scaledWidth - longestWidth;
            break;
          case facebook::react::TextAlignment::Center:
            m_offset.x = (scaledWidth - longestWidth) / 2;
            break;
          default:
            break;
        }
      }
      OH_Drawing_TypographyLayout(m_typography.get(), longestWidth);
    }
  }

  OH_Drawing_Typography* get() const {
    return m_typography.get();
  }

  facebook::react::LinesMeasurements getLinesMeasurements(
      std::u16string& u16Text,
      bool hasAttachmentCharacter) const {
    auto metrics = OH_Drawing_TypographyGetLineMetrics(m_typography.get());
    auto metricsSize = OH_Drawing_LineMetricsGetSize(metrics);
    facebook::react::LinesMeasurements lineMetrics;
    lineMetrics.reserve(metricsSize);
    for (int i = 0; i < metricsSize; i++) {
      OH_Drawing_TypographyGetLineMetricsAt(m_typography.get(), i, metrics);
      auto u16LineText = u16Text.substr(
          metrics->startIndex, metrics->endIndex - metrics->startIndex);
      if (hasAttachmentCharacter) {
        // NOTE: Use std::remove and erase to remove the placeholder character
        // `￼` in the UTF-16 string (corresponding to `0xFFFC` in UTF-16)
        u16LineText.erase(
            std::remove(u16LineText.begin(), u16LineText.end(), 0xFFFC),
            u16LineText.end());
      }
      std::string lineText =
          std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}
              .to_bytes(u16LineText);
      facebook::react::LineMeasurement measurement(
          lineText,
          {{metrics->x / m_scale, metrics->y / m_scale},
           {metrics->width / m_scale, metrics->height / m_scale}},
          metrics->descender / m_scale,
          metrics->capHeight / m_scale,
          metrics->ascender / m_scale,
          metrics->xHeight / m_scale);
      lineMetrics.push_back(std::move(measurement));
    }
    OH_Drawing_DestroyLineMetrics(metrics);
    return lineMetrics;
  }

 private:
  std::shared_ptr<OH_Drawing_Typography> m_typography;
  size_t m_attachmentCount;
  std::vector<size_t> m_fragmentLengths;
  facebook::react::Point m_offset{0, 0};

  float m_scale = 1.0;
};

} // namespace rnoh
