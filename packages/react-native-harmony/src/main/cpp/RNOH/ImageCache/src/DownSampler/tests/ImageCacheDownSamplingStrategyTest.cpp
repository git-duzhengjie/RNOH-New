/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <cmath>
#include "ImageCacheDownSamplingAtLeast.h"
#include "ImageCacheDownSamplingAtMost.h"
#include "ImageCacheDownSamplingBase.h"
#include "ImageCacheDownSamplingCenterInside.h"
#include "ImageCacheDownSamplingDefault.h"
#include "ImageCacheDownSamplingFitCenterMemory.h"
#include "ImageCacheDownSamplingFitCenterQuality.h"

using namespace ImageCachePro;

static Image_Size src(uint32_t w, uint32_t h) {
  return {w, h};
}
static Image_Size req(uint32_t w, uint32_t h) {
  return {w, h};
}

// ============================================================================
// Tier 1 — pure computation, zero platform deps
// 7 down-sampling strategies + base class methods (HighestOneBit/GetFactor)
// Each GetScaleFactor(sourceSize, requestSize) → float sampling factor.
// ============================================================================

// ---------------------------------------------------------------------------
// Base class: GetFactor (protected, tested indirectly via FitCenterQuality)
// GetFactor = 1.0 / HighestOneBit(max(source/request))
// FitCenterQuality then uses GetFactorAsBit(max(source/width, source/height))
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSamplingBase, GetFactor_ExactlyDivisible_PowerOfTwo) {
  // source 200×200, request 100×100 → GetFactor=0.5, width=100
  // GetFactorAsBit(max(200/100, 200/100)) = GetFactorAsBit(2) = 2
  ImageCacheDownSamplingFitCenterQuality s;
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(200, 200), req(100, 100)));
}

TEST(ImageCacheDownSamplingBase, GetFactor_NotPowerOfTwo_RoundsDown) {
  // source 1000×1000, request 100×100 → GetFactor=1/8=0.125, width=125
  // GetFactorAsBit(max(1000/125, 1000/125)) = GetFactorAsBit(8) = 8
  ImageCacheDownSamplingFitCenterQuality s;
  EXPECT_FLOAT_EQ(8.0f, s.GetScaleFactor(src(1000, 1000), req(100, 100)));
}

TEST(ImageCacheDownSamplingBase, GetFactor_SourceSmallerThanRequest_Returns1) {
  // maxIntFactor = max(100/200, 100/200) = 0 → returns 1.0
  ImageCacheDownSamplingFitCenterQuality s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(200, 200)));
}

// ---------------------------------------------------------------------------
// Default: ignores requestSize; 8K threshold
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSamplingDefault, SourceUnder8K_Returns1) {
  ImageCacheDownSamplingDefault s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(1, 1)));
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(1920, 1080), req(100, 100)));
}

TEST(ImageCacheDownSamplingDefault, SourceExactly8K_Returns1) {
  ImageCacheDownSamplingDefault s;
  // 7680 × 4320 = 33,177,600 == resolutionMax8k
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(7680, 4320), req(1, 1)));
}

TEST(ImageCacheDownSamplingDefault, SourceOver8K_ReturnsRatio) {
  ImageCacheDownSamplingDefault s;
  // 8000 × 4320 = 34,560,000; 34560000/33177600 ≈ 1.04167
  EXPECT_NEAR(1.04167f, s.GetScaleFactor(src(8000, 4320), req(1, 1)), 0.001f);
  // 10000 × 10000 = 100,000,000; 100000000/33177600 ≈ 3.0145
  EXPECT_NEAR(3.0145f, s.GetScaleFactor(src(10000, 10000), req(1, 1)), 0.01f);
}

TEST(ImageCacheDownSamplingDefault, IgnoresRequestSize) {
  ImageCacheDownSamplingDefault s;
  EXPECT_FLOAT_EQ(
      s.GetScaleFactor(src(100, 100), req(10, 10)),
      s.GetScaleFactor(src(100, 100), req(9999, 9999)));
}

TEST(ImageCacheDownSamplingDefault, ZeroDimensions_Returns1) {
  ImageCacheDownSamplingDefault s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(0, 0), req(100, 100)));
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(0, 100), req(100, 100)));
}

