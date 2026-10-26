/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for `rnoh::InterpolationAnimatedNode` — the node
 * that maps a parent `ValueAnimatedNode` through an input/output range
 * pair with optional extrapolation (`identity` / `clamp` / `extend`).
 *
 * Per `gtest-module-catalog.md` the module's Mock requirement is
 * "Mock AnimatedNodesManager". The manager is non-virtual, so per guide
 * §5.7 we construct a real `AnimatedNodesManager` instance via
 * `AnimatedTestHelpers.h` and register a parent value node with it. The
 * interpolation node is constructed directly (without going through
 * `createNode`) so the test can call `update()` and observe the
 * resulting `getOutput()`.
 *
 * Tests exercise:
 *   - Linear number interpolation (default `OutputType::Number`)
 *   - Extrapolation modes: identity / clamp / extend (left & right)
 *   - Edge cases: zero input/output range, value exactly at a knot,
 *     value below inputMin, value above inputMax
 *   - String interpolation (numeric values inside a template string)
 *   - Color interpolation (linear blend between two RGBA colours)
 *   - onAttachedToNode / onDetachedFromNode wiring (m_parent state)
 *   - Detach from non-parent tag throws (production contract)
 */

#include <gtest/gtest.h>

#include <folly/dynamic.h>

#include "AnimatedTestHelpers.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/InterpolationAnimatedNode.h"
#include "RNOHCorePackage/TurboModules/Animated/Nodes/ValueAnimatedNode.h"

using namespace rnoh;
using rnoh::test::makeAnimatedNodesManager;

namespace {

folly::dynamic makeValueNodeConfig(double value, double offset = 0.0) {
  return folly::dynamic::object("type", "value")("value", value)(
      "offset", offset);
}

/**
 * Builds an interpolation config matching the schema produced by the JS
 * `Animated.interpolate` binding. The output-range parameter is templated
 * so the helper can be called with `std::vector<double>` (number
 * interpolation), `std::vector<std::string>` (string interpolation), or
 * `std::vector<uint32_t>` (colour interpolation). Callers must pass the
 * output range as a *typed* `std::vector<...>` constructor (not a braced
 * initializer list) to avoid the `vector(InputIt, InputIt)` constructor
 * trap that would otherwise convert `{"rotate(0deg)"}` to a sequence of
 * char codes [114, 111, 116, ...].
 */
template <typename OutputT>
folly::dynamic makeInterpolationConfig(
    const std::vector<double>& inputRange,
    const std::vector<OutputT>& outputRange,
    std::string extrapolateLeft = "clamp",
    std::string extrapolateRight = "clamp",
    std::string outputType = "") {
  folly::dynamic input = folly::dynamic::array;
  for (auto v : inputRange) {
    input.push_back(v);
  }
  folly::dynamic output = folly::dynamic::array;
  for (auto const& v : outputRange) {
    output.push_back(folly::dynamic(v));
  }
  // The production constructor reads `config["outputType"]` via the const
  // operator[], which throws if the key is missing. Always set the key
  // (possibly empty) so the lookup succeeds and the auto-detection
  // fallback path takes over when no explicit type is given.
  return folly::dynamic::object("inputRange", input)("outputRange", output)(
      "extrapolateLeft", extrapolateLeft)("extrapolateRight", extrapolateRight)(
      "outputType", outputType);
}

/**
 * Convenience overload accepting two braced initializer lists of doubles.
 * Without this overload, calls like
 *   makeInterpolationConfig({0.0, 1.0}, {0.0, 100.0})
 * would fail template argument deduction (braced init lists don't
 * participate in deduction).
 */
folly::dynamic makeInterpolationConfig(
    std::initializer_list<double> inputRange,
    std::initializer_list<double> outputRange,
    std::string extrapolateLeft = "clamp",
    std::string extrapolateRight = "clamp",
    std::string outputType = "") {
  return makeInterpolationConfig(
      std::vector<double>(inputRange),
      std::vector<double>(outputRange),
      std::move(extrapolateLeft),
      std::move(extrapolateRight),
      std::move(outputType));
}

/**
 * Registers a value node with the manager at `parentTag` carrying the
 * given value. The tag is fixed per test so the test is self-contained
 * (no shared state across tests).
 */
void registerParentValue(
    AnimatedNodesManager& manager,
    facebook::react::Tag parentTag,
    double value) {
  manager.createNode(parentTag, makeValueNodeConfig(value, 0.0));
}
} // namespace

// ---------------------------------------------------------------------------
// Linear number interpolation
// ---------------------------------------------------------------------------

