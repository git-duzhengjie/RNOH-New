/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::AssociativeOperationNode<OperatorT>` —
 * the templated node that combines N input value nodes with a binary
 * operator. The four production type aliases are tested here:
 *
 *   - `AdditionAnimatedNode`         (`std::plus<double>`)
 *   - `SubtractionAnimatedNode`      (`std::minus<double>`)
 *   - `MultiplicationAnimatedNode`   (`std::multiplies<double>`)
 *   - `DivisionAnimatedNode`         (`SafeDivides`)
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * N parent value nodes with it. The operation node is constructed
 * directly so the test can call `update()` and observe the result.
 *
 * Tests exercise:
 *   - Constructor parses the `input` array as a list of node tags
 *   - Addition: 2 + 3 = 5, single input is identity, three-input chain
 *   - Subtraction: 10 - 3 = 7, three-input chain (10 - 3 - 2 = 5)
 *   - Multiplication: 2 * 3 = 6, three-input chain (2 * 3 * 4 = 24)
 *   - Division: 10 / 2 = 5, three-input chain (24 / 2 / 3 = 4)
 *   - Division by zero throws `std::runtime_error`
 *   - Update after parent value changes recalculates
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/AssociativeOperationNode.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;
using rnoh::test::makeAnimatedNodesManager;

namespace {

folly::dynamic makeValueNodeConfig(double value, double offset = 0.0) {
  return folly::dynamic::object("type", "value")("value", value)(
      "offset", offset);
}

void registerValue(
    AnimatedNodesManager& manager,
    facebook::react::Tag tag,
    double value) {
  manager.createNode(tag, makeValueNodeConfig(value, 0.0));
}

folly::dynamic makeOpConfig(std::vector<facebook::react::Tag> tags) {
  folly::dynamic arr = folly::dynamic::array;
  for (auto t : tags) {
    // Production code reads `config["input"]` entries via `asDouble()`
    // (JS-side serialisation uses floating-point numbers for tags).
    arr.push_back(static_cast<double>(t));
  }
  return folly::dynamic::object("input", arr);
}
} // namespace

// ---------------------------------------------------------------------------
// AdditionAnimatedNode
// ---------------------------------------------------------------------------

TEST(AdditionAnimatedNode, Update_TwoInputs_ReturnsSum) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 2.0);
  registerValue(*manager, 2, 3.0);

  AdditionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);
}

TEST(AdditionAnimatedNode, Update_SingleInput_ReturnsThatInputValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 42.0);

  // The production loop seeds `value` with the first input's output, so a
  // single-input addition node is effectively an identity.
  AdditionAnimatedNode node(makeOpConfig({1}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 42.0, 1e-9);
}

TEST(AdditionAnimatedNode, Update_ThreeInputs_ReturnsSumOfAll) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 1.0);
  registerValue(*manager, 2, 2.0);
  registerValue(*manager, 3, 3.0);

  AdditionAnimatedNode node(makeOpConfig({1, 2, 3}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 6.0, 1e-9);
}

TEST(AdditionAnimatedNode, Update_AfterParentValueChange_Recalculates) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 10.0);
  registerValue(*manager, 2, 20.0);

  AdditionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();
  ASSERT_NEAR(node.getOutputAsDouble(), 30.0, 1e-9);

  manager->setValue(1, 15.0);
  node.update();
  EXPECT_NEAR(node.getOutputAsDouble(), 35.0, 1e-9);
}

TEST(AdditionAnimatedNode, Update_NegativeInputs_SumsSignedValues) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, -5.0);
  registerValue(*manager, 2, 3.0);

  AdditionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), -2.0, 1e-9);
}

// ---------------------------------------------------------------------------
// SubtractionAnimatedNode
// ---------------------------------------------------------------------------

TEST(SubtractionAnimatedNode, Update_TwoInputs_ReturnsDifference) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 10.0);
  registerValue(*manager, 2, 3.0);

  SubtractionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 7.0, 1e-9);
}

TEST(SubtractionAnimatedNode, Update_ThreeInputs_ChainsLeftToRight) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 10.0);
  registerValue(*manager, 2, 3.0);
  registerValue(*manager, 3, 2.0);

  // 10 - 3 - 2 = 5
  SubtractionAnimatedNode node(makeOpConfig({1, 2, 3}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);
}

TEST(SubtractionAnimatedNode, Update_NegativeResult_ReturnsSignedValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 3.0);
  registerValue(*manager, 2, 10.0);

  SubtractionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), -7.0, 1e-9);
}

// ---------------------------------------------------------------------------
// MultiplicationAnimatedNode
// ---------------------------------------------------------------------------

TEST(MultiplicationAnimatedNode, Update_TwoInputs_ReturnsProduct) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 2.0);
  registerValue(*manager, 2, 3.0);

  MultiplicationAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 6.0, 1e-9);
}

TEST(MultiplicationAnimatedNode, Update_ThreeInputs_ReturnsProductOfAll) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 2.0);
  registerValue(*manager, 2, 3.0);
  registerValue(*manager, 3, 4.0);

  // 2 * 3 * 4 = 24
  MultiplicationAnimatedNode node(makeOpConfig({1, 2, 3}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 24.0, 1e-9);
}

TEST(MultiplicationAnimatedNode, Update_ZeroInput_ReturnsZero) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);
  registerValue(*manager, 2, 99.0);

  MultiplicationAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 0.0, 1e-9);
}

// ---------------------------------------------------------------------------
// DivisionAnimatedNode (SafeDivides)
// ---------------------------------------------------------------------------

TEST(DivisionAnimatedNode, Update_TwoInputs_ReturnsQuotient) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 10.0);
  registerValue(*manager, 2, 2.0);

  DivisionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 5.0, 1e-9);
}

TEST(DivisionAnimatedNode, Update_ThreeInputs_ChainsLeftToRight) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 24.0);
  registerValue(*manager, 2, 2.0);
  registerValue(*manager, 3, 3.0);

  // 24 / 2 / 3 = 4
  DivisionAnimatedNode node(makeOpConfig({1, 2, 3}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 4.0, 1e-9);
}

TEST(DivisionAnimatedNode, Update_DivisionByZero_ThrowsRuntimeError) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 10.0);
  registerValue(*manager, 2, 0.0);

  DivisionAnimatedNode node(makeOpConfig({1, 2}), *manager);

  // SafeDivides throws std::runtime_error when divisor is 0.
  EXPECT_THROW({ node.update(); }, std::runtime_error);
}

TEST(
    DivisionAnimatedNode,
    Update_DivisionByZeroInSecondStep_ThrowsRuntimeError) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 24.0);
  registerValue(*manager, 2, 2.0);
  registerValue(*manager, 3, 0.0);

  DivisionAnimatedNode node(makeOpConfig({1, 2, 3}), *manager);

  EXPECT_THROW({ node.update(); }, std::runtime_error);
}

TEST(DivisionAnimatedNode, Update_FractionalResult_ReturnsExactQuotient) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 1.0);
  registerValue(*manager, 2, 3.0);

  DivisionAnimatedNode node(makeOpConfig({1, 2}), *manager);
  node.update();

  EXPECT_NEAR(node.getOutputAsDouble(), 1.0 / 3.0, 1e-9);
}
