/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextComponentInstance.h"
#include <cmath>
#include <glog/logging.h>
#include <react/renderer/components/text/ParagraphProps.h>
#include <react/renderer/components/text/ParagraphState.h>
#include <react/renderer/core/ConcreteState.h>
#include <memory>
#include <sstream>
#include "RNOH/TextConversions.h"
#include "RNOH/TextMeasurer.h"
#include "react/renderer/attributedstring/primitives.h"

namespace rnoh {
const static float DEFAULT_LINE_SPACING = 0.15f;

TextComponentInstance::TextComponentInstance(Context context)
    : CppComponentInstance(std::move(context)), m_textNode(m_arkUINodeCtx) {
  m_textNode.setAlignment(ARKUI_ALIGNMENT_TOP_START);
  m_lastNodeAlignment = static_cast<int32_t>(ARKUI_ALIGNMENT_TOP_START);
  m_textNode.setTextOverflow(ARKUI_TEXT_OVERFLOW_ELLIPSIS);
}

TextComponentInstance::~TextComponentInstance() {
  this->disposeTextStorage();
}

void TextComponentInstance::onChildInserted(
    ComponentInstance::Shared const& /*childComponentInstance*/,
    std::size_t /*index*/) {
  throw RNOHError("TextComponentInstance does not support children");
}

void TextComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& /*childComponentInstance*/) {
  throw RNOHError("TextComponentInstance does not support children");
}

const std::string& TextComponentInstance::getAccessibilityLabel() const {
  auto const& superAccessibilityLabel =
      CppComponentInstance::getAccessibilityLabel();
  return superAccessibilityLabel.empty() ? m_textContent
                                         : superAccessibilityLabel;
}

