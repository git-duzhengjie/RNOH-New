/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextStorageBuilder.h"

#include <optional>

namespace rnoh {

void TextStorageBuilder::appendRawFragment(
    const facebook::react::AttributedString::Fragment& rawFragment) {
  std::lock_guard<std::mutex> lock(m_mutex);
  ensureUnsealed();

  if (rawFragment.string.empty() || m_estimatedLength > LENGTH_THRESHOLD) {
    return;
  }

  if (!m_styledString) {
    facebook::react::TextAttributes textAttributes = m_baseTextAttributes;
    textAttributes.apply(rawFragment.textAttributes);
    if (!m_typographyStyleApplied) {
      m_typographyStyle.apply(textAttributes);
      m_typographyStyleApplied = true;
    }
    m_styledString = std::make_shared<StyledStringWrapper>(
        m_typographyStyle.get(),
        m_fontRegistry->getFontCollection(),
        m_fontRegistry->getThemeFontFamily(),
        m_scale,
        m_maxFontScale);
  }

  facebook::react::TextAttributes textAttributes = m_baseTextAttributes;
  textAttributes.apply(rawFragment.textAttributes);
  m_estimatedLength += rawFragment.string.size() *
      getOHDrawingFontSize(textAttributes, m_scale, m_maxFontScale);
  if (m_estimatedLength <= LENGTH_THRESHOLD) {
    m_attributedString.ensureUnsealed();
    m_attributedString.appendFragment({rawFragment.string, textAttributes});
    m_styledString->addText(rawFragment.string, textAttributes);
  }
}


void TextStorageBuilder::setAttributedString(
    const facebook::react::AttributedString& attributedString) {
  std::lock_guard<std::mutex> lock(m_mutex);
  ensureUnsealed();

  m_attributedString = facebook::react::AttributedString{};
  m_styledString.reset();
  m_typography.reset();
  m_estimatedLength = 0;

  for (const auto& fragment : attributedString.getFragments()) {
    if (fragment.string.empty() || m_estimatedLength > LENGTH_THRESHOLD) {
      continue;
    }
    if (!m_styledString) {
      facebook::react::TextAttributes textAttributes = fragment.textAttributes;
      if (!m_typographyStyleApplied) {
        m_typographyStyle.apply(textAttributes);
        m_typographyStyleApplied = true;
      }
      m_styledString = std::make_shared<StyledStringWrapper>(
          m_typographyStyle.get(),
          m_fontRegistry->getFontCollection(),
          m_fontRegistry->getThemeFontFamily(),
          m_scale,
          m_maxFontScale);
    }
    m_estimatedLength += fragment.string.size() *
        getOHDrawingFontSize(fragment.textAttributes, m_scale, m_maxFontScale);
    if (m_estimatedLength <= LENGTH_THRESHOLD) {
      m_attributedString.ensureUnsealed();
      m_attributedString.appendFragment(
          {fragment.string, fragment.textAttributes, fragment.parentShadowView});
      m_styledString->addFragment(fragment);
    }
  }
}

void TextStorageBuilder::layoutTypography(vp maxWidth) {
  std::lock_guard<std::mutex> lock(m_mutex);
  ensureUnsealed();
  if (m_estimatedLength > LENGTH_THRESHOLD) {
    return;
  }
  if (m_styledString) {
    std::optional<facebook::react::TextAlignment> textAlign;
    auto ohTextAlign = m_styledString->m_textAlign;
    auto textDirection = m_styledString->m_textDirection;
    bool isRTL = textDirection == OH_Drawing_TextDirection::TEXT_DIRECTION_RTL;
    switch (ohTextAlign) {
      case OH_Drawing_TextAlign::TEXT_ALIGN_LEFT:
        textAlign = facebook::react::TextAlignment::Left;
        break;
      case OH_Drawing_TextAlign::TEXT_ALIGN_START:
        textAlign = isRTL ? facebook::react::TextAlignment::Right
                          : facebook::react::TextAlignment::Left;
        break;
      case OH_Drawing_TextAlign::TEXT_ALIGN_RIGHT:
        textAlign = facebook::react::TextAlignment::Right;
        break;
      case OH_Drawing_TextAlign::TEXT_ALIGN_END:
        textAlign = isRTL ? facebook::react::TextAlignment::Left
                          : facebook::react::TextAlignment::Right;
        break;
      case OH_Drawing_TextAlign::TEXT_ALIGN_CENTER:
        textAlign = facebook::react::TextAlignment::Center;
        break;
      case OH_Drawing_TextAlign::TEXT_ALIGN_JUSTIFY:
        textAlign = facebook::react::TextAlignment::Justified;
        break;
      default:
        break;
    }
    m_typography.emplace(
        m_styledString->get(),
        m_styledString->m_attachmentCount,
        m_styledString->m_fragmentLengths,
        maxWidth,
        m_scale,
        textAlign);
  }
}

TextStorage::Shared TextStorageBuilder::build() {
  std::lock_guard<std::mutex> lock(m_mutex);
  ensureUnsealed();
  seal();
  m_attributedString.seal();
  return std::make_shared<TextStorage>(
      std::move(m_styledString), std::move(m_typography).value());
}

} // namespace rnoh