TEST(ImageCacheDownSamplingDefault, SourcePixelOverflow_Returns1_KnownBug) {
  // 65536 * 65536 = 2^32, wraps to 0 in uint32_t multiplication
  // → resolutionSource=0 <= resolutionMax8k → returns 1 (wrong!)
  // Correct behavior: should return ~129.4 (2^32 / 33177600)
  // Bug: sourceSize.width * sourceSize.height overflows uint32_t
  ImageCacheDownSamplingDefault s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(65536, 65536), req(1, 1)));
}

TEST(ImageCacheDownSamplingDefault, SourceJustOver8K_ReturnsSlightlyAbove1) {
  // 7680 × 4321 = 33,185,280 > 33,177,600 → returns ratio slightly > 1
  ImageCacheDownSamplingDefault s;
  float factor = s.GetScaleFactor(src(7680, 4321), req(1, 1));
  EXPECT_GT(factor, 1.0f);
  EXPECT_NEAR(factor, 33185280.0f / 33177600.0f, 0.001f);
}

TEST(ImageCacheDownSamplingFitCenterMemory, ZeroRequestDimension_Returns1) {
  ImageCacheDownSamplingFitCenterMemory s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(1000, 1000), req(0, 100)));
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(1000, 1000), req(100, 0)));
}

TEST(ImageCacheDownSamplingFitCenterMemory, SourceEqualsRequest_Returns1) {
  ImageCacheDownSamplingFitCenterMemory s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(200, 200), req(200, 200)));
}

TEST(ImageCacheDownSamplingFitCenterQuality, SourceEqualsRequest_Returns1) {
  ImageCacheDownSamplingFitCenterQuality s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(200, 200), req(200, 200)));
}

TEST(ImageCacheDownSamplingAtLeast, SourceEqualsRequest_Returns1) {
  ImageCacheDownSamplingAtLeast s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(200, 200), req(200, 200)));
}

TEST(ImageCacheDownSamplingAtMost, SourceEqualsRequest_Returns1) {
  ImageCacheDownSamplingAtMost s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(200, 200), req(200, 200)));
}

// ---------------------------------------------------------------------------
// AtLeast: picks max ratio (at least fills request), power-of-2 factor
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSamplingAtLeast, NormalDownsample_ReturnsPowerOfTwo) {
  ImageCacheDownSamplingAtLeast s;
  // source 1000×1000, request 100×100 → factor=8
  EXPECT_FLOAT_EQ(8.0f, s.GetScaleFactor(src(1000, 1000), req(100, 100)));
  // source 200×200, request 100×100 → factor=2
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(200, 200), req(100, 100)));
  // source 400×400, request 100×100 → factor=4
  EXPECT_FLOAT_EQ(4.0f, s.GetScaleFactor(src(400, 400), req(100, 100)));
}

TEST(ImageCacheDownSamplingAtLeast, SourceSmallerThanRequest_Returns1) {
  ImageCacheDownSamplingAtLeast s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(200, 200)));
}

TEST(ImageCacheDownSamplingAtLeast, NonPowerOfTwoRatio_RoundsDownToPowerOfTwo) {
  ImageCacheDownSamplingAtLeast s;
  // source 500×500, request 100×100 → scaleFactor=0.2, width=100,
  // GetFactorAsBit(5)=4
  EXPECT_FLOAT_EQ(4.0f, s.GetScaleFactor(src(500, 500), req(100, 100)));
  // source 300×300, request 100×100 → scaleFactor=0.333, width=100,
  // GetFactorAsBit(3)=2
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(300, 300), req(100, 100)));
}

TEST(ImageCacheDownSamplingAtLeast, NonSquareSource_UsesMaxRatio) {
  ImageCacheDownSamplingAtLeast s;
  // source 1000×500, request 100×100 → max(100/1000, 100/500) = 0.2
  // width=round(0.2*1000)=200, height=round(0.2*500)=100
  // GetFactorAsBit(max(1000/200, 500/100)) = GetFactorAsBit(5) = 4
  EXPECT_FLOAT_EQ(4.0f, s.GetScaleFactor(src(1000, 500), req(100, 100)));
}

