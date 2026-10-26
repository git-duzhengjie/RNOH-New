/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextLayoutManager.h"

namespace facebook::react {

TextLayoutManager::TextLayoutManager(
    const std::shared_ptr<const ContextContainer>& contextContainer)
    : contextContainer_(contextContainer),
      textMeasureCache_(kSimpleThreadSafeCacheSizeCap),
      m_textLayoutManagerDelegate(
          contextContainer->at<std::shared_ptr<TextLayoutManagerDelegate>>(
              "textLayoutManagerDelegate")) {}

void* TextLayoutManager::getNativeTextLayoutManager() const {
  return (void*)m_textLayoutManagerDelegate.get();
}

TextMeasurement TextLayoutManager::measure(
    const AttributedStringBox& attributedStringBox,
    const ParagraphAttributes& paragraphAttributes,
    const TextLayoutContext& layoutContext,
    const LayoutConstraints& layoutConstraints) const {
  const auto& attributedString = attributedStringBox.getValue();
  facebook::react::TextMeasureCacheKey key{
      attributedString, paragraphAttributes, layoutConstraints};
  return textMeasureCache_.get(key, [&]() {
    return m_textLayoutManagerDelegate->measure(
        attributedString,
        paragraphAttributes,
        layoutContext,
        layoutConstraints);
  });
}

LinesMeasurements TextLayoutManager::measureLines(
    const AttributedStringBox& attributedStringBox,
    const ParagraphAttributes& paragraphAttributes,
    const Size& size) const {
  return m_textLayoutManagerDelegate->measureLines(
      attributedStringBox, paragraphAttributes, size);
};

Float TextLayoutManager::baseline(
    const AttributedStringBox& attributedStringBox,
    const ParagraphAttributes& paragraphAttributes,
    const Size& size) const {
  // Not implemented for Harmony
  return 0;
}

} // namespace facebook::react
