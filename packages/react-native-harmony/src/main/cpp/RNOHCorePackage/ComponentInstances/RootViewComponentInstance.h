/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <react/renderer/components/root/RootShadowNode.h>
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/StackNode.h"

namespace rnoh {
class RootViewComponentInstance
    : public CppComponentInstance<facebook::react::RootShadowNode>,
      public StackNodeDelegate {
 private:
  StackNode m_stackNode;

 public:
  RootViewComponentInstance(Context context);

  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;

  void onLayoutChanged(
      const facebook::react::LayoutMetrics& layoutMetrics) override;

  void onClick() override;
  void onHoverIn() override;
  void onHoverOut() override;
  StackNode& getLocalRootArkUINode() override;

  bool canHandleTouch() const override {
    return false;
  }

  std::weak_ptr<UIInputEventHandler> getUIInputEventHandler() override {
    auto rnInstance = m_deps->rnInstance.lock();
    if (!rnInstance) {
      return {};
    }
    auto maybeWeakSurface = rnInstance->getSurfaceByRootTag(m_tag);
    if (!maybeWeakSurface.has_value()) {
      return {};
    }
    auto surface = maybeWeakSurface.value().lock();
    if (surface == nullptr) {
      return {};
    }
    return surface->getUIInputEventHandler();
  }
};
} // namespace rnoh