TEST(ImageCacheDownSamplingAtLeast, ZeroSourceDimension_Returns1) {
  ImageCacheDownSamplingAtLeast s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(0, 100), req(100, 100)));
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 0), req(100, 100)));
}

// ---------------------------------------------------------------------------
// AtMost: picks min ratio (at most fits request), ceil + power-of-2
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSamplingAtMost, ExactPowerOfTwo_ReturnsExact) {
  ImageCacheDownSamplingAtMost s;
  // source 200×200, request 100×100 → intScaleFactor=2, returns 2
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(200, 200), req(100, 100)));
  // source 400×400, request 100×100 → intScaleFactor=4, returns 4
  EXPECT_FLOAT_EQ(4.0f, s.GetScaleFactor(src(400, 400), req(100, 100)));
}

TEST(ImageCacheDownSamplingAtMost, NonPowerOfTwo_RoundsUpToNextPowerOfTwo) {
  ImageCacheDownSamplingAtMost s;
  // source 500×500, request 100×100 → intScaleFactor=5, GetFactorAsBit(5)=4,
  // 4<5 → 8
  EXPECT_FLOAT_EQ(8.0f, s.GetScaleFactor(src(500, 500), req(100, 100)));
  // source 300×300, request 100×100 → intScaleFactor=3, GetFactorAsBit(3)=2,
  // 2<3 → 4
  EXPECT_FLOAT_EQ(4.0f, s.GetScaleFactor(src(300, 300), req(100, 100)));
}

TEST(ImageCacheDownSamplingAtMost, SourceSmallerThanRequest_Returns1) {
  ImageCacheDownSamplingAtMost s;
  // intScaleFactor = ceil(max(100/200, 100/200)) = ceil(0.5) = 1 → returns 1
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(200, 200)));
}

TEST(ImageCacheDownSamplingAtMost, ZeroRequestDimension_Returns1) {
  ImageCacheDownSamplingAtMost s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(1000, 1000), req(0, 100)));
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(1000, 1000), req(100, 0)));
}

// ---------------------------------------------------------------------------
// CenterInside: min/max depending on factor==1, optional memory mode
// ---------------------------------------------------------------------------

TEST(ImageCacheDownSamplingCenterInside, NoMemory_FactorNot1_ReturnsMaxFactor) {
  ImageCacheDownSamplingCenterInside s(false);
  // source 1000×1000, request 100×100 → factor=0.125, width=125
  // factor!=1 → GetFactorAsBit(min(1000/125, 1000/125)) = GetFactorAsBit(8) = 8
  EXPECT_FLOAT_EQ(8.0f, s.GetScaleFactor(src(1000, 1000), req(100, 100)));
}

TEST(ImageCacheDownSamplingCenterInside, NoMemory_Factor1_ReturnsMaxFactor) {
  ImageCacheDownSamplingCenterInside s(false);
  // source 100×100, request 200×200 → factor=1.0 (maxIntFactor=0)
  // factor==1 → GetFactorAsBit(max(100/100, 100/100)) = GetFactorAsBit(1) = 1
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(200, 200)));
}

TEST(ImageCacheDownSamplingCenterInside, MemoryMode_UpgradesScaleFactor) {
  // Non-square source to trigger memory adjustment
  // source 1000×500, request 100×100 → factor=0.125
  // width=125, height=63 → min(1000/125, 500/63) = min(8, 7) = 7
  // GetFactorAsBit(7) = HighestOneBit(7) = 4
  // memory=true: 4 < (1/0.125)=8 → yes → scaleFactor = 8
  ImageCacheDownSamplingCenterInside memTrue(true);
  ImageCacheDownSamplingCenterInside memFalse(false);
  EXPECT_FLOAT_EQ(8.0f, memTrue.GetScaleFactor(src(1000, 500), req(100, 100)));
  EXPECT_FLOAT_EQ(4.0f, memFalse.GetScaleFactor(src(1000, 500), req(100, 100)));
}

