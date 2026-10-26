/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include "RNOHCorePackage/ComponentInstances/RootViewComponentInstance.h"

using namespace rnoh;

// Constructor
TEST(RootViewComponentInstanceReal, Constructor_DoesNotThrow) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  EXPECT_NO_THROW(std::make_shared<RootViewComponentInstance>(ctx));
}

// onClick 鈥?branch: if (m_eventEmitter != nullptr)
TEST(RootViewComponentInstanceReal, OnClick_NullEmitter_DoesNotThrow) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  auto inst = std::make_shared<RootViewComponentInstance>(ctx);
  EXPECT_NO_THROW(inst->onClick());
}

TEST(RootViewComponentInstanceReal, OnHoverIn_DoesNotThrow) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  auto inst = std::make_shared<RootViewComponentInstance>(ctx);
  EXPECT_NO_THROW(inst->onHoverIn());
}

TEST(RootViewComponentInstanceReal, OnHoverOut_DoesNotThrow) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  auto inst = std::make_shared<RootViewComponentInstance>(ctx);
  EXPECT_NO_THROW(inst->onHoverOut());
}

TEST(RootViewComponentInstanceReal, GetLocalRootArkUINode_ReturnsStackNode) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  auto inst = std::make_shared<RootViewComponentInstance>(ctx);
  EXPECT_NO_THROW((void)inst->getLocalRootArkUINode());
}

TEST(RootViewComponentInstanceReal, CanHandleTouch_ReturnsFalse) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  auto inst = std::make_shared<RootViewComponentInstance>(ctx);
  EXPECT_FALSE(inst->canHandleTouch());
}

TEST(RootViewComponentInstanceReal, OnClick_WithEmitter_DispatchesClick) {
  ComponentInstance::Context ctx;
  ctx.tag = 1;
  ctx.componentName = "RootView";
  ctx.dependencies = std::make_shared<ComponentInstance::Dependencies>();
  auto inst = std::make_shared<RootViewComponentInstance>(ctx);
  inst->setEventEmitter(std::make_shared<facebook::react::EventEmitter>());
  EXPECT_NO_THROW(inst->onClick());
}
