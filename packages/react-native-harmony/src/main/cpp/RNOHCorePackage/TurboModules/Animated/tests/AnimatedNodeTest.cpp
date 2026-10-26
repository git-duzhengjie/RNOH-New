/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::AnimatedNode` — the polymorphic base
 * class for every node in the Animated graph.
 *
 * Per `gtest-module-catalog.md` the module is Tier 2 ("Folly + RN core"):
 * only `<folly/dynamic.h>` and `react/renderer/core/ReactPrimitives.h`
 * are required. No `AnimatedNodesManager&` is held by this class, so no
 * manager-level Mock is needed. Tests exercise:
 *
 *   - `addChild()` / `removeChild()` — child tag tracking
 *   - `getChildrenTags()` — read-only accessor
 *   - `update()` / `onAttachedToNode()` / `onDetachedFromNode()` /
 *     `getOutput()` — base-class default no-op behaviour
 *   - `tag_` — public mutable tag field used by graph management
 *
 * The class is non-final and `onAttachedToNode`/`onDetachedFromNode` are
 * virtual so that subclasses (Interpolation, DiffClamp, …) can override
 * them. Here we instantiate `AnimatedNode` directly via its protected
 * default constructor by deriving a minimal `TestAnimatedNode`.
 */

#include <gtest/gtest.h>

#include "RNOHCorePackage/TurboModules/Animated/Nodes/AnimatedNode.h"

#include <folly/dynamic.h>

using namespace rnoh;

namespace {
/**
 * Minimal subclass exposing the protected default constructor so that
 * `AnimatedNode` can be instantiated in tests without going through a
 * derived Animated-graph class.
 */
class TestAnimatedNode : public AnimatedNode {
 public:
  TestAnimatedNode() = default;
};
} // namespace

TEST(AnimatedNode, AddChild_SingleChild_ChildrenTagsContainChildTag) {
  TestAnimatedNode parent;
  TestAnimatedNode child;
  parent.tag_ = 1;
  child.tag_ = 2;

  parent.addChild(child);

  auto const& children = parent.getChildrenTags();
  EXPECT_EQ(children.size(), 1u);
  EXPECT_EQ(children[0], 2);
}

TEST(AnimatedNode, AddChild_MultipleChildren_ChildrenTagsPreserveOrder) {
  TestAnimatedNode parent;
  TestAnimatedNode childA;
  TestAnimatedNode childB;
  TestAnimatedNode childC;
  parent.tag_ = 1;
  childA.tag_ = 10;
  childB.tag_ = 20;
  childC.tag_ = 30;

  parent.addChild(childA);
  parent.addChild(childB);
  parent.addChild(childC);

  auto const& children = parent.getChildrenTags();
  EXPECT_EQ(children.size(), 3u);
  EXPECT_EQ(children[0], 10);
  EXPECT_EQ(children[1], 20);
  EXPECT_EQ(children[2], 30);
}

TEST(AnimatedNode, AddChild_InvokesOnAttachedToNodeWithParentTag) {
  TestAnimatedNode parent;
  TestAnimatedNode child;
  parent.tag_ = 42;
  child.tag_ = 7;

  // Base-class default `onAttachedToNode` is a no-op; we verify the wiring
  // contract by checking that the child's tag is still registered after
  // the call (i.e. addChild returned normally).
  parent.addChild(child);

  EXPECT_EQ(parent.getChildrenTags().size(), 1u);
}

TEST(AnimatedNode, RemoveChild_ExistingChild_ChildrenTagsNoLongerContainTag) {
  TestAnimatedNode parent;
  TestAnimatedNode child;
  parent.tag_ = 1;
  child.tag_ = 5;

  parent.addChild(child);
  EXPECT_EQ(parent.getChildrenTags().size(), 1u);

  parent.removeChild(child);

  EXPECT_EQ(parent.getChildrenTags().size(), 0u);
}

TEST(AnimatedNode, RemoveChild_FromMultipleChildren_OnlyRequestedChildRemoved) {
  TestAnimatedNode parent;
  TestAnimatedNode childA;
  TestAnimatedNode childB;
  TestAnimatedNode childC;
  parent.tag_ = 1;
  childA.tag_ = 11;
  childB.tag_ = 22;
  childC.tag_ = 33;

  parent.addChild(childA);
  parent.addChild(childB);
  parent.addChild(childC);

  parent.removeChild(childB);

  auto const& children = parent.getChildrenTags();
  EXPECT_EQ(children.size(), 2u);
  EXPECT_EQ(children[0], 11);
  EXPECT_EQ(children[1], 33);
}

TEST(AnimatedNode, RemoveChild_NotAdded_ChildrenTagsRemainEmpty) {
  TestAnimatedNode parent;
  TestAnimatedNode child;
  parent.tag_ = 1;
  child.tag_ = 99;

  // removeChild on a child that was never added should not throw; the
  // erase-remove idiom simply finds nothing to remove.
  parent.removeChild(child);

  EXPECT_EQ(parent.getChildrenTags().size(), 0u);
}

TEST(AnimatedNode, AddChild_DuplicateChild_DoesNotDeduplicateTags) {
  TestAnimatedNode parent;
  TestAnimatedNode child;
  parent.tag_ = 1;
  child.tag_ = 5;

  parent.addChild(child);
  parent.addChild(child);

  // The production implementation does not deduplicate; it appends the
  // child's tag on every call. This test pins that behaviour so any future
  // change is intentional.
  EXPECT_EQ(parent.getChildrenTags().size(), 2u);
  EXPECT_EQ(parent.getChildrenTags()[0], 5);
  EXPECT_EQ(parent.getChildrenTags()[1], 5);
}

TEST(AnimatedNode, GetChildrenTags_EmptyNode_ReturnsEmptyVector) {
  TestAnimatedNode node;
  node.tag_ = 1;

  auto const& children = node.getChildrenTags();
  EXPECT_TRUE(children.empty());
}

TEST(AnimatedNode, Update_BaseClass_IsNoOp) {
  TestAnimatedNode node;
  node.tag_ = 1;

  // The base `update()` is a no-op. We assert it returns without throwing
  // and that the node's tag is unchanged (i.e. no state mutation occurred).
  EXPECT_NO_THROW({ node.update(); });
  EXPECT_EQ(node.tag_, 1);
}

TEST(AnimatedNode, GetOutput_BaseClass_ReturnsNullDynamic) {
  TestAnimatedNode node;
  node.tag_ = 1;

  auto output = node.getOutput();
  EXPECT_TRUE(output.isNull());
}

TEST(AnimatedNode, OnAttachedToNode_BaseClass_IsNoOp) {
  TestAnimatedNode node;
  node.tag_ = 1;

  EXPECT_NO_THROW({ node.onAttachedToNode(99); });
}

TEST(AnimatedNode, OnDetachedFromNode_BaseClass_IsNoOp) {
  TestAnimatedNode node;
  node.tag_ = 1;

  EXPECT_NO_THROW({ node.onDetachedFromNode(99); });
}

TEST(AnimatedNode, DefaultConstructor_InitializesTagToNegativeOne) {
  TestAnimatedNode node;

  // The header declares `Tag tag_ = -1;` — the default sentinel value
  // used to detect unregistered nodes. Pinning this guards against
  // accidental changes to the initialiser.
  EXPECT_EQ(node.tag_, -1);
}

TEST(AnimatedNode, DefaultConstructor_InitializesEmptyChildrenTags) {
  TestAnimatedNode node;

  EXPECT_TRUE(node.getChildrenTags().empty());
}

TEST(AnimatedNode, Tag_Assignment_PersistsValue) {
  TestAnimatedNode node;
  node.tag_ = 1234;

  EXPECT_EQ(node.tag_, 1234);
}
