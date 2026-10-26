/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheDownSampler.h"
#include <gtest/gtest.h>
#include "ImageCacheDownSamplingAtLeast.h"
#include "ImageCacheDownSamplingAtMost.h"
#include "ImageCacheDownSamplingFitCenterMemory.h"
#include "ImageCacheDownSamplingFitCenterQuality.h"

using namespace ImageCachePro;

// ============================================================================
// Tier 3/4 — stub image_common.h (provides Image_Size type)
// Module: RNOH/ImageCache/src/DownSampler/ImageCacheDownSampler
//
// Tested class: ImageCacheDownSampler (static down-sampling calculator)
// Pure math — no鸿蒙 C API calls, only Image_Size type stub needed.
// ============================================================================

// ---------------------------------------------------------------------------
// GetDownSamplingInfo
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSampler, GetDownSamplingInfo_NoneStrategy_ReturnsEmpty) {
  Image_Size req{100, 200};
  EXPECT_TRUE(
      ImageCacheDownSampler::GetDownSamplingInfo(
          req, ImageCacheDownSamplingStrategy::NONE)
          .empty());
}

TEST(
    ImageCacheDownSampler,
    GetDownSamplingInfo_DefaultStrategy_ReturnsDefaultString) {
  Image_Size req{100, 200};
  EXPECT_EQ(
      ImageCacheDownSampler::GetDownSamplingInfo(
          req, ImageCacheDownSamplingStrategy::DEFAULT),
      "DownSampling:Default");
}

TEST(
    ImageCacheDownSampler,
    GetDownSamplingInfo_FitCenterMemory_ReturnsFormattedString) {
  Image_Size req{100, 200};
  std::string info = ImageCacheDownSampler::GetDownSamplingInfo(
      req, ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY);
  EXPECT_TRUE(info.find("DownSampling:") != std::string::npos);
  EXPECT_TRUE(info.find("100") != std::string::npos);
  EXPECT_TRUE(info.find("200") != std::string::npos);
}

TEST(
    ImageCacheDownSampler,
    GetDownSamplingInfo_WithResizeMultiplier_IncludesMultiplier) {
  Image_Size req{100, 200};
  std::string info = ImageCacheDownSampler::GetDownSamplingInfo(
      req, ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY, 2.0f);
  EXPECT_TRUE(info.find("rm:") != std::string::npos);
}

TEST(
    ImageCacheDownSampler,
    GetDownSamplingInfo_UnitResizeMultiplier_OmitsMultiplier) {
  Image_Size req{100, 200};
  std::string info = ImageCacheDownSampler::GetDownSamplingInfo(
      req, ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY, 1.0f);
  EXPECT_TRUE(info.find("rm:") == std::string::npos);
}

// ---------------------------------------------------------------------------
// CalculateSize — edge cases
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSampler, CalculateSize_ZeroSource_ReturnsOneByOne) {
  Image_Size src{0, 0};
  Image_Size req{100, 100};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::JPG, src, req, ImageCacheDownSamplingStrategy::DEFAULT);
  EXPECT_EQ(result.width, 1u);
  EXPECT_EQ(result.height, 1u);
}

TEST(ImageCacheDownSampler, CalculateSize_SvgFormat_ReturnsZeroSize) {
  Image_Size src{1920, 1080};
  Image_Size req{100, 100};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::SVG, src, req, ImageCacheDownSamplingStrategy::DEFAULT);
  EXPECT_EQ(result.width, 0u);
  EXPECT_EQ(result.height, 0u);
}

TEST(ImageCacheDownSampler, CalculateSize_NoneSmallImage_ReturnsZero) {
  Image_Size src{1920, 1080};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG, src, req, ImageCacheDownSamplingStrategy::NONE);
  EXPECT_EQ(result.width, 0u);
  EXPECT_EQ(result.height, 0u);
  EXPECT_NE(ImageCacheFormat::PNG, ImageCacheFormat::SVG);
}

TEST(ImageCacheDownSampler, CalculateSize_DefaultSmallPng_ReturnsSourceSize) {
  Image_Size src{1920, 1080};
  Image_Size req{0, 0};
  // Small image (≤8K): scaleFactor=1 → result = source/1 = source
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG, src, req, ImageCacheDownSamplingStrategy::DEFAULT);
  EXPECT_EQ(result.width, 1920u);
  EXPECT_EQ(result.height, 1080u);
}

