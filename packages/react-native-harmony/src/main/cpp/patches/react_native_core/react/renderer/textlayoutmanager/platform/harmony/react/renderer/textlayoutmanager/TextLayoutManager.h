/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/attributedstring/AttributedStringBox.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/graphics/Size.h>
#include <react/renderer/textlayoutmanager/TextLayoutContext.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <react/utils/ContextContainer.h>
#include <memory>

namespace facebook::react {

class TextLayoutManager;

using SharedTextLayoutManager = std::shared_ptr<const TextLayoutManager>;

class TextLayoutManagerDelegate {
 public:
  virtual TextMeasurement measure(
      const AttributedString& attributedString,
      const ParagraphAttributes& paragraphAttributes,
      const TextLayoutContext& layoutContext,
      const LayoutConstraints& layoutConstraints) const = 0;

  virtual LinesMeasurements measureLines(
      const AttributedStringBox& attributedStringBox,
      const ParagraphAttributes& paragraphAttributes,
      const Size& size) const = 0;
};

/*
 * Cross platform facade for text measurement (Harmony platform)
 */
class TextLayoutManager {
 public:
  TextLayoutManager(const std::shared_ptr<const ContextContainer>& contextContainer);
  virtual ~TextLayoutManager() = default;

  /*
   * Not copyable.
   */
  TextLayoutManager(const TextLayoutManager&) = delete;
  TextLayoutManager& operator=(const TextLayoutManager&) = delete;

  /*
   * Not movable.
   */
  TextLayoutManager(TextLayoutManager&&) = delete;
  TextLayoutManager& operator=(TextLayoutManager&&) = delete;

  /*
   * Measures `attributedStringBox` using native text rendering infrastructure.
   */
  TextMeasurement measure(
      const AttributedStringBox& attributedStringBox,
      const ParagraphAttributes& paragraphAttributes,
      const TextLayoutContext& layoutContext,
      const LayoutConstraints& layoutConstraints) const;

  /*
   * Measures lines of `attributedString` using native text rendering
   * infrastructure.
   */
  LinesMeasurements measureLines(
      const AttributedStringBox& attributedStringBox,
      const ParagraphAttributes& paragraphAttributes,
      const Size& size) const;

  /*
   * Calculates baseline of `attributedString` using native text rendering
   * infrastructure.
   */
  Float baseline(
      const AttributedStringBox& attributedStringBox,
      const ParagraphAttributes& paragraphAttributes,
      const Size& size) const;

  void* getNativeTextLayoutManager() const;

 protected:
  std::shared_ptr<const ContextContainer> contextContainer_;
  TextMeasureCache textMeasureCache_;
  std::shared_ptr<TextLayoutManagerDelegate> m_textLayoutManagerDelegate;
};

} // namespace facebook::react
