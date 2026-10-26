/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ObjectAnimatedNode.h"

namespace rnoh {

ObjectAnimatedNode::ObjectAnimatedNode(
    folly::dynamic const& config,
    AnimatedNodesManager& nodesManager)
    : m_nodesManager(nodesManager) {
  RNOH_ASSERT(config.count("value") > 0);
  m_input = config["value"];
  m_output = this->replaceAnimatedValueNodeTagsWithValues(m_input);
}

folly::dynamic ObjectAnimatedNode::getOutput() {
  return m_output;
}

void ObjectAnimatedNode::update() {
  m_output = this->replaceAnimatedValueNodeTagsWithValues(m_input);
}

folly::dynamic ObjectAnimatedNode::replaceAnimatedValueNodeTagsWithValues(
    const folly::dynamic& dyn) {
  if (dyn.isObject()) {
    if (dyn.count("nodeTag") > 0) {
      RNOH_ASSERT(dyn["nodeTag"].isNumber());
      auto nodeTag = dyn["nodeTag"].asInt();
      auto& node = m_nodesManager.getNodeByTag(nodeTag);
      return node.getOutput();
    } else {
      folly::dynamic result = folly::dynamic::object();
      for (const auto& keyAndValue : dyn.items()) {
        auto key = keyAndValue.first.asString();
        auto value = keyAndValue.second;
        result[key] = this->replaceAnimatedValueNodeTagsWithValues(value);
      }
      return result;
    }
  } else if (dyn.isArray()) {
    folly::dynamic result = folly::dynamic::array();
    for (auto& item : dyn) {
      result.push_back(this->replaceAnimatedValueNodeTagsWithValues(item));
    }
    return result;
  } else {
    return dyn;
  }
}
} // namespace rnoh