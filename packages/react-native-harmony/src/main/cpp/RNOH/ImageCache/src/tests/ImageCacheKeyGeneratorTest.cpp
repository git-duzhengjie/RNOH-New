/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include "ImageCacheKeyGeneratorDefault.h"
#include "ImageCacheOption.h" // Must include before KeyGeneratorDefault (inline ctors access Option members)

using namespace ImageCachePro;

// ============================================================================
// Tier 2 — glog stub + ohresmgr stub
// Module: RNOH/ImageCache/src/ImageCacheKeyGeneratorDefault
//
// Tested class: ImageCacheKeyGeneratorDefault (cache key generation)
// Generates request/memory keys from ImageCacheSource + params.
// ============================================================================

class KeyGeneratorTest : public ::testing::Test {
 protected:
  ImageCacheKeyGeneratorDefault generator;
  ImageCacheSource source;

  ImageCacheKeyGenerator::MemoryKeyParams makeParams() {
    ImageCacheKeyGenerator::MemoryKeyParams params;
    params.imageSource = &source;
    params.type = ImageCacheRequestType::MAIN_SRC;
    return params;
  }
};

// ---------------------------------------------------------------------------
// String source key
// ---------------------------------------------------------------------------

TEST_F(KeyGeneratorTest, GenerateRequestKey_StringSource_ContainsUrl) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateRequestKey(makeParams());
  EXPECT_TRUE(key.find("src=http://example.com/img.png;") != std::string::npos);
}

TEST_F(
    KeyGeneratorTest,
    GenerateRequestKey_StringSource_HasOrientationAndDynamicRange) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateRequestKey(makeParams());
  // MAIN_SRC type includes orientation and dynamicRange
  EXPECT_TRUE(key.find("o:") != std::string::npos);
  EXPECT_TRUE(key.find("d:") != std::string::npos);
}

TEST_F(KeyGeneratorTest, GenerateMemoryKey_StringSource_ContainsUrl) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateMemoryKey(makeParams());
  EXPECT_TRUE(key.find("src=http://example.com/img.png;") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Resource source key
// ---------------------------------------------------------------------------

TEST_F(
    KeyGeneratorTest,
    GenerateRequestKey_ResourceSource_ContainsResourceInfo) {
  source.SetResource({"com.app", "module", 42, "param"});
  auto key = generator.GenerateRequestKey(makeParams());
  EXPECT_TRUE(key.find("com.app") != std::string::npos);
  EXPECT_TRUE(key.find("module") != std::string::npos);
  EXPECT_TRUE(key.find("42") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Type-specific behavior
// ---------------------------------------------------------------------------

TEST_F(KeyGeneratorTest, GenerateRequestKey_NonMainSrc_NoOrientation) {
  source.SetString("http://example.com/img.png");
  auto params = makeParams();
  params.type = ImageCacheRequestType::PLACE_SRC;
  auto key = generator.GenerateRequestKey(params);
  // Non-MAIN_SRC should not include orientation
  EXPECT_TRUE(key.find("o:") == std::string::npos);
}

// ---------------------------------------------------------------------------
// isAnimator
// ---------------------------------------------------------------------------

TEST_F(KeyGeneratorTest, GenerateMemoryKey_Animator_HasAnimatorFlag) {
  source.SetString("http://example.com/img.png");
  auto params = makeParams();
  params.isAnimator = true;
  auto key = generator.GenerateMemoryKey(params);
  EXPECT_TRUE(key.find("animator=true") != std::string::npos);
}

TEST_F(KeyGeneratorTest, GenerateRequestKey_NonAnimator_NoAnimatorFlag) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateRequestKey(makeParams());
  EXPECT_TRUE(key.find("animator=true") == std::string::npos);
}

// ---------------------------------------------------------------------------
// Transformation info
// ---------------------------------------------------------------------------

TEST_F(
    KeyGeneratorTest,
    GenerateRequestKey_WithTransformation_ContainsTransformationInfo) {
  source.SetString("http://example.com/img.png");
  auto params = makeParams();
  params.transformationInfo = "blur:5";
  auto key = generator.GenerateRequestKey(params);
  EXPECT_TRUE(key.find("blur:5") != std::string::npos);
}

TEST_F(
    KeyGeneratorTest,
    GenerateRequestKey_WithoutTransformation_NoTransformationInfo) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateRequestKey(makeParams());
  // No transformationInfo set → not in key
  EXPECT_TRUE(key.find("blur") == std::string::npos);
}

// ---------------------------------------------------------------------------
// resizeMultiplier
// ---------------------------------------------------------------------------

TEST_F(
    KeyGeneratorTest,
    GenerateRequestKey_NonUnitResizeMultiplier_HasRmPrefix) {
  source.SetString("http://example.com/img.png");
  auto params = makeParams();
  params.resizeMultiplier = 2.0f;
  auto key = generator.GenerateRequestKey(params);
  EXPECT_TRUE(key.find("rm:") != std::string::npos);
}

TEST_F(KeyGeneratorTest, GenerateRequestKey_UnitResizeMultiplier_NoRmPrefix) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateRequestKey(makeParams());
  EXPECT_TRUE(key.find("rm:") == std::string::npos);
}