void TextComponentInstance::onPropsChanged(
    SharedConcreteProps const& textProps) {
  CppComponentInstance::onPropsChanged(textProps);
  if (textProps == nullptr) {
    return;
  }
  // padding
  TextPaddingInfo info = TextConversions::getArkUIPadding(textProps);
  VLOG(3) << "[text-debug] textProps->setPadding top=" << info.top
          << ", right=" << info.right << ", bottom=" << info.bottom
          << ", left=" << info.left;
  if (info.top != 0.f || info.right != 0.f || info.bottom != 0.f ||
      info.left != 0.f) {
    m_textNode.setPadding(info.top, info.right, info.bottom, info.left);
  }

  // Copy Option
  VLOG(3) << "[text-debug] textProps->isSelectable=" << textProps->isSelectable;
  if (textProps->isSelectable) {
    m_textNode.setTextCopyOption(
        ArkUI_CopyOptions::ARKUI_COPY_OPTIONS_LOCAL_DEVICE);
  }

  if (textProps->rawProps != nullptr) {
    // stack align
    facebook::react::TextAlignment alignHorizon =
        facebook::react::TextAlignment::Left;
    if (textProps->rawProps.count("writingDirection") != 0 &&
        textProps->rawProps["writingDirection"] == "rtl") {
      alignHorizon = facebook::react::TextAlignment::Right;
    }
    std::string alignVertical = "top";
    if (textProps->rawProps.count("textAlignVertical") != 0 &&
        textProps->rawProps["textAlignVertical"].isString()) {
      alignVertical = textProps->rawProps["textAlignVertical"].asString();
    }
    if (textProps->textAttributes.alignment.has_value() &&
        textProps->textAttributes.alignment.value() !=
            facebook::react::TextAlignment::Natural) {
      alignHorizon = textProps->textAttributes.alignment.value();
    } else if (
        m_layoutMetrics.layoutDirection ==
        facebook::react::LayoutDirection::RightToLeft) {
      alignHorizon = facebook::react::TextAlignment::Right;
    }
    auto alignment = TextConversions::getAlignment(alignHorizon, alignVertical);

    facebook::react::TextAlignment alignHorizonOld =
        facebook::react::TextAlignment::Left;
    std::string alignVerticalOld = "top";
    if (m_props->rawProps.count("textAlignVertical") != 0 &&
        m_props->rawProps["textAlignVertical"].isString()) {
      alignVerticalOld = m_props->rawProps["textAlignVertical"].asString();
    }
    if (m_props->textAttributes.alignment.has_value() &&
        m_props->textAttributes.alignment.value() !=
            facebook::react::TextAlignment::Natural) {
      alignHorizonOld = m_props->textAttributes.alignment.value();
    } else if (
        m_layoutMetrics.layoutDirection ==
        facebook::react::LayoutDirection::RightToLeft) {
      alignHorizonOld = facebook::react::TextAlignment::Right;
    }
    auto oldAlignment =
        TextConversions::getAlignment(alignHorizonOld, alignVerticalOld);

    if (alignment != oldAlignment) {
      VLOG(3) << "[text-debug] alignment=" << alignment
              << ", oldAlignment=" << oldAlignment;
      m_textNode.setAlignment(alignment);
      m_lastNodeAlignment = static_cast<int32_t>(alignment);
    }

    // enable
    if (textProps->rawProps.count("disabled") != 0) {
      bool enableFlag = textProps->rawProps["disabled"].asBool();
      VLOG(3) << "[text-debug] rawProps->disabled=" << enableFlag;
      m_textNode.setTextEnable(!enableFlag);
    }

    // selectionColor
    if (textProps->rawProps.count("selectionColor") != 0 &&
        !textProps->rawProps["selectionColor"].isNull()) {
      uint32_t selectionColor = textProps->rawProps["selectionColor"].asInt();
      selectionColor = (selectionColor & 0x00ffffff) | 0x33000000;
      VLOG(3) << "[text-debug] selectionColor: " << selectionColor;
      m_textNode.setSelectedBackgroundColor(selectionColor);
    }

    // dataDetectorType
    if (textProps->rawProps.count("dataDetectorType") != 0) {
      const auto& dataDetectorType = textProps->rawProps["dataDetectorType"];
      VLOG(3) << "[text-debug] dataDetectorType: "
              << dataDetectorType.asString();
      auto dataDetectorEnable =
          TextConversions::getArkUITextDataDetectorEnable(dataDetectorType);
      VLOG(3) << "[text-debug] dataDetectorEnable: " << dataDetectorEnable;
      m_textNode.setTextDataDetectorEnable(dataDetectorEnable);
      if (dataDetectorEnable) {
        auto arkUIDataDetectorType =
            TextConversions::getArkUITextDataDetectorTypes(dataDetectorType);
        if (arkUIDataDetectorType.has_value()) {
          VLOG(3) << "[text-debug] arkUIDataDetectorType: "
                  << arkUIDataDetectorType.value();
          m_textNode.setTextDataDetectorType(arkUIDataDetectorType.value());
        }
      }
    }
  }

  VLOG(3) << "[text-debug] setProps end";
}

void TextComponentInstance::onFinalizeUpdates() {
  CppComponentInstance::onFinalizeUpdates();
  auto parent = m_parent.lock();
  if (m_props->importantForAccessibility ==
          facebook::react::ImportantForAccessibility::Auto &&
      parent && !parent->getAccessibilityGroup()) {
    m_textNode.setAccessibilityMode(
        ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_ENABLED);
  }
}

void TextComponentInstance::onParentAccessibilityGroupChanged() {
  updateAccessibilityMode();
}

void TextComponentInstance::updateAccessibilityMode() {
  if (m_props &&
      m_props->importantForAccessibility ==
          facebook::react::ImportantForAccessibility::Auto) {
    auto parent = m_parent.lock();
    if (parent && parent->getAccessibilityGroup()) {
      // Parent has accessibility group, let system decide text accessibility
      // mode
      m_textNode.setAccessibilityMode(
          ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_AUTO);
    } else {
      // Parent has no accessibility group or parent is null, text should be
      // accessible
      m_textNode.setAccessibilityMode(
          ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_ENABLED);
    }
  }
}

