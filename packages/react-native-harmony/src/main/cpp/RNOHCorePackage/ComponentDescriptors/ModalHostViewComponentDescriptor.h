/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/modal/ModalHostViewShadowNode.h>
#include <react/renderer/core/ConcreteComponentDescriptor.h>
#include "RNOH/RNInstance.h"

namespace rnoh {

/*
 * RNOH uses a custom ModalHostViewComponentDescriptor so that we can set the
 * initial state to the actual size of the screen and not the default {0,0}.
 * This fixes a bug in KeyboardAvoidingView which relies on the height value
 * from the initial onLayout.
 */

class ModalHostViewComponentDescriptor final
    : public facebook::react::ConcreteComponentDescriptor<
          facebook::react::ModalHostViewShadowNode> {
 public:
  using ConcreteComponentDescriptor::ConcreteComponentDescriptor;

  void adopt(facebook::react::ShadowNode& shadowNode) const override {
    auto& layoutableShadowNode =
        static_cast<facebook::react::YogaLayoutableShadowNode&>(shadowNode);
    auto& stateData =
        static_cast<
            const facebook::react::ModalHostViewShadowNode::ConcreteState&>(
            *shadowNode.getState())
            .getData();

    layoutableShadowNode.setSize(facebook::react::Size{
        stateData.screenSize.width, stateData.screenSize.height});
    layoutableShadowNode.setPositionType(YGPositionTypeAbsolute);

    ConcreteComponentDescriptor::adopt(shadowNode);
  }

  facebook::react::State::Shared createInitialState(
      const facebook::react::Props::Shared& props,
      const facebook::react::ShadowNodeFamily::Shared& family) const override;
};

} // namespace rnoh
