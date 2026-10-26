/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextStorageProvider.h"
#include "ffrt/cpp/pattern/job_ring.h"
#include "ffrt/cpp/pattern/job_utils.h"
#include "react/renderer/components/text/ParagraphShadowNode.h"
#include <cxxreact/TraceSection.h>
#include "RNOH/ParallelCheck.h"

namespace rnoh {

inline void performOnFFRTThread(
    facebook::react::Tag tag,
    std::function<void()>&& task) {
  switch ((tag / 2) % 4) {
    case 0: {
      static auto job_ring =
          ffrt::job_ring<true>::make(ffrt::job_ring_attr().qos(5));
      job_ring->submit(std::move(task));
      break;
    }
    case 1: {
      static auto job_ring =
          ffrt::job_ring<true>::make(ffrt::job_ring_attr().qos(5));
      job_ring->submit(std::move(task));
      break;
    }
    case 2: {
      static auto job_ring =
          ffrt::job_ring<true>::make(ffrt::job_ring_attr().qos(5));
      job_ring->submit(std::move(task));
      break;
    }
    case 3: {
      static auto job_ring =
          ffrt::job_ring<true>::make(ffrt::job_ring_attr().qos(5));
      job_ring->submit(std::move(task));
      break;
    }
  }
}

using TextMeasurement = facebook::react::TextMeasurement;
using AttributedString = facebook::react::AttributedString;
using ParagraphAttributes = facebook::react::ParagraphAttributes;
using LayoutConstraints = facebook::react::LayoutConstraints;
using ParagraphShadowNode = facebook::react::ParagraphShadowNode;

void TextStorageProvider::uiManagerDidCreateShadowNode(
    const ShadowNode& shadowNode) noexcept {
  if (shadowNode.getComponentHandle() != ParagraphShadowNode::Handle()) {
    return;
  }

  const auto& paragraphShadowNode =
      static_cast<const ParagraphShadowNode&>(shadowNode);
  const auto& concreteProps = paragraphShadowNode.getConcreteProps();
  if (!concreteProps.paragraphAttributes.adjustsFontSizeToFit) {
    auto tag = shadowNode.getTag();
    performOnFFRTThread(
        tag,
        [tag,
         paragraphAttributes = concreteProps.paragraphAttributes,
         textAttributes = concreteProps.textAttributes,
         direction = paragraphShadowNode.getYGDirection(),
         weakTextStorageProvider = weak_from_this()]() {
          facebook::react::TraceSection s(
              "#RNOH::TextStorageProvider::emplaceBuilder");
          if (auto textStorageProvider = weakTextStorageProvider.lock()) {
            textStorageProvider->emplaceBuilder(
                tag, textAttributes, direction, paragraphAttributes);
          }
        });
  }
}

void TextStorageProvider::uiManagerDidAppendChildNode(
    const ShadowNode& parentShadowNode,
    const ShadowNode& childShadowNode) noexcept {
  if (childShadowNode.getComponentHandle() != ParagraphShadowNode::Handle()) {
    return;
  }

  const auto& paragraphShadowNode =
      static_cast<const ParagraphShadowNode&>(childShadowNode);
  const auto& concreteProps = paragraphShadowNode.getConcreteProps();
  if (concreteProps.paragraphAttributes.adjustsFontSizeToFit) {
    return;
  }

  auto tag = childShadowNode.getTag();

  auto textAttributes = facebook::react::TextAttributes::defaultTextAttributes();
  textAttributes.fontSizeMultiplier = m_fontScale.load();
  textAttributes.apply(concreteProps.textAttributes);
  textAttributes.layoutDirection =
      paragraphShadowNode.getYGDirection() == YGDirectionRTL
      ? facebook::react::LayoutDirection::RightToLeft
      : facebook::react::LayoutDirection::LeftToRight;

  facebook::react::AttributedString attributedString;
  facebook::react::BaseTextShadowNode::Attachments attachments;
  facebook::react::BaseTextShadowNode::buildAttributedString(
      textAttributes, paragraphShadowNode, attributedString, attachments);

  if (attributedString.getFragments().empty()) {
    return;
  }

  performOnFFRTThread(
      tag,
      [tag,
       attributedString = std::move(attributedString),
       weakTextStorageProvider = weak_from_this()]() {
        facebook::react::TraceSection s(
            "#RNOH::TextStorageProvider::preMeasure");
        if (auto textStorageProvider = weakTextStorageProvider.lock()) {
          textStorageProvider->updateBuilder(tag, attributedString);
          textStorageProvider->completeBuilder(tag, 0);
        }
      });
}

bool TextStorageProvider::isLatestRevision(
    facebook::react::Tag tag,
    uint64_t revision) const {
  if (revision == 0) {
    return true;
  }
  std::lock_guard<std::mutex> lk(m_pendingRevisionMutex);
  auto it = m_pendingRevision.find(tag);
  return it != m_pendingRevision.end() && it->second == revision;
}

TextStorage::Shared TextStorageProvider::get(
    const facebook::react::AttributedString& attributedString,
    const facebook::react::ParagraphAttributes& paragraphAttributes,
    const facebook::react::TextLayoutContext& layoutContext,
    const facebook::react::LayoutConstraints& layoutConstraints,
    std::function<bool(TextStorage::Shared)> validator) const {
  auto fontScaleNow = m_fontScale.load();
  UnwrappedCacheKey queryKey{
      attributedString, paragraphAttributes, layoutContext, fontScaleNow};
  auto cached = m_unwrappedCache.get(queryKey);
  if (cached.has_value() && cached.value() &&
      layoutConstraints.maximumSize.width >=
          cached.value()->measurement.size.width &&
      validator(cached.value())) {
    facebook::react::TraceSection s("#RNOH::TextStorageProvider::get_cacheHit");
    return cached.value();
  }
  facebook::react::TraceSection s("#RNOH::TextStorageProvider::get_cacheMiss");

  auto textStorage = TextStorage::create(
      attributedString,
      paragraphAttributes,
      layoutContext,
      layoutConstraints,
      *m_fontRegistry);
  return textStorage;
}

void TextStorageProvider::emplaceBuilder(
    facebook::react::Tag tag,
    const facebook::react::TextAttributes& textAttributes,
    YGDirection direction,
    const facebook::react::ParagraphAttributes& paragraphAttributes) {
  auto baseTextAttributes =
      facebook::react::TextAttributes::defaultTextAttributes();
  baseTextAttributes.fontSizeMultiplier = m_fontScale.load();
  baseTextAttributes.apply(textAttributes);
  baseTextAttributes.layoutDirection = direction == YGDirectionRTL
      ? facebook::react::LayoutDirection::RightToLeft
      : facebook::react::LayoutDirection::LeftToRight;
  std::lock_guard<std::mutex> lock(m_builderMutex);
  RNOH_ASSERT(!m_builderByParagraphTag.count(tag));
  m_builderByParagraphTag.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(
          paragraphAttributes,
          std::move(baseTextAttributes),
          m_scale.load(),
          m_fontRegistry));
}