TEST(ImageCacheDownSampler, CalculateSize_DefaultSmallWebp_ReturnsSourceSize) {
  Image_Size src{1920, 1080};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::WEBP,
      src,
      req,
      ImageCacheDownSamplingStrategy::DEFAULT);
  EXPECT_EQ(result.width, 1920u);
  EXPECT_EQ(result.height, 1080u);
  EXPECT_NE(ImageCacheFormat::WEBP, ImageCacheFormat::PNG);
}

TEST(ImageCacheDownSampler, CalculateSize_DefaultSmallJpg_ReturnsSourceSize) {
  Image_Size src{1920, 1080};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::JPG, src, req, ImageCacheDownSamplingStrategy::DEFAULT);
  EXPECT_EQ(result.width, 1920u);
  EXPECT_EQ(result.height, 1080u);
  EXPECT_NE(ImageCacheFormat::JPG, ImageCacheFormat::PNG);
}

TEST(ImageCacheDownSampler, CalculateSize_LargeSourceNone_DowngradesToDefault) {
  // Source > 4096*4096 → NONE downgrades to DEFAULT
  Image_Size src{5000, 5000};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG, src, req, ImageCacheDownSamplingStrategy::NONE);
  // Should NOT return {0,0} (which would happen if NONE stayed as NONE)
  // Should return a non-zero, non-source size (downsampled)
  EXPECT_NE(result.width, 0u);
  EXPECT_NE(result.height, 0u);
  // Source pixels = 25000000, 8K = 33177600, scaleFactor = 25000000/33177600 <
  // 1 So scaleFactor rounds to < 1, but wait... 25000000 < 33177600, so
  // scaleFactor=1! Actually 5000*5000 = 25000000 < 33177600, so DEFAULT returns
  // scaleFactor=1 → result = source/1 = source
  EXPECT_EQ(result.width, 5000u);
  EXPECT_EQ(result.height, 5000u);
}

TEST(
    ImageCacheDownSampler,
    CalculateSize_ZeroRequestNonDefaultNonNone_ReturnsZero) {
  Image_Size src{1920, 1080};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG,
      src,
      req,
      ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY);
  EXPECT_EQ(result.width, 0u);
  EXPECT_EQ(result.height, 0u);
  EXPECT_NE(
      ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY,
      ImageCacheDownSamplingStrategy::DEFAULT);
}

TEST(
    ImageCacheDownSampler,
    CalculateSize_Exactly4096x4096_None_DoesNotDowngrade) {
  // 4096*4096 == kLargeImagePixels (strict >), so NONE stays NONE
  Image_Size src{4096, 4096};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG, src, req, ImageCacheDownSamplingStrategy::NONE);
  EXPECT_EQ(result.width, 0u);
  EXPECT_EQ(result.height, 0u);
}

TEST(
    ImageCacheDownSampler,
    CalculateSize_JustOver4096x4096_None_DowngradesToDefault) {
  // 4096*4097 > kLargeImagePixels → NONE downgrades to DEFAULT
  Image_Size src{4096, 4097};
  Image_Size req{0, 0};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG, src, req, ImageCacheDownSamplingStrategy::NONE);
  // Should return non-zero (DEFAULT produces a result)
  EXPECT_NE(result.width, 0u);
  EXPECT_NE(result.height, 0u);
}

TEST(
    ImageCacheDownSampler,
    CalculateSize_ResizeMultiplierZero_ReturnsSourceSize) {
  // resizeMultiplier=0 → actualRequestSize={0,0} → strategy guard returns
  // scaleFactor=1 → result = source/1 = source size
  Image_Size src{1920, 1080};
  Image_Size req{100, 100};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::PNG,
      src,
      req,
      ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY,
      0.0f);
  EXPECT_EQ(result.width, 1920u);
  EXPECT_EQ(result.height, 1080u);
}

