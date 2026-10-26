/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RootViewComponentInstance.h"
#include "conversions.h"

namespace rnoh {
RootViewComponentInstance::RootViewComponentInstance(Context context)
    : CppComponentInstance(std::move(context)), m_stackNode(m_arkUINodeCtx) {
  m_stackNode.setStackNodeDelegate(this);
}

void RootViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_stackNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void RootViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_stackNode.removeChild(childComponentInstance->getLocalRootArkUINode());
};

void RootViewComponentInstance::onHoverIn() {
  m_eventEmitter->dispatchEvent(
      "pointerEnter", [=](facebook::jsi::Runtime& runtime) {
        auto payload = facebook::jsi::Object(runtime);
        return payload;
      },
      facebook::react::RawEvent::Category::Continuous);
}

void RootViewComponentInstance::onHoverOut() {
  m_eventEmitter->dispatchEvent(
      "pointerLeave", [=](facebook::jsi::Runtime& runtime) {
        auto payload = facebook::jsi::Object(runtime);
        return payload;
      },
      facebook::react::RawEvent::Category::Continuous);
}

void RootViewComponentInstance::onClick() {
  if (m_eventEmitter != nullptr) {
    m_eventEmitter->dispatchEvent(
        "click", [=](facebook::jsi::Runtime& runtime) {
          auto payload = facebook::jsi::Object(runtime);
          return payload;
        });
  }
}

StackNode& RootViewComponentInstance::getLocalRootArkUINode() {
  return m_stackNode;
}

void RootViewComponentInstance::onLayoutChanged(
    const facebook::react::LayoutMetrics& layoutMetrics) {
  CppComponentInstance::onLayoutChanged(layoutMetrics);
  folly::dynamic payload = folly::dynamic::object("surfaceId", this->getTag())(
      "width", layoutMetrics.frame.size.width)(
      "height", layoutMetrics.frame.size.height);
  m_deps->arkTSChannel->postMessage(
      "RNOH::ROOT_COMPONENT_DIMENSIONS_CHANGED", payload);
}
} // namespace rnoh