void TextComponentInstance::onStateChanged(
    SharedConcreteState const& textState) {
  CppComponentInstance::onStateChanged(textState);
  m_touchTargetChildrenNeedUpdate = true;
  auto const& stateData = textState->getData();
  auto const& fragments = stateData.attributedString.getFragments();
  this->disposeTextStorage();
  if (fragments.empty()) {
    return;
  }
  auto const& paragraphLayoutManager = stateData.layoutManager.lock();

  auto textMeasurer = static_cast<rnoh::TextMeasurer*>(
      paragraphLayoutManager->getNativeTextLayoutManager());
  RNOH_ASSERT(textMeasurer);

  auto contentSize = m_layoutMetrics.getContentFrame().size;
  if (std::isnan(contentSize.width) || contentSize.width <= 0) {
    return;
  }

  m_textStorage = textMeasurer->getHostTextStorage(
      stateData.attributedString,
      stateData.paragraphAttributes,
      {m_layoutMetrics.pointScaleFactor},
      contentSize);

  RNOH_ASSERT(m_textStorage);
  m_textNode.setTextContentWithStyledString(m_textStorage->styledString->get());
  m_textContent = stateData.attributedString.getString();

  this->setTextAttributes(fragments[0].textAttributes);
}

void TextComponentInstance::setTextAttributes(
    const facebook::react::TextAttributes& textAttributes) {
  // TextAlign
  if (textAttributes.alignment.has_value()) {
    int32_t align = TextConversions::getArkUITextAlignment(
        textAttributes.alignment.value());
    VLOG(3) << "[text-debug] textAttributes.alignment=" << align;
    m_textNode.setTextAlign(align);
  }

  facebook::react::TextAlignment alignHorizon =
      facebook::react::TextAlignment::Left;
  if (textAttributes.alignment.has_value() &&
      textAttributes.alignment.value() !=
          facebook::react::TextAlignment::Natural) {
    alignHorizon = textAttributes.alignment.value();
  } else if (
      textAttributes.layoutDirection ==
      facebook::react::LayoutDirection::RightToLeft) {
    alignHorizon = facebook::react::TextAlignment::Right;
  }
  std::string alignVertical = "top";
  if (m_props != nullptr && m_props->rawProps != nullptr &&
      m_props->rawProps.count("textAlignVertical") != 0 &&
      m_props->rawProps["textAlignVertical"].isString()) {
    alignVertical = m_props->rawProps["textAlignVertical"].asString();
  }
  auto nodeAlignment =
      TextConversions::getAlignment(alignHorizon, alignVertical);
  if (static_cast<int32_t>(nodeAlignment) != m_lastNodeAlignment) {
    m_textNode.setAlignment(nodeAlignment);
    m_lastNodeAlignment = static_cast<int32_t>(nodeAlignment);
  }
}

TextNode& TextComponentInstance::getLocalRootArkUINode() {
  return m_textNode;
}

class TextFragmentTouchTarget : public TouchTarget {
 public:
  TextFragmentTouchTarget(
      facebook::react::Tag tag,
      TouchTarget::Weak parentTouchTarget,
      facebook::react::SharedTouchEventEmitter touchEventEmitter)
      : m_tag(tag),
        m_touchEventEmitter(std::move(touchEventEmitter)),
        m_parentTouchTarget(parentTouchTarget) {}

  void clearRects() {
    m_rects.clear();
  }

  void insertRects(std::vector<facebook::react::Rect> const& rects) {
    m_rects.insert(m_rects.end(), rects.begin(), rects.end());
  }

  facebook::react::SharedTouchEventEmitter getTouchEventEmitter()
      const override {
    return m_touchEventEmitter;
  }

  facebook::react::Point computeChildPoint(
      facebook::react::Point const& point,
      TouchTarget::Shared const& /*child*/) const override {
    return point;
  }

  bool containsPoint(facebook::react::Point const& point) const override {
    for (auto rect : m_rects) {
      if (rect.containsPoint(point)) {
        return true;
      }
    }
    return false;
  }

  bool containsPointInBoundingBox(
      facebook::react::Point const& point) override {
    return containsPoint(point);
  }

  bool canHandleTouch() const override {
    return m_touchEventEmitter != nullptr;
  }

  bool canChildrenHandleTouch() const override {
    return false;
  }

  bool canSubtreeHandleTouch(facebook::react::Point const& point) override {
    return false;
  }