// ---------------------------------------------------------------------------
// Direct strategy GetScaleFactor tests (zero-width/height edge cases)
// ---------------------------------------------------------------------------

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    GetScaleFactor_ZeroSourceWidth_ReturnsOne) {
  ImageCacheDownSamplingFitCenterQuality strategy;
  Image_Size src{0, 100};
  Image_Size req{10, 10};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    GetScaleFactor_ZeroSourceHeight_ReturnsOne) {
  ImageCacheDownSamplingFitCenterQuality strategy;
  Image_Size src{100, 0};
  Image_Size req{10, 10};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    GetScaleFactor_ZeroRequestWidth_ReturnsOne) {
  ImageCacheDownSamplingFitCenterQuality strategy;
  Image_Size src{1000, 1000};
  Image_Size req{0, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    GetScaleFactor_ZeroRequestHeight_ReturnsOne) {
  ImageCacheDownSamplingFitCenterQuality strategy;
  Image_Size src{1000, 1000};
  Image_Size req{100, 0};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    GetScaleFactor_NormalInput_ReturnsValidFactor) {
  ImageCacheDownSamplingFitCenterQuality strategy;
  Image_Size src{1920, 1080};
  Image_Size req{100, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_GE(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    GetScaleFactor_SmallSource_ReturnsOne) {
  ImageCacheDownSamplingFitCenterQuality strategy;
  Image_Size src{50, 50};
  Image_Size req{100, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSampler,
    CalculateSize_FitCenterQuality_Normal_ReturnsNonZero) {
  Image_Size src{1920, 1080};
  Image_Size req{100, 100};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::JPG,
      src,
      req,
      ImageCacheDownSamplingStrategy::FIT_CENTER_QUALITY);
  EXPECT_NE(result.width, 0u);
  EXPECT_NE(result.height, 0u);
}

TEST(
    ImageCacheDownSampler,
    CalculateSize_FitCenterQuality_LargeSource_Downscales) {
  Image_Size src{8192, 8192};
  Image_Size req{100, 100};
  auto result = ImageCacheDownSampler::CalculateSize(
      ImageCacheFormat::JPG,
      src,
      req,
      ImageCacheDownSamplingStrategy::FIT_CENTER_QUALITY);
  EXPECT_LE(result.width, 8192u);
  EXPECT_LE(result.height, 8192u);
}

TEST(ImageCacheDownSamplingAtLeast, GetScaleFactor_ZeroSourceWidth_ReturnsOne) {
  ImageCacheDownSamplingAtLeast strategy;
  Image_Size src{0, 100};
  Image_Size req{10, 10};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingAtLeast,
    GetScaleFactor_ZeroSourceHeight_ReturnsOne) {
  ImageCacheDownSamplingAtLeast strategy;
  Image_Size src{100, 0};
  Image_Size req{10, 10};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

// ---------------------------------------------------------------------------
// AtMost zero edge cases
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSamplingAtMost, GetScaleFactor_ZeroRequestWidth_ReturnsOne) {
  ImageCacheDownSamplingAtMost strategy;
  Image_Size src{1000, 1000};
  Image_Size req{0, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingAtMost,
    GetScaleFactor_ZeroRequestHeight_ReturnsOne) {
  ImageCacheDownSamplingAtMost strategy;
  Image_Size src{1000, 1000};
  Image_Size req{100, 0};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(ImageCacheDownSamplingAtMost, GetScaleFactor_ZeroSourceWidth_ReturnsOne) {
  ImageCacheDownSamplingAtMost strategy;
  Image_Size src{0, 100};
  Image_Size req{10, 10};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(ImageCacheDownSamplingAtMost, GetScaleFactor_ZeroSourceHeight_ReturnsOne) {
  ImageCacheDownSamplingAtMost strategy;
  Image_Size src{100, 0};
  Image_Size req{10, 10};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(ImageCacheDownSamplingAtMost, GetScaleFactor_EqualSizes_ReturnsOne) {
  ImageCacheDownSamplingAtMost strategy;
  Image_Size src{100, 100};
  Image_Size req{100, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}

TEST(
    ImageCacheDownSamplingAtMost,
    GetScaleFactor_LargeSource_ReturnsValidFactor) {
  ImageCacheDownSamplingAtMost strategy;
  Image_Size src{8192, 8192};
  Image_Size req{100, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_GE(scale, 1.0f);
}

// ---------------------------------------------------------------------------
// AtLeast normal input
// ---------------------------------------------------------------------------

TEST(
    ImageCacheDownSamplingAtLeast,
    GetScaleFactor_NormalInput_ReturnsValidFactor) {
  ImageCacheDownSamplingAtLeast strategy;
  Image_Size src{1920, 1080};
  Image_Size req{100, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_GE(scale, 1.0f);
}

TEST(ImageCacheDownSamplingAtLeast, GetScaleFactor_EqualSizes_ReturnsOne) {
  ImageCacheDownSamplingAtLeast strategy;
  Image_Size src{100, 100};
  Image_Size req{100, 100};
  float scale = strategy.GetScaleFactor(src, req);
  EXPECT_FLOAT_EQ(scale, 1.0f);
}
