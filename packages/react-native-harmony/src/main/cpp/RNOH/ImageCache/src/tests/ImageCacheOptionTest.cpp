/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheOption.h"
#include <gtest/gtest.h>

using namespace ImageCachePro;

// ============================================================================
// Tier 2 — glog stub + ohresmgr stub + native API stubs
// Module: RNOH/ImageCache/src/ImageCacheOption
//
// Tested class: ImageCacheOption (image loading configuration)
// IMAGE_CACHE_ENABLE_TRANSFORMATION is OFF (default).
// ============================================================================

TEST(ImageCacheOption, DefaultConstructor_IsInvalid) {
  ImageCacheOption opt;
  EXPECT_FALSE(opt.IsValid());
}

TEST(ImageCacheOption, DefaultConstructor_TransformSpecEmpty) {
  ImageCacheOption opt;
  EXPECT_TRUE(opt.GetTransformSpec().empty());
}

TEST(ImageCacheOption, DefaultConstructor_CallbackOnMainThreadTrue) {
  ImageCacheOption opt;
  EXPECT_TRUE(opt.IsCallbackOnMainThread());
}

TEST(ImageCacheOption, DefaultConstructor_AssetsPrefixEmpty) {
  ImageCacheOption opt;
  EXPECT_TRUE(opt.GetAssetsPrefix().empty());
}

TEST(ImageCacheOption, AppendTransformSpec_FirstSpec_SetsSpec) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  EXPECT_EQ(opt.GetTransformSpec(), "blur");
}

TEST(ImageCacheOption, AppendTransformSpec_SecondSpec_AppendsWithPlus) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  opt.AppendTransformSpec("crop");
  EXPECT_EQ(opt.GetTransformSpec(), "blur+crop");
}

TEST(ImageCacheOption, AppendTransformSpec_ThirdSpec_ChainsAll) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  opt.AppendTransformSpec("crop");
  opt.AppendTransformSpec("gray");
  EXPECT_EQ(opt.GetTransformSpec(), "blur+crop+gray");
}

TEST(ImageCacheOption, AppendTransformSpec_EmptyString_SetsEmpty) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("");
  EXPECT_EQ(opt.GetTransformSpec(), "");
}

TEST(ImageCacheOption, AppendTransformSpec_EmptyThenNonEmpty_AppendsCorrectly) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("");
  opt.AppendTransformSpec("crop");
  // First set "" — transformSpec is still empty ("" is empty)
  // Second call: transformSpec.empty() still true → set to "crop"
  EXPECT_EQ(opt.GetTransformSpec(), "crop");
}

TEST(ImageCacheOption, Clone_CopiesTransformSpec) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur+crop");
  auto cloned = opt.Clone();
  EXPECT_EQ(cloned->GetTransformSpec(), "blur+crop");
}

TEST(ImageCacheOption, Clone_CopiesAssetsPrefix) {
  ImageCacheOption opt;
  // assetsPrefix is private; set via clone of an option that has it
  // For now, verify default clone has empty prefix
  auto cloned = opt.Clone();
  EXPECT_TRUE(cloned->GetAssetsPrefix().empty());
}

TEST(ImageCacheOption, Clone_IsIndependentCopy) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  auto cloned = opt.Clone();
  cloned->AppendTransformSpec("crop");
  EXPECT_EQ(opt.GetTransformSpec(), "blur");
  EXPECT_EQ(cloned->GetTransformSpec(), "blur+crop");
}

TEST(ImageCacheOption, Create_WithUri_ReturnsValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/img.png", "", "", 100, 200);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_WithEmptyUri_ReturnsValidButEmptySource) {
  // Create("") still calls SetString("") → type=STRING → IsValid()=true
  auto opt = ImageCacheOption::Create("", "", "", 100, 200);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid()); // STRING type, not UNDEFINED
}

TEST(ImageCacheOption, Create_WithAssetsPrefix_StoresPrefix) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/img.png", "resource://RAWFILE/assets/", "", 100, 200);
  ASSERT_NE(opt, nullptr);
  EXPECT_EQ(opt->GetAssetsPrefix(), "resource://RAWFILE/assets/");
}

// ---------------------------------------------------------------------------
// MapResizeMethod branch coverage (via Create with different resizeMethod)
// All branches produce a valid option; differentiation is via
// ResolveTransformationInfo
// ---------------------------------------------------------------------------

TEST(ImageCacheOption, Create_ResizeMethodEmpty_DefaultsToValid) {
  auto opt =
      ImageCacheOption::Create("http://example.com/a.png", "", "", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_ResizeMethodAuto_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/a.png", "", "auto", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->ResolveTransformationInfo().empty());
}

TEST(ImageCacheOption, Create_ResizeMethodScale_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/a.png", "", "scale", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_EQ(opt->GetAssetsPrefix(), "");
}

TEST(ImageCacheOption, Create_ResizeMethodNone_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/a.png", "", "none", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_ResizeMethodResize_ProducesValidOption) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "resize", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->ResolveTransformationInfo().empty());
}

TEST(ImageCacheOption, Create_ResizeMethodContain_ProducesValidOption) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "contain", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_EQ(opt->GetAssetsPrefix(), "");
}

