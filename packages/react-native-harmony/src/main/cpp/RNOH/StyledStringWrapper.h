/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <arkui/styled_string.h>
#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/attributedstring/primitives.h>
#include <react/renderer/core/Sealable.h>
#include <react/renderer/graphics/Size.h>
#include <react/renderer/textlayoutmanager/TextLayoutManager.h>
#include <memory>
#include "FontRegistry.h"

namespace facebook::react {
class TextAttributes;
}
namespace rnoh {

using SharedStyledString = std::shared_ptr<ArkUI_StyledString>;

using UniqueTextStyle = std::
    unique_ptr<OH_Drawing_TextStyle, decltype(&OH_Drawing_DestroyTextStyle)>;

double getOHDrawingFontSize(
    const facebook::react::TextAttributes& textAttributes,
    facebook::react::Float pointScaleFactor,
    facebook::react::Float maxFontSizeMultiplier);

/**
 * @internal
 */
class StyledStringWrapper final : public facebook::react::Sealable {
 public:
  using Shared = std::shared_ptr<StyledStringWrapper>;
  static Shared createStyledString(
      facebook::react::AttributedString const& attributedString,
      facebook::react::ParagraphAttributes const& paragraphAttributes,
      facebook::react::TextLayoutContext const& layoutContext,
      FontRegistry const& fontRegistry);
  StyledStringWrapper(
      OH_Drawing_TypographyStyle* typographyStyle,
      SharedFontCollection fontCollection,
      const std::string& themeFontFamily,
      float scale,
      facebook::react::Float maxFontScale)
      : m_styledString(
            OH_ArkUI_StyledString_Create(typographyStyle, fontCollection.get()),
            OH_ArkUI_StyledString_Destroy),
        m_scale(scale),
        m_maxFontScale(maxFontScale),
        m_fontCollection(std::move(fontCollection)),
        m_themeFontFamilyName(themeFontFamily),
        m_textAlign(OH_Drawing_TypographyGetTextAlign(typographyStyle)),
        m_textDirection(
            OH_Drawing_TypographyGetTextDirection(typographyStyle)) {}

  ArkUI_StyledString* get() const {
    seal();
    return m_styledString.get();
  }

  void addFragment(
      const facebook::react::AttributedString::Fragment& fragment) {
    // Thread affinity is intentionally NOT enforced here: the parallel
    // text pre-measurement pipeline (TextStorageProvider::performOnFFRTThread)
    // creates this object on one FFRT worker and may call addFragment/addText
    // from another worker of the same job_ring. All cross-thread access is
    // serialized by TextStorageBuilder::m_mutex, so a ThreadGuard would only
    // produce false aborts (SIGABRT) under heavy parallel text load.
    ensureUnsealed();
    if (!fragment.isAttachment()) {
      addText(fragment.string, fragment.textAttributes);
    } else {
      addAttachment(fragment.parentShadowView.layoutMetrics.frame.size);
    }
  }

  void addText(
      const std::string& text,
      const facebook::react::TextAttributes& textAttributes);

  void addAttachment(const facebook::react::Size& size);

 private:
  using FontVariant = facebook::react::FontVariant;

  SharedStyledString m_styledString;
  size_t m_attachmentCount = 0;
  std::vector<size_t> m_fragmentLengths{};
  SharedFontCollection m_fontCollection;
  float m_scale;
  facebook::react::Float m_maxFontScale;
  std::string m_themeFontFamilyName;
  OH_Drawing_TextAlign m_textAlign;
  OH_Drawing_TextDirection m_textDirection;

  friend class TextStorage;
  friend class TextStorageBuilder;
};
} // namespace rnoh
