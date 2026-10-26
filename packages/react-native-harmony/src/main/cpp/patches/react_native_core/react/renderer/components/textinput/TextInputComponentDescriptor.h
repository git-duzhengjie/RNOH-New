/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/textinput/TextInputShadowNode.h> // RNOH patch
#include <react/renderer/core/ConcreteComponentDescriptor.h>

namespace facebook::react {

// G.ARR.07: Array size must be specified for external link arrays.
// The size 18 accounts for "TextLayoutManager" (17 chars) + null terminator (1
// char).
extern const char TextLayoutManagerKey[18];

/*
 * Descriptor for <TextInput> component.
 */
class TextInputComponentDescriptor final
    : public ConcreteComponentDescriptor<TextInputShadowNode> {
 public:
  TextInputComponentDescriptor(const ComponentDescriptorParameters& parameters)
      : ConcreteComponentDescriptor<TextInputShadowNode>(parameters),
        textLayoutManager_(
            getManagerByName<TextLayoutManager>(
                contextContainer_,
                TextLayoutManagerKey)) {}

 protected:
  void adopt(ShadowNode& shadowNode) const override {
    ConcreteComponentDescriptor::adopt(shadowNode);

    auto& concreteShadowNode = static_cast<TextInputShadowNode&>(shadowNode);
    concreteShadowNode.setTextLayoutManager(textLayoutManager_);
  }

 private:
  const std::shared_ptr<const TextLayoutManager> textLayoutManager_;
};

} // namespace facebook::react