// ---------------------------------------------------------------------------
// Signature
// ---------------------------------------------------------------------------

TEST_F(KeyGeneratorTest, GenerateRequestKey_WithSignature_ContainsSignature) {
  source.SetString("http://example.com/img.png");
  auto params = makeParams();
  params.signature = "abc123";
  auto key = generator.GenerateRequestKey(params);
  EXPECT_TRUE(key.find("signature=abc123") != std::string::npos);
}

TEST_F(KeyGeneratorTest, GenerateRequestKey_WithoutSignature_NoSignatureField) {
  source.SetString("http://example.com/img.png");
  auto key = generator.GenerateRequestKey(makeParams());
  EXPECT_TRUE(key.find("signature=") == std::string::npos);
}

// ---------------------------------------------------------------------------
// Null source
// ---------------------------------------------------------------------------

TEST_F(KeyGeneratorTest, GenerateRequestKey_NullSource_ReturnsEmpty) {
  ImageCacheKeyGenerator::MemoryKeyParams params;
  params.imageSource = nullptr;
  auto key = generator.GenerateRequestKey(params);
  EXPECT_TRUE(key.empty());
}

// ---------------------------------------------------------------------------
// MemoryKeyParams constructors (from ImageCacheOption)
// ---------------------------------------------------------------------------

TEST_F(KeyGeneratorTest, MemoryKeyParams_SharedPtrOption_PopulatesFields) {
  source.SetString("http://example.com/img.png");
  auto opt = ImageCacheOption::Create(
      "http://example.com/img.png", "prefix", "resize", 100, 200);
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &source, ImageCacheRequestType::MAIN_SRC, opt, false);
  EXPECT_EQ(params.imageSource, &source);
  EXPECT_EQ(params.type, ImageCacheRequestType::MAIN_SRC);
  EXPECT_FALSE(params.isAnimator);
  EXPECT_FLOAT_EQ(params.resizeMultiplier, 1.0f);
}

TEST_F(KeyGeneratorTest, MemoryKeyParams_SharedPtrNullOption_KeepsDefaults) {
  source.SetString("http://example.com/img.png");
  std::shared_ptr<ImageCacheOption> nullOpt;
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &source, ImageCacheRequestType::PLACE_SRC, nullOpt, true);
  EXPECT_EQ(params.imageSource, &source);
  EXPECT_EQ(params.type, ImageCacheRequestType::PLACE_SRC);
  EXPECT_TRUE(params.isAnimator);
  EXPECT_TRUE(params.signature.empty());
  EXPECT_FLOAT_EQ(params.resizeMultiplier, 1.0f);
}

TEST_F(KeyGeneratorTest, MemoryKeyParams_RefOption_PopulatesFields) {
  source.SetString("http://example.com/img.png");
  auto opt =
      ImageCacheOption::Create("http://example.com/img.png", "", "", 50, 60);
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &source, ImageCacheRequestType::ERROR_SRC, *opt, true);
  EXPECT_EQ(params.imageSource, &source);
  EXPECT_EQ(params.type, ImageCacheRequestType::ERROR_SRC);
  EXPECT_TRUE(params.isAnimator);
  EXPECT_FLOAT_EQ(params.resizeMultiplier, 1.0f);
}

TEST_F(KeyGeneratorTest, MemoryKeyParams_DefaultConstructor_HasDefaults) {
  ImageCacheKeyGenerator::MemoryKeyParams params;
  EXPECT_EQ(params.imageSource, nullptr);
  EXPECT_EQ(params.type, ImageCacheRequestType::MAIN_SRC);
  EXPECT_FALSE(params.isAnimator);
  EXPECT_FLOAT_EQ(params.resizeMultiplier, 1.0f);
  EXPECT_TRUE(params.signature.empty());
  EXPECT_TRUE(params.transformationInfo.empty());
}

TEST_F(
    KeyGeneratorTest,
    MemoryKeyParams_SharedPtrOption_AnimatorFlag_Preserved) {
  source.SetString("http://example.com/anim.gif");
  auto opt =
      ImageCacheOption::Create("http://example.com/anim.gif", "", "", 10, 10);
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &source, ImageCacheRequestType::MAIN_SRC, opt, true);
  EXPECT_TRUE(params.isAnimator);
}

TEST_F(KeyGeneratorTest, MemoryKeyParams_RefOption_NonAnimator_FlagFalse) {
  source.SetString("http://example.com/img.png");
  auto opt =
      ImageCacheOption::Create("http://example.com/img.png", "", "", 10, 10);
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &source, ImageCacheRequestType::MAIN_SRC, *opt, false);
  EXPECT_FALSE(params.isAnimator);
}
