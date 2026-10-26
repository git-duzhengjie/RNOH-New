/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheData.h"
#include <gtest/gtest.h>

#include <memory>

using namespace ImageCachePro;

// ============================================================================
// Tier 3/4 — 鸿蒙 C API 函数级 Mock (OH_PixelmapNative_Release stub)
// Module: RNOH/ImageCache/src/MemoryCache/ImageCacheData
//
// Tested class: ImageCacheData (PixelMap lifecycle manager)
// Mock: OH_PixelmapNative_Release (link-time replacement)
// ============================================================================

TEST(ImageCacheData, Constructor_SinglePixelmap_HasFrameCountOne) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  ImageCacheData data(pixelmap);
  EXPECT_EQ(data.GetFrameCount(), 1u);
  EXPECT_EQ(data.GetPixelmap(), pixelmap);
}

TEST(ImageCacheData, Constructor_NullPixelmap_HasFrameCountZero) {
  ImageCacheData data(nullptr);
  EXPECT_EQ(data.GetFrameCount(), 0u);
  EXPECT_EQ(data.GetPixelmap(), nullptr);
}

TEST(ImageCacheData, GetPixelmapAt_IndexZero_ReturnsFirstPixelmap) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x5678);
  ImageCacheData data(pixelmap);
  EXPECT_EQ(data.GetPixelmapAt(0), pixelmap);
}

TEST(ImageCacheData, GetPixelmapList_ReturnsNonEmpty) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0xABCD);
  ImageCacheData data(pixelmap);
  EXPECT_NE(data.GetPixelmapList(), nullptr);
  EXPECT_NE(data.GetPixelmapList()[0], nullptr);
}

TEST(ImageCacheData, Constructor_AnimatedFrames_StoresAllFrames) {
  // Create animated pixelmap list (3 frames)
  auto pixelmaps = std::make_unique<OH_PixelmapNative*[]>(3);
  pixelmaps[0] = reinterpret_cast<OH_PixelmapNative*>(0x100);
  pixelmaps[1] = reinterpret_cast<OH_PixelmapNative*>(0x200);
  pixelmaps[2] = reinterpret_cast<OH_PixelmapNative*>(0x300);
  auto delays = std::make_unique<int32_t[]>(3);
  delays[0] = 100;
  delays[1] = 200;
  delays[2] = 300;
  ImageCacheData data(std::move(pixelmaps), std::move(delays), 3);
  EXPECT_EQ(data.GetFrameCount(), 3u);
  EXPECT_EQ(data.GetPixelmapAt(0), reinterpret_cast<OH_PixelmapNative*>(0x100));
  EXPECT_EQ(data.GetPixelmapAt(1), reinterpret_cast<OH_PixelmapNative*>(0x200));
  EXPECT_EQ(data.GetPixelmapAt(2), reinterpret_cast<OH_PixelmapNative*>(0x300));
}

TEST(ImageCacheData, GetDelayTimeList_Animated_ReturnsDelays) {
  auto pixelmaps = std::make_unique<OH_PixelmapNative*[]>(2);
  pixelmaps[0] = reinterpret_cast<OH_PixelmapNative*>(0x1);
  pixelmaps[1] = reinterpret_cast<OH_PixelmapNative*>(0x2);
  auto delays = std::make_unique<int32_t[]>(2);
  delays[0] = 50;
  delays[1] = 150;
  ImageCacheData data(std::move(pixelmaps), std::move(delays), 2);
  int32_t* delayList = data.GetDelayTimeList();
  ASSERT_NE(delayList, nullptr);
  EXPECT_EQ(delayList[0], 50);
  EXPECT_EQ(delayList[1], 150);
}

TEST(ImageCacheData, GetPixelmapAt_OutOfBounds_ReturnsNullptr) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  ImageCacheData data(pixelmap);
  EXPECT_EQ(data.GetPixelmapAt(5), nullptr);
}

TEST(ImageCacheData, GetPixelmapAt_OnNullData_ReturnsNullptr) {
  ImageCacheData data(nullptr);
  EXPECT_EQ(data.GetPixelmapAt(0), nullptr);
}

TEST(ImageCacheData, GetPixelmap_OnAnimatedData_ReturnsFirstFrame) {
  auto pixelmaps = std::make_unique<OH_PixelmapNative*[]>(3);
  pixelmaps[0] = reinterpret_cast<OH_PixelmapNative*>(0x100);
  pixelmaps[1] = reinterpret_cast<OH_PixelmapNative*>(0x200);
  pixelmaps[2] = reinterpret_cast<OH_PixelmapNative*>(0x300);
  auto delays = std::make_unique<int32_t[]>(3);
  delays[0] = 100;
  delays[1] = 200;
  delays[2] = 300;
  ImageCacheData data(std::move(pixelmaps), std::move(delays), 3);
  EXPECT_EQ(data.GetPixelmap(), reinterpret_cast<OH_PixelmapNative*>(0x100));
}

TEST(ImageCacheData, GetPixelmapList_OnNullData_ReturnsNull) {
  ImageCacheData data(nullptr);
  EXPECT_EQ(data.GetPixelmapList(), nullptr);
}

TEST(ImageCacheData, GetDelayTimeList_OnSinglePixelmap_ReturnsNull) {
  auto* pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  ImageCacheData data(pixelmap);
  EXPECT_EQ(data.GetDelayTimeList(), nullptr);
}

TEST(ImageCacheData, Constructor_NullPixelmapList_HasFrameCountZero) {
  std::unique_ptr<OH_PixelmapNative*[]> nullList = nullptr;
  auto delays = std::make_unique<int32_t[]>(1);
  delays[0] = 100;
  ImageCacheData data(std::move(nullList), std::move(delays), 0);
  EXPECT_EQ(data.GetFrameCount(), 0u);
}