  facebook::react::Tag getTouchTargetTag() const override {
    return m_tag;
  }

  TouchTarget::Shared getTouchTargetParent() const override {
    return m_parentTouchTarget.lock();
  };

  std::vector<TouchTarget::Shared> getTouchTargetChildren() override {
    return {};
  }

  facebook::react::LayoutMetrics getLayoutMetrics() const override {
    return {};
  }

  facebook::react::Transform getTransform() const override {
    return {};
  }

  facebook::react::Rect getBoundingBox() override {
    return {};
  };

  bool isClippingSubviews() const override {
    return false;
  }

  void markBoundingBoxAsDirty() override{};

 private:
  facebook::react::Tag m_tag;
  facebook::react::SharedTouchEventEmitter m_touchEventEmitter;
  std::vector<facebook::react::Rect> m_rects{};
  TouchTarget::Weak m_parentTouchTarget;
};

std::vector<TouchTarget::Shared>
TextComponentInstance::getTouchTargetChildren() {
  if (m_state == nullptr) {
    m_fragmentTouchTargetByTag.clear();
    return {};
  }

  if (m_touchTargetChildrenNeedUpdate) {
    m_touchTargetChildrenNeedUpdate = false;
    updateFragmentTouchTargets(m_state->getData());
  }

  auto const& fragments = m_state->getData().attributedString.getFragments();

  auto attachmentCount = 0;
  std::vector<TouchTarget::Shared> result;
  for (auto const& fragment : fragments) {
    if (!fragment.isAttachment()) {
      result.push_back(
          m_fragmentTouchTargetByTag.at(fragment.parentShadowView.tag));
    }
  }
  result.insert(result.end(), m_children.begin(), m_children.end());
  return result;
}

void TextComponentInstance::updateFragmentTouchTargets(
    facebook::react::ParagraphState const& newState) {
  auto const& fragments = newState.attributedString.getFragments();
  if (!m_textStorage || fragments.empty()) {
    m_fragmentTouchTargetByTag.clear();
    return;
  }

  auto const& typography = m_textStorage->arkUITypography;
  auto rects = typography.getRectsForFragments();

  FragmentTouchTargetByTag touchTargetByTag;
  size_t textFragmentCount = 0;
  for (auto const& fragment : fragments) {
    if (fragment.isAttachment()) {
      continue;
    }
    auto index = textFragmentCount;
    auto eventEmitter =
        std::dynamic_pointer_cast<const facebook::react::TouchEventEmitter>(
            fragment.parentShadowView.eventEmitter);
    auto tag = fragment.parentShadowView.tag;
    auto touchTargetEntry = touchTargetByTag.find(tag);

    if (touchTargetEntry == touchTargetByTag.end()) {
      // if no entry has been inserted yet into the new map
      if (auto prevTouchTarget = m_fragmentTouchTargetByTag.find(tag);
          prevTouchTarget != m_fragmentTouchTargetByTag.end()) {
        // either reuse the existing touch target if there was a fragment with
        // the same tag
        auto fragmentTouchTarget =
            std::static_pointer_cast<TextFragmentTouchTarget>(
                prevTouchTarget->second);
        fragmentTouchTarget->clearRects();
        touchTargetEntry =
            touchTargetByTag.try_emplace(tag, std::move(fragmentTouchTarget))
                .first;
      } else {
        // or create a new one for a new fragment tag
        auto newTouchTarget = std::make_shared<TextFragmentTouchTarget>(
            tag, this->shared_from_this(), eventEmitter);
        touchTargetEntry =
            touchTargetByTag.try_emplace(tag, std::move(newTouchTarget)).first;
      }
    }

    auto fragmentTouchTarget =
        std::static_pointer_cast<TextFragmentTouchTarget>(
            touchTargetEntry->second);
    fragmentTouchTarget->insertRects(rects.at(index));
    textFragmentCount++;
  }
  m_fragmentTouchTargetByTag = std::move(touchTargetByTag);
}

void TextComponentInstance::disposeTextStorage() {
  m_textNode.resetTextContentWithStyledString();
  m_textStorage = nullptr;
}
} // namespace rnoh
