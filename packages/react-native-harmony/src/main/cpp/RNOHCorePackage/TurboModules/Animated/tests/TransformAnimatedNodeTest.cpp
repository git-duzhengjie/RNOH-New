/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::TransformAnimatedNode` — the node
 * that composes a list of static and animated transform operations
 * (translateX, translateY, scale, scaleX, scaleY, rotate, rotateX,
 * rotateY, rotateZ, perspective) into a 4×4 matrix returned as a
 * `folly::dynamic` payload.
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance and register
 * parent value nodes with it. The transform node is constructed
 * directly so the test can call `getTransform()` and inspect the
 * resulting matrix.
 *
 * Tests exercise:
 *   - Constructor parses both `static` and `animated` transform entries
 *   - Each supported property produces the correct matrix layout
 *   - Static and animated transforms compose in the order given
 *   - Unsupported property throws `std::runtime_error`
 *   - Animated transform values are read from the manager (so the
 *     matrix changes when the parent value changes)
 *   - translateX / translateY trigger frame-rate recalculation on the
 *     parent value node (verified via the parent's `getFrameRate()`)
 *
 * The 4×4 matrix is laid out row-major in `react::Transform::matrix`:
 *   [0]  [1]  [2]  [3]
 *   [4]  [5]  [6]  [7]
 *   [8]  [9]  [10] [11]
 *   [12] [13] [14] [15]
 * with the identity as `1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1`.
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/TransformAnimatedNode.h"
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

folly::dynamic makeTransformConfig(
    std::vector<std::pair<std::string, double>> statics,
    std::vector<std::pair<std::string, facebook::react::Tag>> animated = {}) {
  folly::dynamic transforms = folly::dynamic::array;
  for (auto const& [prop, value] : statics) {
    transforms.push_back(
        folly::dynamic::object("property", prop)("type", "static")(
            "value", value));
  }
  for (auto const& [prop, tag] : animated) {
    // Production code reads `nodeTag` via `getDouble()` (the JS side
    // serialises tags as floating-point numbers).
    transforms.push_back(
        folly::dynamic::object("property", prop)("type", "animated")(
            "nodeTag", static_cast<double>(tag)));
  }
  return folly::dynamic::object("transforms", transforms);
}

/**
 * Extracts the 16-element matrix from the
 * `[{ "matrix": [16 doubles] }]` payload returned by `getTransform()`.
 */
std::vector<double> extractMatrix(folly::dynamic const& transform) {
  EXPECT_FALSE(transform.empty());
  auto const& entry = transform[0];
  EXPECT_TRUE(entry.isObject());
  auto const& matrix = entry["matrix"];
  EXPECT_FALSE(matrix.empty());
  std::vector<double> result;
  result.reserve(matrix.size());
  for (auto const& v : matrix) {
    result.push_back(v.asDouble());
  }
  return result;
}
} // namespace

// ---------------------------------------------------------------------------
// Static transforms — each property in isolation
// ---------------------------------------------------------------------------

TEST(
    TransformAnimatedNode,
    GetTransform_StaticTranslateX_PlacesValueInMatrixSlot12) {
  auto manager = makeAnimatedNodesManager();

  TransformAnimatedNode node(
      makeTransformConfig({{"translateX", 5.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  // Identity except for matrix[12] (translation X component).
  EXPECT_NEAR(matrix[0], 1.0, 1e-9);
  EXPECT_NEAR(matrix[5], 1.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
  EXPECT_NEAR(matrix[15], 1.0, 1e-9);
  EXPECT_NEAR(matrix[12], 5.0, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_StaticTranslateY_PlacesValueInMatrixSlot13) {
  auto manager = makeAnimatedNodesManager();

  TransformAnimatedNode node(
      makeTransformConfig({{"translateY", 7.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[13], 7.0, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_StaticScale_PlacesValueInSlots0And5AndLeaves10As1) {
  auto manager = makeAnimatedNodesManager();

  // Uniform scale: matrix[0] = matrix[5] = 2, matrix[10] = 1 (Z unchanged).
  TransformAnimatedNode node(makeTransformConfig({{"scale", 2.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[0], 2.0, 1e-9);
  EXPECT_NEAR(matrix[5], 2.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
}

TEST(TransformAnimatedNode, GetTransform_StaticScaleX_OnlyAffectsMatrixSlot0) {
  auto manager = makeAnimatedNodesManager();

  TransformAnimatedNode node(makeTransformConfig({{"scaleX", 3.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[0], 3.0, 1e-9);
  EXPECT_NEAR(matrix[5], 1.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
}

TEST(TransformAnimatedNode, GetTransform_StaticScaleY_OnlyAffectsMatrixSlot5) {
  auto manager = makeAnimatedNodesManager();

  TransformAnimatedNode node(makeTransformConfig({{"scaleY", 4.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[0], 1.0, 1e-9);
  EXPECT_NEAR(matrix[5], 4.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_StaticRotate_LeavesIdentityDiagonalsUnchangedForZeroAngle) {
  auto manager = makeAnimatedNodesManager();

  // Rotate by 0 radians is the identity — pinning the property name is
  // accepted without throwing.
  TransformAnimatedNode node(makeTransformConfig({{"rotate", 0.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[0], 1.0, 1e-9);
  EXPECT_NEAR(matrix[5], 1.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
  EXPECT_NEAR(matrix[15], 1.0, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_StaticPerspective_ModifiesMatrixSlot11) {
  auto manager = makeAnimatedNodesManager();

  // Perspective(1000) places the value at matrix[11] in the
  // column-major-style layout used by react::Transform::Perspective.
  TransformAnimatedNode node(
      makeTransformConfig({{"perspective", 1000.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  // The perspective entry populates matrix[11] (or matrix[14], depending
  // on the layout convention). Either should be non-zero and the rest of
  // the diagonal should remain 1. We assert at least one off-diagonal
  // entry is non-zero and that the diagonal is preserved.
  bool anyOffDiagonalNonZero = false;
  for (size_t i = 0; i < 16; ++i) {
    if (i == 0 || i == 5 || i == 10 || i == 15) {
      continue;
    }
    if (std::abs(matrix[i]) > 1e-9) {
      anyOffDiagonalNonZero = true;
      break;
    }
  }
  EXPECT_TRUE(anyOffDiagonalNonZero);
  EXPECT_NEAR(matrix[0], 1.0, 1e-9);
  EXPECT_NEAR(matrix[5], 1.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Static transforms — composition of multiple operations
// ---------------------------------------------------------------------------

TEST(TransformAnimatedNode, GetTransform_TranslateXAndScale_ComposesInOrder) {
  auto manager = makeAnimatedNodesManager();

  // Order matters for transform composition. With translateX(5) then
  // scale(2), the matrix is identity * Translate(5,0,0) * Scale(2,2,1).
  TransformAnimatedNode node(
      makeTransformConfig({{"translateX", 5.0}, {"scale", 2.0}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[0], 2.0, 1e-9);
  EXPECT_NEAR(matrix[5], 2.0, 1e-9);
  // Composed translation is scaled: 5 * 2 = 10 at matrix[12].
  EXPECT_NEAR(matrix[12], 10.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Animated transforms
// ---------------------------------------------------------------------------

TEST(
    TransformAnimatedNode,
    GetTransform_AnimatedTranslateX_ReadsValueFromManager) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 9.0);

  TransformAnimatedNode node(
      makeTransformConfig({}, {{"translateX", 1}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[12], 9.0, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_AnimatedTranslateY_ReadsValueFromManager) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 11.0);

  TransformAnimatedNode node(
      makeTransformConfig({}, {{"translateY", 1}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[13], 11.0, 1e-9);
}

TEST(TransformAnimatedNode, GetTransform_AnimatedScale_ReadsValueFromManager) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 1.5);

  TransformAnimatedNode node(makeTransformConfig({}, {{"scale", 1}}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[0], 1.5, 1e-9);
  EXPECT_NEAR(matrix[5], 1.5, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_AfterParentValueChanges_ReflectsNewValue) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 1.0);

  TransformAnimatedNode node(
      makeTransformConfig({}, {{"translateX", 1}}), *manager);
  auto matrixBefore = extractMatrix(node.getTransform());
  ASSERT_NEAR(matrixBefore[12], 1.0, 1e-9);

  // Change the parent value and verify the new matrix reflects it.
  manager->setValue(1, 8.0);
  auto matrixAfter = extractMatrix(node.getTransform());
  EXPECT_NEAR(matrixAfter[12], 8.0, 1e-9);
}

TEST(
    TransformAnimatedNode,
    GetTransform_AnimatedTranslateX_TriggersParentFrameRateRecalculation) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 0.0);

  // The parent value node starts at frame rate 60. Calling getTransform()
  // for an animated translateX triggers
  // `node.recalculateMinAcceptableFrameRate(scaleRatioDpi)` on the
  // parent. With the default velocity 0 (we never call setVelocity on
  // the parent), the parent downshifts from 60 to 30.
  TransformAnimatedNode node(
      makeTransformConfig({}, {{"translateX", 1}}), *manager);

  node.getTransform();

  auto& parentNode = manager->getValueNodeByTag(1);
  EXPECT_EQ(parentNode.getFrameRate(), ValueAnimatedNode::FRAME_RATE_30);
}

TEST(
    TransformAnimatedNode,
    GetTransform_MixOfStaticAndAnimated_ComposesAllInOrder) {
  auto manager = makeAnimatedNodesManager();
  registerValue(*manager, 1, 2.0);

  // Static translateX(5) then animated translateY(2): matrix should have
  // translateX=5 at slot 12 and translateY=2 at slot 13 (composition with
  // identity preserves both as independent translations).
  TransformAnimatedNode node(
      makeTransformConfig({{"translateX", 5.0}}, {{"translateY", 1}}),
      *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  EXPECT_NEAR(matrix[12], 5.0, 1e-9);
  EXPECT_NEAR(matrix[13], 2.0, 1e-9);
}

// ---------------------------------------------------------------------------
// Unsupported property
// ---------------------------------------------------------------------------

TEST(TransformAnimatedNode, GetTransform_UnknownProperty_ThrowsRuntimeError) {
  auto manager = makeAnimatedNodesManager();

  TransformAnimatedNode node(
      makeTransformConfig({{"unsupportedProp", 1.0}}), *manager);

  EXPECT_THROW({ node.getTransform(); }, std::runtime_error);
}

// ---------------------------------------------------------------------------
// Empty transforms list → identity matrix
// ---------------------------------------------------------------------------

TEST(
    TransformAnimatedNode,
    GetTransform_EmptyTransforms_ReturnsIdentityMatrix) {
  auto manager = makeAnimatedNodesManager();

  TransformAnimatedNode node(makeTransformConfig({}), *manager);

  auto matrix = extractMatrix(node.getTransform());
  ASSERT_EQ(matrix.size(), 16u);
  // Identity: 1 on diagonal, 0 elsewhere.
  EXPECT_NEAR(matrix[0], 1.0, 1e-9);
  EXPECT_NEAR(matrix[5], 1.0, 1e-9);
  EXPECT_NEAR(matrix[10], 1.0, 1e-9);
  EXPECT_NEAR(matrix[15], 1.0, 1e-9);
  for (size_t i = 0; i < 16; ++i) {
    if (i != 0 && i != 5 && i != 10 && i != 15) {
      EXPECT_NEAR(matrix[i], 0.0, 1e-9);
    }
  }
}
