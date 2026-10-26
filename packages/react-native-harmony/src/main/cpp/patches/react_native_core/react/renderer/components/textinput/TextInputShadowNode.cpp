/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextInputShadowNode.h"

#include <react/debug/react_native_assert.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/attributedstring/AttributedStringBox.h>
#include <react/renderer/attributedstring/TextAttributes.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/core/LayoutContext.h>
#include <react/renderer/core/conversions.h>
#include <react/renderer/textlayoutmanager/TextLayoutContext.h>
#include <react/renderer/textlayoutmanager/TextLayoutManagerExtended.h>

namespace facebook::react {

extern const char TextInputComponentName[] = "TextInput";
    
    

AttributedStringBox TextInputShadowNode::attributedStringBoxToMeasure(
    const LayoutContext& layoutContext) const {
  bool hasMeaningfulState =
      getState() && getState()->getRevision() != State::initialRevisionValue;

  if (hasMeaningfulState) {
    const auto& attributedStringBox = getStateData().attributedStringBox;
    if (attributedStringBox.getMode() ==
            AttributedStringBox::Mode::OpaquePointer ||
        !attributedStringBox.getValue().isEmpty()) {
      return getStateData().attributedStringBox;
    }
  }

  auto attributedString = hasMeaningfulState
      ? AttributedString{}
      : getAttributedString(layoutContext);

  if (attributedString.isEmpty()) {
    attributedString = getPlaceholderAttributedString(layoutContext);
  }

  return AttributedStringBox{attributedString};
}
    
AttributedString TextInputShadowNode::getPlaceholderAttributedString(
    const LayoutContext& layoutContext) const {
  const auto& props = getConcreteProps();

  AttributedString attributedString;
  attributedString.setBaseTextAttributes(
      props.getEffectiveTextAttributes(layoutContext.fontSizeMultiplier));

  if (!props.placeholder.empty()) {
    attributedString.appendFragment(
        {.string = props.placeholder,
          .textAttributes = attributedString.getBaseTextAttributes(),
          .parentShadowView = {}});
  }
  return attributedString;
}

AttributedString TextInputShadowNode::getAttributedString(
    const LayoutContext& layoutContext) const {
  auto textAttributes = getConcreteProps().getEffectiveTextAttributes(
      layoutContext.fontSizeMultiplier);
  auto attributedString = AttributedString{};

  attributedString.appendFragment(AttributedString::Fragment{
      .string = getConcreteProps().text,
      .textAttributes = textAttributes,
      // TODO: Is this really meant to be by value?
      .parentShadowView = ShadowView(*this)});

  auto attachments = Attachments{};
  BaseTextShadowNode::buildAttributedString(
      textAttributes, *this, attributedString, attachments);
  attributedString.setBaseTextAttributes(textAttributes);

  return attributedString;
}

void TextInputShadowNode::setTextLayoutManager(
    std::shared_ptr<const TextLayoutManager> textLayoutManager) {
  ensureUnsealed();
  textLayoutManager_ = std::move(textLayoutManager);
}

void TextInputShadowNode::updateStateIfNeeded(
    const LayoutContext& layoutContext) {
  ensureUnsealed();

  auto reactTreeAttributedString = getAttributedString(layoutContext);
  const auto& state = getStateData();

  react_native_assert(textLayoutManager_);
  react_native_assert(
      (!state.layoutManager || state.layoutManager == textLayoutManager_) &&
      "`StateData` refers to a different `TextLayoutManager`");

  if (state.reactTreeAttributedString.isContentEqual(
          reactTreeAttributedString) &&
      state.layoutManager == textLayoutManager_) {
    return;
  }

  auto newState = TextInputState{};
  newState.attributedStringBox = AttributedStringBox{reactTreeAttributedString};
  newState.paragraphAttributes = getConcreteProps().paragraphAttributes;
  newState.reactTreeAttributedString = reactTreeAttributedString;
  newState.layoutManager = textLayoutManager_;
  newState.mostRecentEventCount = getConcreteProps().mostRecentEventCount;
  setStateData(std::move(newState));
}

#pragma mark - LayoutableShadowNode

Size TextInputShadowNode::measureContent(
    const LayoutContext& layoutContext,
    const LayoutConstraints& layoutConstraints) const {
  TextLayoutContext textLayoutContext{};
  textLayoutContext.pointScaleFactor = layoutContext.pointScaleFactor;
  return textLayoutManager_
      ->measure(
          attributedStringBoxToMeasure(layoutContext),
          getConcreteProps().getEffectiveParagraphAttributes(),
          textLayoutContext,
          layoutConstraints)
      .size;
}

Float TextInputShadowNode::baseline(
    const LayoutContext& layoutContext,
    Size size) const {
  auto attributedString = getAttributedString(layoutContext);

  if (attributedString.isEmpty()) {
    attributedString = getPlaceholderAttributedString(layoutContext);
  }

  // Yoga expects a baseline relative to the Node's border-box edge instead of
  // the content, so we need to adjust by the padding and border widths, which
  // have already been set by the time of baseline alignment
  auto top = YGNodeLayoutGetBorder(&yogaNode_, YGEdgeTop) +
      YGNodeLayoutGetPadding(&yogaNode_, YGEdgeTop);

  AttributedStringBox attributedStringBox{std::move(attributedString)};
  auto lines =
      TextLayoutManagerExtended(*textLayoutManager_)
          .measureLines(
              attributedStringBox, getConcreteProps().getEffectiveParagraphAttributes(), size);
  return LineMeasurement::baseline(lines) + top;
}

void TextInputShadowNode::layout(LayoutContext layoutContext) {
  updateStateIfNeeded(layoutContext);
  ConcreteViewShadowNode::layout(layoutContext);
}

} // namespace facebook::react
