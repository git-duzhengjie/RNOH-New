/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/textlayoutmanager/TextMeasureCache.h>
#include <react/utils/SimpleThreadSafeCache.h>
#include <yoga/YGEnums.h>
#include <atomic>
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "TaskExecutor/ThreadTaskRunner.h"
#include "TextConversions.h"
#include "TextStorageBuilder.h"
#include "react/renderer/uimanager/UIManagerNodeOperationHook.h"

namespace rnoh {

class TextStorageProvider;

class UnwrappedCacheKey final {
 public:
  facebook::react::AttributedString attributedString;
  facebook::react::ParagraphAttributes paragraphAttributes;
  facebook::react::TextLayoutContext layoutContext;
  // fontScale is baked into each TextStorage (as fontSizeMultiplier) at build
  // time, so it must be part of the cache key: when it changes (e.g. the user
  // adjusts accessibility font size), the old entries must no longer match,
  // otherwise measure() returns stale results computed for a different scale.
  float fontScale{1.0f};

  bool operator==(const UnwrappedCacheKey& other) const {
    return facebook::react::areAttributedStringsEquivalentDisplayWise(
               attributedString, other.attributedString) &&
        paragraphAttributes == other.paragraphAttributes &&
        layoutContext.pointScaleFactor == other.layoutContext.pointScaleFactor &&
        fontScale == other.fontScale;
  }
};

} // namespace rnoh

namespace std {
template <>
struct hash<rnoh::UnwrappedCacheKey> {
  size_t operator()(const rnoh::UnwrappedCacheKey& key) const noexcept {
    return folly::hash::hash_combine(
        0,
        facebook::react::attributedStringHashDisplayWise(key.attributedString),
        key.paragraphAttributes,
        key.layoutContext.pointScaleFactor,
        key.fontScale);
  }
};
} // namespace std

namespace rnoh {

class TextStorageProvider
    : public facebook::react::UIManagerNodeOperationHook,
      public std::enable_shared_from_this<TextStorageProvider> {
  using ShadowNode = facebook::react::ShadowNode;

 public:
  using Weak = std::weak_ptr<TextStorageProvider>;
  using Shared = std::shared_ptr<TextStorageProvider>;
  explicit TextStorageProvider(FontRegistry::Shared fontRegistry)
      : m_fontRegistry(std::move(fontRegistry)) {}

  ~TextStorageProvider() {
    DLOG(INFO) << "~TextStorageProvider";
  };

  void uiManagerDidCreateShadowNode(
      const ShadowNode& shadowNode) noexcept override;

  void uiManagerDidCloneShadowNode(
      const ShadowNode& shadowNode,
      const ShadowNode& clonedShadowNode) noexcept override {}

  void uiManagerDidAppendChildNode(
      const ShadowNode& parentShadowNode,
      const ShadowNode& childShadowNode) noexcept override;

  /**
   * @threadSafe
   */
  void clear();

  void setParams(float scale, float fontScale);

 protected:
  /**
   * @threadSafe
   */
  TextStorage::Shared get(
      const facebook::react::AttributedString& attributedString,
      const facebook::react::ParagraphAttributes& paragraphAttributes,
      const facebook::react::TextLayoutContext& layoutContext,
      const facebook::react::LayoutConstraints& layoutConstraints,
      std::function<bool(TextStorage::Shared)> validator) const;

 private:
  /**
   * @threadSafe
   */
  void emplaceBuilder(
      facebook::react::Tag tag,
      const facebook::react::TextAttributes& textAttributes,
      YGDirection direction,
      const facebook::react::ParagraphAttributes& paragraphAttributes);

  /**
   * @threadSafe
   */
  void updateBuilder(
      facebook::react::Tag tag,
      const facebook::react::AttributedString& attributedString);

  /**
   * @threadSafe
   */
  void completeBuilder(facebook::react::Tag tag, uint64_t revision = 0);

  /**
   * @threadSafe
   */
  void eraseBuilder(facebook::react::Tag tag);

  bool isLatestRevision(facebook::react::Tag tag, uint64_t revision) const;

  std::unordered_map<facebook::react::Tag, TextStorageBuilder>
      m_builderByParagraphTag;
  std::mutex m_builderMutex;

  std::unordered_map<facebook::react::Tag, uint64_t> m_pendingRevision;
  mutable std::mutex m_pendingRevisionMutex;

  using UnwrappedCache = facebook::react::SimpleThreadSafeCache<
      UnwrappedCacheKey,
      TextStorage::Shared,
      textStorageThreadSafeCacheSizeCap>;
  mutable UnwrappedCache m_unwrappedCache;

 protected:
  FontRegistry::Shared m_fontRegistry;
  // m_fontScale / m_scale are written by setParams() on the ArkTS thread and
  // read by the layout thread (emplaceBuilder). Use std::atomic to make those
  // reads/writes well-defined instead of a data race (torn reads on ARM).
  std::atomic<float> m_fontScale{1.0f};
  std::atomic<float> m_scale{1.0f};
};

} // namespace rnoh