TEST(InterpolationAnimatedNode, Update_Midpoint_ReturnsMidpointOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {0.0, 100.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  auto output = node.getOutput();
  ASSERT_TRUE(output.isDouble());
  EXPECT_NEAR(output.asDouble(), 50.0, 1e-6);
}

TEST(InterpolationAnimatedNode, Update_AtLowerKnot_ReturnsLowerOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 10.0, 1e-6);
}

TEST(InterpolationAnimatedNode, Update_AtUpperKnot_ReturnsUpperOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 1.0);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 20.0, 1e-6);
}

TEST(InterpolationAnimatedNode, Update_MultiSegmentRange_PicksCorrectSegment) {
  // Three knots → two segments: [0,1]→[0,100] and [1,2]→[100,200]
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 1.5);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0, 2.0}, {0.0, 100.0, 200.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 150.0, 1e-6);
}

// ---------------------------------------------------------------------------
// Extrapolation modes
// ---------------------------------------------------------------------------

TEST(
    InterpolationAnimatedNode,
    Update_BelowInputMin_WithClamp_ReturnsLowerOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, -1.0);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}, "clamp", "clamp"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 10.0, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_AboveInputMax_WithClamp_ReturnsUpperOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 2.0);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}, "clamp", "clamp"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 20.0, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_BelowInputMin_WithIdentity_ReturnsRawValue) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, -3.5);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}, "identity", "clamp"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  // Identity extrapolation returns the raw input value (-3.5).
  EXPECT_NEAR(node.getOutput().asDouble(), -3.5, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_AboveInputMax_WithIdentity_ReturnsRawValue) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 4.2);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}, "clamp", "identity"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 4.2, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_AboveInputMax_WithExtend_ContinuesLinearly) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 2.0);

  // slope = (20 - 10) / (1 - 0) = 10 per unit. Extend past 1.0:
  // output = 20 + (2.0 - 1.0) * 10 = 30
  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}, "clamp", "extend"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 30.0, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_BelowInputMin_WithExtend_ContinuesLinearly) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, -1.0);

  // slope = 10 per unit. Extend past 0.0:
  // output = 10 + (-1.0 - 0.0) * 10 = 0
  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {10.0, 20.0}, "extend", "clamp"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 0.0, 1e-6);
}

// ---------------------------------------------------------------------------
// Degenerate ranges
// ---------------------------------------------------------------------------

TEST(
    InterpolationAnimatedNode,
    Update_DegenerateOutputRange_ReturnsConstantOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);

  // outputMin == outputMax: production short-circuits to outputMin.
  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {42.0, 42.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 42.0, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_DegenerateInputRange_BelowInputMin_ReturnsLowerOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, -1.0);

  // inputMin == inputMax. Production: if value < inputMax → outputMin.
  InterpolationAnimatedNode node(
      makeInterpolationConfig({1.0, 1.0}, {10.0, 20.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 10.0, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    Update_DegenerateInputRange_AtOrAboveInputMax_ReturnsUpperOutput) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 1.0);

  // inputMin == inputMax. value >= inputMax → outputMax.
  InterpolationAnimatedNode node(
      makeInterpolationConfig({1.0, 1.0}, {10.0, 20.0}), *manager);
  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 20.0, 1e-6);
}

// ---------------------------------------------------------------------------
// String interpolation
// ---------------------------------------------------------------------------