TEST(
    ImageCacheDownSamplingCenterInside,
    MemoryMode_NoUpgradeWhenAlreadyAtThreshold) {
  // Square source: min == max, scaleFactor == 1/factor → no upgrade
  ImageCacheDownSamplingCenterInside s(true);
  float result = s.GetScaleFactor(src(1000, 1000), req(100, 100));
  EXPECT_FLOAT_EQ(8.0f, result);
  EXPECT_GT(s.GetScaleFactor(src(200, 200), req(100, 100)), 0.0f);
}

// ---------------------------------------------------------------------------
// FitCenterMemory: min ratio, upgrades if scaleFactor < 1/percentageFactor
// ---------------------------------------------------------------------------

TEST(
    ImageCacheDownSamplingFitCenterMemory,
    NormalDownsample_UpgradesToNextPowerOfTwo) {
  ImageCacheDownSamplingFitCenterMemory s;
  // source 1000×1000, request 100×100 → percentage=0.1
  // GetFactorAsBit(10)=8, 8 < (1/0.1)=10 → yes → 16
  EXPECT_FLOAT_EQ(16.0f, s.GetScaleFactor(src(1000, 1000), req(100, 100)));
}

TEST(ImageCacheDownSamplingFitCenterMemory, ExactPowerOfTwo_NoUpgrade) {
  ImageCacheDownSamplingFitCenterMemory s;
  // source 200×200, request 100×100 → percentage=0.5
  // GetFactorAsBit(2)=2, 2 < (1/0.5)=2 → no → 2
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(200, 200), req(100, 100)));
}

TEST(ImageCacheDownSamplingFitCenterMemory, SourceSmallerThanRequest_Returns1) {
  ImageCacheDownSamplingFitCenterMemory s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(200, 200)));
}

TEST(ImageCacheDownSamplingFitCenterMemory, ZeroSourceDimension_Returns1) {
  ImageCacheDownSamplingFitCenterMemory s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(0, 100), req(100, 100)));
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 0), req(100, 100)));
}

// ---------------------------------------------------------------------------
// FitCenterQuality: max ratio via GetFactor, no upgrade
// ---------------------------------------------------------------------------

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    NormalDownsample_ReturnsPowerOfTwo) {
  ImageCacheDownSamplingFitCenterQuality s;
  // source 1000×1000, request 100×100 → factor=0.125, width=125
  // GetFactorAsBit(max(1000/125, 1000/125)) = GetFactorAsBit(8) = 8
  EXPECT_FLOAT_EQ(8.0f, s.GetScaleFactor(src(1000, 1000), req(100, 100)));
  // source 200×200, request 100×100 → factor=0.5, width=100
  // GetFactorAsBit(2) = 2
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(200, 200), req(100, 100)));
}

TEST(ImageCacheDownSamplingFitCenterQuality, NonPowerOfTwoRatio_RoundsDown) {
  ImageCacheDownSamplingFitCenterQuality s;
  // source 500×500, request 100×100 → factor=1/HighestOneBit(5)=1/4=0.25
  // width=125 → GetFactorAsBit(max(500/125, 500/125)) = GetFactorAsBit(4) = 4
  EXPECT_FLOAT_EQ(4.0f, s.GetScaleFactor(src(500, 500), req(100, 100)));
}

TEST(
    ImageCacheDownSamplingFitCenterQuality,
    SourceSmallerThanRequest_Returns1) {
  ImageCacheDownSamplingFitCenterQuality s;
  // factor = 1.0 (maxIntFactor=0) → width=100 → GetFactorAsBit(1) = 1
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(100, 100), req(200, 200)));
}

// ---------------------------------------------------------------------------
// Cross-strategy: same input, different outputs (verify strategies differ)
// ---------------------------------------------------------------------------

