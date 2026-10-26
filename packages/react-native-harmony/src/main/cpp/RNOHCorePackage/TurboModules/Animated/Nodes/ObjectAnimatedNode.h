/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "RNOHCorePackage/TurboModules/Animated/AnimatedNodesManager.h"
#include "ValueAnimatedNode.h"

namespace rnoh {

class ObjectAnimatedNode : public ValueAnimatedNode {
  AnimatedNodesManager& m_nodesManager;
  folly::dynamic m_input;
  folly::dynamic m_output;

 public:
  ObjectAnimatedNode(
      folly::dynamic const& config,
      AnimatedNodesManager& nodesManager);

  folly::dynamic getOutput() override;

  void update() override;

 private:
  /**
   * @param dyn example: [{"translateX":{"nodeTag":5}}]
   */
  folly::dynamic replaceAnimatedValueNodeTagsWithValues(
      const folly::dynamic& dyn);
};

} // namespace rnoh
