/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/image/ImageShadowNode.h>
#include <react/renderer/components/rncore/ShadowNodes.h>
#include <react/renderer/components/root/RootShadowNode.h>
#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>
#include <react/renderer/components/text/ParagraphShadowNode.h>
#include <react/renderer/components/text/RawTextShadowNode.h>
#include <react/renderer/components/text/TextShadowNode.h>
#include <react/renderer/components/textinput/TextInputShadowNode.h>
#include <react/renderer/components/view/ViewShadowNode.h>
#include <mutex>
#include <string>
#include <unordered_set>

namespace rnoh {

const std::unordered_set<facebook::react::ComponentHandle>
    parallelComponentHandles = {
        facebook::react::ParagraphShadowNode::Handle(),
        facebook::react::ImageShadowNode::Handle(),
        facebook::react::ScrollViewShadowNode::Handle(),
        facebook::react::ViewShadowNode::Handle(),
        facebook::react::TextShadowNode::Handle(),
        facebook::react::RawTextShadowNode::Handle(),
        facebook::react::RootShadowNode::Handle(),
        facebook::react::SwitchShadowNode::Handle(),
        facebook::react::PullToRefreshViewShadowNode::Handle(),
        facebook::react::ActivityIndicatorViewShadowNode::Handle(),
};

class ComponentNameManager {
 public:
  static ComponentNameManager& getInstance();

  void addComponentName(const std::string& componentName);
  bool hasComponentName(const std::string& componentName) const;

 private:
  ComponentNameManager() = default;
  ~ComponentNameManager() = default;

  ComponentNameManager(const ComponentNameManager&) = delete;
  ComponentNameManager& operator=(const ComponentNameManager&) = delete;

  // parallelComponentNames is read from FFRT worker threads (hasComponentName
  // in the parallel preallocation/creation path) and written from other threads
  // (addComponentName). Guard it with a mutex to avoid a data race on the
  // unordered_set, which would be undefined behavior.
  std::unordered_set<std::string> parallelComponentNames = {};
  mutable std::mutex m_mutex;
};

} // namespace rnoh