TEST(
    ImageCacheDownSamplingStrategy,
    SameInput_AllStrategies_ProduceDifferentOrConsistentResults) {
  auto source = src(1000, 500);
  auto request = req(100, 100);

  ImageCacheDownSamplingDefault def;
  ImageCacheDownSamplingAtLeast atLeast;
  ImageCacheDownSamplingAtMost atMost;
  ImageCacheDownSamplingCenterInside centerNoMem(false);
  ImageCacheDownSamplingCenterInside centerMem(true);
  ImageCacheDownSamplingFitCenterMemory fitMem;
  ImageCacheDownSamplingFitCenterQuality fitQual;

  // Default ignores request → returns 1 (source < 8K)
  EXPECT_FLOAT_EQ(1.0f, def.GetScaleFactor(source, request));
  // AtLeast: max(100/1000, 100/500)=0.2, width=200,
  // GetFactorAsBit(max(1000/200, 500/100))=GetFactorAsBit(5)=4
  EXPECT_FLOAT_EQ(4.0f, atLeast.GetScaleFactor(source, request));
  // AtMost: intScaleFactor=ceil(max(1000/100, 500/100))=ceil(10)=10,
  // GetFactorAsBit(10)=8, 8<10 → 16
  EXPECT_FLOAT_EQ(16.0f, atMost.GetScaleFactor(source, request));
  // CenterInside no-mem: factor=0.125, width=125, height=63, min(8,7)=7,
  // GetFactorAsBit(7)=4
  EXPECT_FLOAT_EQ(4.0f, centerNoMem.GetScaleFactor(source, request));
  // CenterInside mem: scaleFactor=4 < 8 → 8
  EXPECT_FLOAT_EQ(8.0f, centerMem.GetScaleFactor(source, request));
  // FitCenterMemory: percentage=min(100/1000, 100/500)=0.1, width=100,
  // height=50 GetFactorAsBit(min(1000/100, 500/50))=GetFactorAsBit(10)=8, 8<10
  // → 16
  EXPECT_FLOAT_EQ(16.0f, fitMem.GetScaleFactor(source, request));
  // FitCenterQuality: factor=0.125, width=125, height=63
  // GetFactorAsBit(max(1000/125,
  // 500/63))=GetFactorAsBit(max(8,7))=GetFactorAsBit(8)=8
  EXPECT_FLOAT_EQ(8.0f, fitQual.GetScaleFactor(source, request));
}

// ---------------------------------------------------------------------------
// Base class direct tests (improves Coverage for ImageCacheDownSamplingBase)
// ---------------------------------------------------------------------------

TEST(
    ImageCacheDownSamplingBase,
    FitCenterQuality_SourceEqualsRequest_Returns1) {
  ImageCacheDownSamplingFitCenterQuality s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(200, 200), req(200, 200)));
}

TEST(ImageCacheDownSamplingBase, FitCenterQuality_SourceSmaller_Returns1) {
  ImageCacheDownSamplingFitCenterQuality s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(50, 50), req(100, 100)));
}

TEST(
    ImageCacheDownSamplingBase,
    FitCenterQuality_VeryLargeSource_ReturnsHighFactor) {
  ImageCacheDownSamplingFitCenterQuality s;
  auto factor = s.GetScaleFactor(src(8000, 8000), req(100, 100));
  EXPECT_GT(factor, 1.0f);
  EXPECT_LE(factor, 128.0f);
}

TEST(ImageCacheDownSamplingBase, Default_SmallSourceUnder8K_Returns1) {
  ImageCacheDownSamplingDefault s;
  EXPECT_FLOAT_EQ(1.0f, s.GetScaleFactor(src(1000, 1000), req(100, 100)));
}

TEST(ImageCacheDownSamplingBase, Default_SourceOver8K_ReturnsHighFactor) {
  ImageCacheDownSamplingDefault s;
  auto factor = s.GetScaleFactor(src(9000, 9000), req(100, 100));
  EXPECT_GT(factor, 1.0f);
}

TEST(ImageCacheDownSamplingBase, AtLeast_HalfSizeRequest_Returns2) {
  ImageCacheDownSamplingAtLeast s;
  EXPECT_FLOAT_EQ(2.0f, s.GetScaleFactor(src(100, 100), req(50, 50)));
}

TEST(ImageCacheDownSamplingBase, AtMost_LargeSource_ReturnsHighFactor) {
  ImageCacheDownSamplingAtMost s;
  auto factor = s.GetScaleFactor(src(4000, 4000), req(100, 100));
  EXPECT_GT(factor, 1.0f);
}

TEST(ImageCacheDownSamplingBase, CenterInside_MemoryStrategy_ReturnsFactor) {
  ImageCacheDownSamplingFitCenterMemory s;
  auto factor = s.GetScaleFactor(src(1000, 500), req(100, 100));
  EXPECT_GE(factor, 1.0f);
}