void TextStorageProvider::updateBuilder(
    facebook::react::Tag tag,
    const facebook::react::AttributedString& attributedString) {
  std::lock_guard<std::mutex> lock(m_builderMutex);
  auto it = m_builderByParagraphTag.find(tag);
  if (it == m_builderByParagraphTag.end()) {
    return;
  }
  it->second.setAttributedString(attributedString);
}

void TextStorageProvider::completeBuilder(
    facebook::react::Tag tag,
    uint64_t revision) {
  if (revision != 0) {
    std::lock_guard<std::mutex> lk(m_pendingRevisionMutex);
    auto it = m_pendingRevision.find(tag);
    if (it == m_pendingRevision.end() || it->second != revision) {
      return;
    }
    m_pendingRevision.erase(it);
  }

  facebook::react::TraceSection s(
      "#RNOH::TextStorageProvider::completeBuilder_build");
  facebook::react::AttributedString attributedStringSnapshot;
  facebook::react::ParagraphAttributes paragraphAttributesSnapshot;
  {
    std::lock_guard<std::mutex> lock(m_builderMutex);
    auto it = m_builderByParagraphTag.find(tag);
    if (it == m_builderByParagraphTag.end()) {
      return;
    }
    attributedStringSnapshot = it->second.getAttributedString();
    paragraphAttributesSnapshot = it->second.getParagraphAttributes();
  }

  if (attributedStringSnapshot.getFragments().empty()) {
    return;
  }

  auto textStorage = TextStorage::create(
      attributedStringSnapshot,
      paragraphAttributesSnapshot,
      facebook::react::TextLayoutContext{m_scale.load()},
      facebook::react::LayoutConstraints{},
      *m_fontRegistry);
  if (textStorage) {
    UnwrappedCacheKey key{
        std::move(attributedStringSnapshot),
        std::move(paragraphAttributesSnapshot),
        facebook::react::TextLayoutContext{textStorage->pointScaleFactor},
        m_fontScale.load()};
    m_unwrappedCache.get(key, [&textStorage]() { return textStorage; });
  }
}

void TextStorageProvider::eraseBuilder(facebook::react::Tag tag) {
  {
    std::lock_guard<std::mutex> lock(m_builderMutex);
    m_builderByParagraphTag.erase(tag);
  }
}

void TextStorageProvider::clear() {
  std::lock_guard<std::mutex> lock(m_builderMutex);
  m_builderByParagraphTag.clear();
}

void TextStorageProvider::setParams(float scale, float fontScale) {
  m_scale.store(scale);
  m_fontScale.store(fontScale);
}

} // namespace rnoh