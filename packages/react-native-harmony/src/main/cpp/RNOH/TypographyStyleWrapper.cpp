/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TypographyStyleWrapper.h"
#include <native_drawing/drawing_text_typography.h>
#include "RNOH/TextConversions.h"

namespace rnoh {

int32_t getOHDrawingTextAlign(const facebook::react::TextAlignment& textAlign) {
  switch (textAlign) {
    case facebook::react::TextAlignment::Left:
      return OH_Drawing_TextAlign::TEXT_ALIGN_LEFT;
    case facebook::react::TextAlignment::Right:
      return OH_Drawing_TextAlign::TEXT_ALIGN_RIGHT;
    case facebook::react::TextAlignment::Center:
      return OH_Drawing_TextAlign::TEXT_ALIGN_CENTER;
    case facebook::react::TextAlignment::Justified:
      return OH_Drawing_TextAlign::TEXT_ALIGN_JUSTIFY;
    default:
      return OH_Drawing_TextAlign::TEXT_ALIGN_START;
  }
}

int32_t getOHDrawingTextDirection(
    const facebook::react::WritingDirection& writingDirection) {
  switch (writingDirection) {
    case facebook::react::WritingDirection::LeftToRight:
      return OH_Drawing_TextDirection::TEXT_DIRECTION_LTR;
    case facebook::react::WritingDirection::RightToLeft:
      return OH_Drawing_TextDirection::TEXT_DIRECTION_RTL;
    default:
      return OH_Drawing_TextDirection::TEXT_DIRECTION_LTR;
  }
}

OH_Drawing_EllipsisModal mapEllipsizeMode(
    facebook::react::EllipsizeMode ellipsizeMode) {
  switch (ellipsizeMode) {
    case facebook::react::EllipsizeMode::Head:
      return ELLIPSIS_MODAL_HEAD;
    case facebook::react::EllipsizeMode::Middle:
      return ELLIPSIS_MODAL_MIDDLE;
    case facebook::react::EllipsizeMode::Tail:
      return ELLIPSIS_MODAL_TAIL;
    default:
      return ELLIPSIS_MODAL_TAIL;
  }
}

TypographyStyleWrapper::TypographyStyleWrapper(
    facebook::react::ParagraphAttributes const& paragraphAttributes)
    : m_typographyStyle(
          OH_Drawing_CreateTypographyStyle(),
          OH_Drawing_DestroyTypographyStyle) {
  if (paragraphAttributes.ellipsizeMode !=
      facebook::react::EllipsizeMode::Clip) {
    OH_Drawing_SetTypographyTextEllipsis(m_typographyStyle.get(), "...");
    OH_Drawing_SetTypographyTextEllipsisModal(
        m_typographyStyle.get(),
        mapEllipsizeMode(paragraphAttributes.ellipsizeMode));
  }

  if (paragraphAttributes.maximumNumberOfLines > 0) {
    OH_Drawing_SetTypographyTextMaxLines(
        m_typographyStyle.get(), paragraphAttributes.maximumNumberOfLines);
  }

  OH_Drawing_SetTypographyTextWordBreakType(
      m_typographyStyle.get(),
      TextConversions::getArkUIWordBreakStrategy(
          paragraphAttributes.textBreakStrategy));

  // RN 0.88: ParagraphAttributes 已移出 writingDirection 字段，
  // 文本方向改由 TextAttributes.layoutDirection 在 apply() 中处理。
}

void TypographyStyleWrapper::apply(
    const facebook::react::TextAttributes& textAttributes) {
  ensureUnsealed();
  auto textAlign = textAttributes.alignment;
  int32_t alignValue;
  if (textAlign.has_value() &&
      textAlign.value() != facebook::react::TextAlignment::Natural) {
    alignValue = getOHDrawingTextAlign(textAlign.value());
  } else if (
      textAttributes.layoutDirection ==
      facebook::react::LayoutDirection::RightToLeft) {
    alignValue = OH_Drawing_TextAlign::TEXT_ALIGN_RIGHT;
  } else if (
      textAttributes.layoutDirection ==
      facebook::react::LayoutDirection::LeftToRight) {
    alignValue = OH_Drawing_TextAlign::TEXT_ALIGN_LEFT;
  } else {
    alignValue = OH_Drawing_TextAlign::TEXT_ALIGN_START;
  }
  OH_Drawing_SetTypographyTextAlign(m_typographyStyle.get(), alignValue);

  auto textDirection = textAttributes.baseWritingDirection;
  if (textDirection.has_value()) {
    OH_Drawing_SetTypographyTextDirection(
        m_typographyStyle.get(),
        getOHDrawingTextDirection(textDirection.value()));
  }
  seal();
}

}; // namespace rnoh
