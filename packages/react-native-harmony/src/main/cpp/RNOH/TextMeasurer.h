/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/textlayoutmanager/TextLayoutManager.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <react/utils/SimpleThreadSafeCache.h>
#include "FontRegistry.h"
#include "TextStorageProvider.h"

namespace rnoh {

class TextMeasurer;

class ZeroLeftPaddingCacheKey final {
 public:
  facebook::react::AttributedString attributedString;
  facebook::react::ParagraphAttributes paragraphAttributes;
  facebook::react::TextLayoutContext layoutContext;
  int ceiledPixelWidth;

  bool operator==(const ZeroLeftPaddingCacheKey& other) const {
    return facebook::react::areAttributedStringsEquivalentDisplayWise(
               attributedString, other.attributedString) &&
        paragraphAttributes == other.paragraphAttributes &&
        layoutContext.pointScaleFactor ==
        other.layoutContext.pointScaleFactor &&
        ceiledPixelWidth == other.ceiledPixelWidth;
  }
};

} // namespace rnoh

namespace std {
template <>
struct hash<rnoh::ZeroLeftPaddingCacheKey> {
  size_t operator()(const rnoh::ZeroLeftPaddingCacheKey& key) const noexcept {
    return folly::hash::hash_combine(
        0,
        facebook::react::attributedStringHashDisplayWise(key.attributedString),
        key.paragraphAttributes,
        key.layoutContext.pointScaleFactor,
        key.ceiledPixelWidth);
  }
};
} // namespace std

namespace rnoh {

class TextMeasurer final : public facebook::react::TextLayoutManagerDelegate,
                           public TextStorageProvider {
 public:
  using Weak = std::weak_ptr<TextMeasurer>;
  explicit TextMeasurer(FontRegistry::Shared fontRegistry)
      : TextStorageProvider(fontRegistry) {}

  ~TextMeasurer() {
    DLOG(INFO) << "~TextMeasurer";
  };

  /**
   * @threadSafe
   */
  facebook::react::TextMeasurement measure(
      const facebook::react::AttributedString& attributedString,
      const facebook::react::ParagraphAttributes& paragraphAttributes,
      const facebook::react::TextLayoutContext& layoutContext,
      const facebook::react::LayoutConstraints& layoutConstraints)
      const override;

  facebook::react::LinesMeasurements measureLines(
      const facebook::react::AttributedStringBox& attributedStringBox,
      const facebook::react::ParagraphAttributes& paragraphAttributes,
      const facebook::react::Size& size) const override;

  /**
   * @threadSafe
   */
  TextStorage::Shared getHostTextStorage(
      const facebook::react::AttributedString& attributedString,
      const facebook::react::ParagraphAttributes& paragraphAttributes,
      const facebook::react::TextLayoutContext& layoutContext,
      const facebook::react::Size& size) const;

 private:
  using ZeroLeftPaddingCache = facebook::react::SimpleThreadSafeCache<
      ZeroLeftPaddingCacheKey,
      TextStorage::Shared,
      textStorageThreadSafeCacheSizeCap>;
  mutable ZeroLeftPaddingCache m_zeroLeftPaddingCache;
};

} // namespace rnoh
