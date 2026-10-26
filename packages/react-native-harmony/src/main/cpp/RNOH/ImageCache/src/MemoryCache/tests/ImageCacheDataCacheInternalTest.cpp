/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheDataCacheInternal.h"
#include <gtest/gtest.h>

#include <memory>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — pure C++ data structure (header-only)
// Module: RNOH/ImageCache/src/MemoryCache/ImageCacheDataCacheInternal
//
// Tested struct: ImageCacheDataCacheInternal (cache entry wrapper)
// Tests getters: GetImageCacheSize, GetImageData, GetFrameDecodeContext
// ============================================================================

TEST(ImageCacheDataCacheInternal, GetImageCacheSize_ReturnsSize) {
  auto entry =
      std::make_shared<ImageCacheDataCacheInternal>(42, nullptr, nullptr);
  EXPECT_EQ(entry->GetImageCacheSize(), 42u);
}

TEST(ImageCacheDataCacheInternal, GetImageCacheSize_Zero_ReturnsZero) {
  auto entry =
      std::make_shared<ImageCacheDataCacheInternal>(0, nullptr, nullptr);
  EXPECT_EQ(entry->GetImageCacheSize(), 0u);
}

TEST(ImageCacheDataCacheInternal, GetImageData_NullByDefault_ReturnsNull) {
  auto entry =
      std::make_shared<ImageCacheDataCacheInternal>(10, nullptr, nullptr);
  EXPECT_EQ(entry->GetImageData(), nullptr);
}

TEST(ImageCacheDataCacheInternal, GetImageData_WithPixelmap_ReturnsData) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  auto data = std::make_shared<ImageCacheData>(pixelmap);
  auto entry = std::make_shared<ImageCacheDataCacheInternal>(10, data, nullptr);
  EXPECT_EQ(entry->GetImageData(), data);
}

TEST(
    ImageCacheDataCacheInternal,
    GetFrameDecodeContext_NullByDefault_ReturnsNull) {
  auto entry =
      std::make_shared<ImageCacheDataCacheInternal>(10, nullptr, nullptr);
  EXPECT_EQ(entry->GetFrameDecodeContext(), nullptr);
}

TEST(
    ImageCacheDataCacheInternal,
    GetFrameDecodeContext_WithContext_ReturnsContext) {
  auto ctx = std::make_shared<FrameDecodeContext>();
  auto entry = std::make_shared<ImageCacheDataCacheInternal>(10, nullptr, ctx);
  EXPECT_EQ(entry->GetFrameDecodeContext(), ctx);
}

TEST(ImageCacheDataCacheInternal, Constructor_AllFields_StoresCorrectly) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x5678);
  auto data = std::make_shared<ImageCacheData>(pixelmap);
  auto ctx = std::make_shared<FrameDecodeContext>();
  auto entry = std::make_shared<ImageCacheDataCacheInternal>(99, data, ctx);
  EXPECT_EQ(entry->GetImageCacheSize(), 99u);
  EXPECT_EQ(entry->GetImageData(), data);
  EXPECT_EQ(entry->GetFrameDecodeContext(), ctx);
}

TEST(
    ImageCacheDataCacheInternal,
    GetImageCacheSize_LargeValue_ReturnsCorrectly) {
  auto entry = std::make_shared<ImageCacheDataCacheInternal>(
      4ull * 1024 * 1024 * 1024, nullptr, nullptr);
  EXPECT_EQ(entry->GetImageCacheSize(), 4ull * 1024 * 1024 * 1024);
}

TEST(
    ImageCacheDataCacheInternal,
    GetImageData_DifferentPixelmap_ReturnsCorrectData) {
  auto* pm1 = reinterpret_cast<OH_PixelmapNative*>(0x1111);
  auto* pm2 = reinterpret_cast<OH_PixelmapNative*>(0x2222);
  auto data1 = std::make_shared<ImageCacheData>(pm1);
  auto data2 = std::make_shared<ImageCacheData>(pm2);
  auto e1 = std::make_shared<ImageCacheDataCacheInternal>(10, data1, nullptr);
  auto e2 = std::make_shared<ImageCacheDataCacheInternal>(10, data2, nullptr);
  EXPECT_NE(e1->GetImageData(), e2->GetImageData());
}

TEST(
    ImageCacheDataCacheInternal,
    GetFrameDecodeContext_DifferentContexts_NotEqual) {
  auto ctx1 = std::make_shared<FrameDecodeContext>();
  auto ctx2 = std::make_shared<FrameDecodeContext>();
  auto e1 = std::make_shared<ImageCacheDataCacheInternal>(10, nullptr, ctx1);
  auto e2 = std::make_shared<ImageCacheDataCacheInternal>(10, nullptr, ctx2);
  EXPECT_NE(e1->GetFrameDecodeContext(), e2->GetFrameDecodeContext());
}

TEST(ImageCacheDataCacheInternal, Constructor_SizeOne_StoresCorrectly) {
  auto entry =
      std::make_shared<ImageCacheDataCacheInternal>(1, nullptr, nullptr);
  EXPECT_EQ(entry->GetImageCacheSize(), 1u);
  EXPECT_NE(entry->GetImageCacheSize(), 0u);
}
