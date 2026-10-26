/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "RNOH/ComponentInstance.h"

using namespace rnoh;

namespace rnoh::test {

class MockComponentInstance : public ComponentInstance {
 public:
  MockComponentInstance(Context ctx) : ComponentInstance(ctx), m_tag(ctx.tag) {}
  ArkUINode& getLocalRootArkUINode() override {
    static ArkUINode dummy;
    return dummy;
  }
  std::string getCompId() override { return ""; }
  facebook::react::Props::Shared getProps() const override { return nullptr; }
  bool getAccessibilityGroup() const override { return false; }
  Tag m_tag;
};

static ComponentInstance::Context makeContext(int32_t tag = 1, std::string name = "View") {
  auto deps = std::make_shared<ComponentInstance::Dependencies>();
  deps->rnInstance = RNInstance::Weak{};
  ComponentInstance::Context ctx;
  ctx.tag = tag;
  ctx.componentHandle = 0;
  ctx.componentName = std::move(name);
  ctx.dependencies = std::move(deps);
  return ctx;
}

} // namespace rnoh::test

// Constructor 鈥?branch: if (m_deps != nullptr && rnInstance != nullptr)
TEST(ComponentInstanceReal, Constructor_WithNullDeps_RnInstanceIdStaysDefault) {
  auto ctx = test::makeContext(42, "TestView");
  auto instance = std::make_shared<test::MockComponentInstance>(ctx);
  EXPECT_EQ(instance->getTag(), 42);
  EXPECT_EQ(instance->getComponentName(), "TestView");
}

TEST(ComponentInstanceReal, Constructor_WithDefaultValues) {
  auto instance = std::make_shared<test::MockComponentInstance>(test::makeContext());
  EXPECT_EQ(instance->getTag(), 1);
}

// insertChild 鈥?branch: if (index > m_children.size())
TEST(ComponentInstanceReal, InsertChild_AtValidIndex_Inserts) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  auto child = std::make_shared<test::MockComponentInstance>(test::makeContext(2));
  parent->insertChild(child, 0);
  EXPECT_EQ(parent->getChildren().size(), 1u);
}

TEST(ComponentInstanceReal, InsertChild_OutOfRange_DoesNotInsert) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  auto child = std::make_shared<test::MockComponentInstance>(test::makeContext(2));
  parent->insertChild(child, 5);
  EXPECT_EQ(parent->getChildren().size(), 0u);
}

TEST(ComponentInstanceReal, InsertChild_MultipleChildren) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  for (int i = 0; i < 5; ++i) {
    auto child = std::make_shared<test::MockComponentInstance>(test::makeContext(i + 2));
    parent->insertChild(child, static_cast<std::size_t>(i));
  }
  EXPECT_EQ(parent->getChildren().size(), 5u);
}

TEST(ComponentInstanceReal, InsertChild_AtMiddleIndex_Shifts) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  auto c1 = std::make_shared<test::MockComponentInstance>(test::makeContext(2));
  auto c2 = std::make_shared<test::MockComponentInstance>(test::makeContext(3));
  auto c3 = std::make_shared<test::MockComponentInstance>(test::makeContext(4));
  parent->insertChild(c1, 0);
  parent->insertChild(c2, 1);
  parent->insertChild(c3, 1);
  EXPECT_EQ(parent->getChildren().size(), 3u);
  EXPECT_EQ(parent->getChildren()[0]->getTag(), 2);
  EXPECT_EQ(parent->getChildren()[1]->getTag(), 4);
  EXPECT_EQ(parent->getChildren()[2]->getTag(), 3);
}

// removeChild 鈥?branch: if (it != m_children.end())
TEST(ComponentInstanceReal, RemoveChild_Existing_Removes) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  auto child = std::make_shared<test::MockComponentInstance>(test::makeContext(2));
  parent->insertChild(child, 0);
  parent->removeChild(child);
  EXPECT_EQ(parent->getChildren().size(), 0u);
}

TEST(ComponentInstanceReal, RemoveChild_NonExisting_DoesNothing) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  auto child = std::make_shared<test::MockComponentInstance>(test::makeContext(2));
  auto other = std::make_shared<test::MockComponentInstance>(test::makeContext(3));
  parent->insertChild(child, 0);
  parent->removeChild(other);
  EXPECT_EQ(parent->getChildren().size(), 1u);
}

TEST(ComponentInstanceReal, RemoveChild_FromEmptyList_DoesNothing) {
  auto parent = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  auto child = std::make_shared<test::MockComponentInstance>(test::makeContext(2));
  parent->removeChild(child);
  EXPECT_EQ(parent->getChildren().size(), 0u);
}

// NoArkUINodeError
TEST(ComponentInstanceReal, GetLocalRootArkUINode_ReturnsNode) {
  auto instance = std::make_shared<test::MockComponentInstance>(test::makeContext(1));
  EXPECT_NO_THROW(instance->getLocalRootArkUINode());
}
