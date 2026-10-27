/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <react/renderer/core/Sealable.h>
#include "ArkUITypography.h"
#include "StyledStringWrapper.h"
#include "TextConversions.h"
#include "TextStorage.h"
#include "TypographyStyleWrapper.h"

namespace rnoh {

/**
 * @internal
 */
class TextStorageBuilder final : public facebook::react::Sealable {
 public:
  // builders that are longer than the threshold are discarded
  static constexpr double LENGTH_THRESHOLD = 2800;
  TextStorageBuilder(
      const facebook::react::ParagraphAttributes& paragraphAttributes,
      facebook::react::TextAttributes&& baseTextAttributes,
      facebook::react::Float pointScaleFactor,
      FontRegistry::Shared fontRegistry)
      : m_typographyStyle(paragraphAttributes),
        m_baseTextAttributes(baseTextAttributes),
        m_scale(pointScaleFactor),
        m_maxFontScale(m_baseTextAttributes.maxFontSizeMultiplier),
        m_paragraphAttributes(paragraphAttributes),
        m_fontRegistry(fontRegistry),
        m_estimatedLength(0) {}

  /**
   * @threadSafe
   */
  void appendRawFragment(
      const facebook::react::AttributedString::Fragment& rawFragment);

  /**
   * @threadSafe
   */
  void setAttributedString(
      const facebook::react::AttributedString& attributedString);

  /**
   * @threadSafe
   */
  void layoutTypography(vp maxWidth);

  /**
   * @threadSafe
   */
  bool empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_styledString || !m_typography.has_value();
  }

  /**
   * @threadSafe
   */
  TextStorage::Shared build();

  /**
   * @threadSafe
   */
  facebook::react::AttributedString const& getAttributedString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_attributedString;
  }

  /**
   * @threadSafe
   */
  facebook::react::ParagraphAttributes const& getParagraphAttributes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_paragraphAttributes;
  }

 private:
  bool m_typographyStyleApplied = false;
  TypographyStyleWrapper m_typographyStyle;
  StyledStringWrapper::Shared m_styledString;
  std::optional<ArkUITypography> m_typography;

  const facebook::react::TextAttributes m_baseTextAttributes;
  facebook::react::Float m_scale;
  facebook::react::Float m_maxFontScale;
  facebook::react::AttributedString m_attributedString{};
  const facebook::react::ParagraphAttributes m_paragraphAttributes;

  double m_estimatedLength;

  FontRegistry::Shared m_fontRegistry;

  mutable std::mutex m_mutex;
};

} // namespace rnoh
