/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <native_drawing/drawing_text_declaration.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/attributedstring/TextAttributes.h>
#include <react/renderer/core/Sealable.h>
#include "Assert.h"

namespace rnoh {

using SharedTypographyStyle = std::shared_ptr<OH_Drawing_TypographyStyle>;

/**
 * @internal
 */
class TypographyStyleWrapper final : public facebook::react::Sealable {
 public:
  explicit TypographyStyleWrapper(
      facebook::react::ParagraphAttributes const& paragraphAttributes);

  void apply(const facebook::react::TextAttributes& textAttributes);

  OH_Drawing_TypographyStyle* get() const {
    seal();
    return m_typographyStyle.get();
  }

 private:
  SharedTypographyStyle m_typographyStyle;
  friend class StyledStringWrapper;
};

} // namespace rnoh
