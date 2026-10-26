/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::ModulusAnimatedNode` — the node that
 * wraps an input value into the range `[0, modulus)` using a double
 * `fmod()` (with a correction step that lifts negative results back
 * into the positive range).
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * a parent value node with it. The modulus node is constructed directly
 * so the test can call `update()` and observe the result.
 *
 * Tests exercise:
 *   - Positive value within `[0, modulus)` stays unchanged
 *   - Value equal to the modulus wraps to 0
 *   - Value above the modulus wraps (e.g. 7 % 5 = 2)
 *   - Negative value wraps into the positive range (e.g. -1 % 5 = 4)
 *   - Negative value with fractional modulus (e.g. -0.5 % 1.0 = 0.5)
 *   - Zero input → zero output
 *   - Large input value (well above modulus) wraps correctly
 *   - Small fractional modulus (e.g. 0.1)
 *   - Constructor extracts `input` and `modulus` fields from config
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ModulusAnimatedNode.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;
using rnoh::test::makeAnimatedNodesManager;

namespace {

folly::dynamic makeValueNodeConfig(double value, double offset = 0.0) {
  return folly::dynamic::object("type", "value")("value", value)(
      "offset", offset);
}

void registerParentValue(
    AnimatedNodesManager& manager,
    facebook::react::Tag parentTag,
    double value) {
  manager.createNode(parentTag, makeValueNodeConfig(value, 0.0));
}

folly::dynamic makeModulusConfig(
    facebook::react::Tag inputTag,
    double modulus) {
  // Production code reads `config["input"]` via `getDouble()` (the JS
  // side serialises the tag as a floating-point number); mirror that
  // here by casting the integer tag to double when constructing the
  // config payload.
  return folly::dynamic::object("input", static_cast<double>(inputTag))(
      "modulus", modulus);
}
} // namespace

TEST(ModulusAnimatedNode, Update_PositiveValueBelowModulus_StaysUnchanged) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 3.0);

  ModulusAnimatedNode node(makeModulusConfig(1, 10.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 3.0, 1e-9);
}

TEST(ModulusAnimatedNode, Update_ValueEqualToModulus_WrapsToZero) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 10.0);

  ModulusAnimatedNode node(makeModulusConfig(1, 10.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

TEST(ModulusAnimatedNode, Update_ValueAboveModulus_WrapsToRemainder) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 7.0);

  // 7 mod 5 = 2
  ModulusAnimatedNode node(makeModulusConfig(1, 5.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 2.0, 1e-9);
}

TEST(ModulusAnimatedNode, Update_NegativeValue_WrapsIntoPositiveRange) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, -1.0);

  // fmod(-1, 5) = -1; production adds modulus: (-1 + 5) = 4; fmod(4, 5) = 4
  ModulusAnimatedNode node(makeModulusConfig(1, 5.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 4.0, 1e-9);
}

TEST(
    ModulusAnimatedNode,
    Update_NegativeValueWithFractionalModulus_WrapsToPositive) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, -0.5);

  // fmod(-0.5, 1.0) = -0.5; (-0.5 + 1.0) = 0.5; fmod(0.5, 1.0) = 0.5
  ModulusAnimatedNode node(makeModulusConfig(1, 1.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 0.5, 1e-9);
}

TEST(ModulusAnimatedNode, Update_ZeroValue_WrapsToZero) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  ModulusAnimatedNode node(makeModulusConfig(1, 7.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

TEST(ModulusAnimatedNode, Update_LargeValue_WrapsCorrectly) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 1003.0);

  // 1003 mod 5 = 3
  ModulusAnimatedNode node(makeModulusConfig(1, 5.0), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 3.0, 1e-9);
}

TEST(ModulusAnimatedNode, Update_SmallFractionalModulus_WrapsCorrectly) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.27);

  // 0.27 mod 0.1 = 0.07 (with floating-point tolerance)
  ModulusAnimatedNode node(makeModulusConfig(1, 0.1), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 0.07, 1e-6);
}

TEST(ModulusAnimatedNode, Update_AfterParentValueChanges_RecalculatesEachTime) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 1.0);

  ModulusAnimatedNode node(makeModulusConfig(1, 3.0), *manager);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 1.0, 1e-9);

  // Update parent to a value above the modulus.
  manager->setValue(1, 5.0);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 2.0, 1e-9);

  // And back below the modulus.
  manager->setValue(1, 0.5);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 0.5, 1e-9);
}

TEST(ModulusAnimatedNode, Constructor_ExtractsInputTagAndModulusFromConfig) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 42, 0.0);

  // Constructor must use the tag from config (42) — verified by the fact
  // that update() finds the parent node. A wrong tag would throw via
  // getValueNodeByTag's AnimatedNodeNotFoundError.
  ModulusAnimatedNode node(makeModulusConfig(42, 3.0), *manager);
  EXPECT_NO_THROW({ node.update(); });
}