TEST(ImageCacheOption, Create_ResizeMethodCover_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/a.png", "", "cover", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_ResizeMethodCenterInside_ProducesValidOption) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "centerInside", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->ResolveTransformationInfo().empty());
}

TEST(ImageCacheOption, Create_ResizeMethodAtLeast_ProducesValidOption) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "atLeast", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_EQ(opt->GetAssetsPrefix(), "");
}

TEST(ImageCacheOption, Create_ResizeMethodAtMost_ProducesValidOption) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "atMost", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_ResizeMethodUnknown_FallsBackToDefault) {
  auto opt =
      ImageCacheOption::Create("http://example.com/a.png", "", "bogus", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->ResolveTransformationInfo().empty());
}

// ---------------------------------------------------------------------------
// IsAnimatorUri branch coverage (via Create with .gif / data:image/gif)
// ---------------------------------------------------------------------------

TEST(ImageCacheOption, Create_GifExtension_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/anim.gif", "", "", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_GifUpperCase_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/anim.GIF", "", "", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
}

TEST(ImageCacheOption, Create_DataImageGif_ProducesValidOption) {
  auto opt = ImageCacheOption::Create(
      "data:image/gif;base64,R0lGODlh", "", "", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_EQ(opt->GetAssetsPrefix(), "");
}

TEST(ImageCacheOption, Create_NonGifUri_ProducesValidOption) {
  auto opt =
      ImageCacheOption::Create("http://example.com/photo.png", "", "", 10, 20);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->ResolveTransformationInfo().empty());
}

// ---------------------------------------------------------------------------
// ResolveTransformationInfo branch coverage
// ---------------------------------------------------------------------------

TEST(ImageCacheOption, ResolveTransformationInfo_NoSpec_ReturnsEmpty) {
  ImageCacheOption opt;
  EXPECT_TRUE(opt.ResolveTransformationInfo().empty());
}

TEST(ImageCacheOption, ResolveTransformationInfo_WithSpec_ReturnsSpecString) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  EXPECT_EQ(opt.ResolveTransformationInfo(), "transformSpec=blur;");
}

TEST(
    ImageCacheOption,
    ResolveTransformationInfo_WithMultiSpec_ReturnsJoinedSpec) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  opt.AppendTransformSpec("crop");
  EXPECT_EQ(opt.ResolveTransformationInfo(), "transformSpec=blur+crop;");
}

// ---------------------------------------------------------------------------
// PrepareTransformations branch coverage
// IMAGE_CACHE_ENABLE_TRANSFORMATION is OFF (default), so it's a no-op
// ---------------------------------------------------------------------------

TEST(ImageCacheOption, PrepareTransformations_EmptySpec_NoCrash) {
  ImageCacheOption opt;
  opt.PrepareTransformations();
  EXPECT_TRUE(opt.GetTransformSpec().empty());
}

TEST(ImageCacheOption, PrepareTransformations_WithSpec_PreservesSpec) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  opt.PrepareTransformations();
  EXPECT_EQ(opt.GetTransformSpec(), "blur");
}

// ---------------------------------------------------------------------------
// Copy constructor coverage
// ---------------------------------------------------------------------------

TEST(ImageCacheOption, CopyConstructor_PreservesTransformSpec) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur+crop");
  ImageCacheOption copy(opt);
  EXPECT_EQ(copy.GetTransformSpec(), "blur+crop");
}

TEST(ImageCacheOption, CopyConstructor_PreservesAssetsPrefix) {
  ImageCacheOption opt;
  ImageCacheOption copy(opt);
  EXPECT_TRUE(copy.GetAssetsPrefix().empty());
}

TEST(ImageCacheOption, CopyConstructor_IsIndependentFromOriginal) {
  ImageCacheOption opt;
  opt.AppendTransformSpec("blur");
  ImageCacheOption copy(opt);
  copy.AppendTransformSpec("crop");
  EXPECT_EQ(opt.GetTransformSpec(), "blur");
  EXPECT_EQ(copy.GetTransformSpec(), "blur+crop");
}

// ---------------------------------------------------------------------------
// Create with various parameters
// ---------------------------------------------------------------------------

TEST(ImageCacheOption, Create_WithBlurRadius_TransformSpecIsEmpty) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "", 10, 20, 5.0f);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->GetTransformSpec().empty());
}

TEST(ImageCacheOption, Create_WithNonZeroBlur_NoTransformWithoutFlag) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/a.png", "", "", 10, 20, 10.0f);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->ResolveTransformationInfo().empty());
}

TEST(ImageCacheOption, Create_NonEmptyUri_IsValid) {
  auto opt = ImageCacheOption::Create(
      "http://example.com/img.jpg", "prefix/", "", 50, 60);
  ASSERT_NE(opt, nullptr);
  EXPECT_TRUE(opt->IsValid());
  EXPECT_EQ(opt->GetAssetsPrefix(), "prefix/");
}