TEST(
    InterpolationAnimatedNode,
    Update_StringOutputType_InterpolatesEmbeddedNumbers) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);

  InterpolationAnimatedNode node(
      makeInterpolationConfig(
          {0.0, 1.0},
          std::vector<std::string>{"rotate(0deg)", "rotate(90deg)"},
          "clamp",
          "clamp",
          "string"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  auto output = node.getOutput();
  ASSERT_TRUE(output.isString());
  // The interpolation replaces the numeric segment with the interpolated
  // value. At ratio=0.5, the value should be 45 (formatted with
  // std::to_string(double) as "45.000000"). The result must retain the
  // "rotate(" prefix from the output-range template.
  std::string result = output.getString();
  EXPECT_NE(result.find("rotate"), std::string::npos);
  EXPECT_NE(result.find("45"), std::string::npos);
}

TEST(
    InterpolationAnimatedNode,
    Update_StringOutputType_AtLowerKnot_ReturnsStartString) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  InterpolationAnimatedNode node(
      makeInterpolationConfig(
          {0.0, 1.0},
          std::vector<std::string>{"rotate(0deg)", "rotate(90deg)"},
          "clamp",
          "clamp",
          "string"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  auto output = node.getOutput();
  ASSERT_TRUE(output.isString());
  // Production uses `std::to_string(double)` which yields "0.000000"
  // rather than "0". Assert the literal start-string content is replaced
  // by the interpolated numeric form.
  std::string result = output.getString();
  EXPECT_NE(result.find("rotate"), std::string::npos);
  EXPECT_NE(result.find("0.000000"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Color interpolation
// ---------------------------------------------------------------------------

TEST(InterpolationAnimatedNode, Update_ColorOutputType_BlendsColorsAtMidpoint) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);

  // Black → White. At midpoint the blended colour should be a mid-grey,
  // i.e. R=G=B≈127 (depending on float-to-uint conversion rounding).
  InterpolationAnimatedNode node(
      makeInterpolationConfig(
          {0.0, 1.0},
          std::vector<uint32_t>{0xFF000000u, 0xFFFFFFFFu},
          "clamp",
          "clamp",
          "color"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  // Production stores the blended colour via `setValue(ColorValue)` where
  // `ColorValue = uint32_t`. C++ overload resolution picks
  // `setValue(double)` over `setValue(folly::dynamic)` because the
  // `uint32_t → double` standard conversion is preferred over the
  // user-defined `uint32_t → folly::dynamic` conversion. The stored
  // value is therefore a double whose magnitude equals the packed
  // 0xAARRGGBB integer.
  auto output = node.getOutput();
  ASSERT_TRUE(output.isDouble());
  auto colorValue = static_cast<uint32_t>(output.asDouble());
  // Extract the red, green, blue byte (alpha is the highest byte).
  uint8_t red = (colorValue >> 16) & 0xFF;
  uint8_t green = (colorValue >> 8) & 0xFF;
  uint8_t blue = colorValue & 0xFF;
  EXPECT_NEAR(red, 127, 5);
  EXPECT_NEAR(green, 127, 5);
  EXPECT_NEAR(blue, 127, 5);
}

TEST(
    InterpolationAnimatedNode,
    Update_ColorOutputType_AtLowerKnot_ReturnsStartColor) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.0);

  InterpolationAnimatedNode node(
      makeInterpolationConfig(
          {0.0, 1.0},
          std::vector<uint32_t>{0xFF112233u, 0xFFFFFFFFu},
          "clamp",
          "clamp",
          "color"),
      *manager);
  node.onAttachedToNode(1);
  node.update();

  // At the lower knot the production code clamps mixValue to 0 and
  // returns colour A unchanged. The stored value is a double (see the
  // BlendsColorsAtMidpoint test for the overload-resolution rationale).
  ASSERT_TRUE(node.getOutput().isDouble());
  EXPECT_EQ(static_cast<uint32_t>(node.getOutput().asDouble()), 0xFF112233u);
}

// ---------------------------------------------------------------------------
// Lifecycle: attach / detach
// ---------------------------------------------------------------------------

TEST(InterpolationAnimatedNode, OnAttachedToNode_SetsParentAndEnablesUpdate) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {0.0, 100.0}), *manager);

  // Before attach, update() is a no-op (graph still being constructed).
  EXPECT_NO_THROW({ node.update(); });
  // No output has been produced — the stored value is still the
  // folly::dynamic default-constructed null.
  EXPECT_TRUE(node.getOutput().isNull());

  node.onAttachedToNode(1);
  node.update();

  EXPECT_NEAR(node.getOutput().asDouble(), 50.0, 1e-6);
}

TEST(
    InterpolationAnimatedNode,
    OnDetachedFromNode_FromNonParentTag_ThrowsRuntimeError) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);
  auto nonParentTag = 999;

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {0.0, 100.0}), *manager);
  node.onAttachedToNode(1);

  EXPECT_THROW(node.onDetachedFromNode(nonParentTag), std::runtime_error);
}

TEST(
    InterpolationAnimatedNode,
    OnDetachedFromNode_FromParentTag_ClearsParentAndStopsUpdating) {
  auto manager = makeAnimatedNodesManager();
  registerParentValue(*manager, 1, 0.5);

  InterpolationAnimatedNode node(
      makeInterpolationConfig({0.0, 1.0}, {0.0, 100.0}), *manager);
  node.onAttachedToNode(1);
  node.update();
  ASSERT_NEAR(node.getOutput().asDouble(), 50.0, 1e-6);

  node.onDetachedFromNode(1);
  // After detach, update() returns early (m_parent == nullopt) and the
  // previously stored value is preserved.
  EXPECT_NO_THROW({ node.update(); });
  EXPECT_NEAR(node.getOutput().asDouble(), 50.0, 1e-6);
}

// ---------------------------------------------------------------------------
// Invalid extrapolation string throws
// ---------------------------------------------------------------------------

TEST(
    InterpolationAnimatedNode,
    Constructor_InvalidExtrapolationType_ThrowsRuntimeError) {
  auto manager = makeAnimatedNodesManager();

  EXPECT_THROW(
      InterpolationAnimatedNode(
          makeInterpolationConfig({0.0, 1.0}, {0.0, 1.0}, "bogus", "clamp"),
          *manager),
      std::runtime_error);
}
