/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include "ImageCacheData.h"

#include <memory>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — pure C++ data structures (header-only, no platform calls)
// Module: RNOH/ImageCache/include/ImageCacheData (structs)
//
// Tested structs:
//   - ImageCacheLoadData (None/From/IsNone/IsImageData/IsFrameDecodeData/
//     HasFrameContext/GetImageData/GetImageDataList/GetFrameContext)
//   - ImageCacheLoadResult (Ok/Failure/IsSuccess/GetFrameIndex)
//   - FrameDecodeRequest (IsBatch)
// ============================================================================

// ---------------------------------------------------------------------------
// ImageCacheLoadData
// ---------------------------------------------------------------------------

TEST(ImageCacheLoadData, None_HasTypeNone) {
  auto data = ImageCacheLoadData::None();
  EXPECT_TRUE(data.IsNone());
  EXPECT_FALSE(data.IsImageData());
  EXPECT_FALSE(data.IsFrameDecodeData());
}

TEST(ImageCacheLoadData, None_HasNoFrameContext) {
  auto data = ImageCacheLoadData::None();
  EXPECT_FALSE(data.HasFrameContext());
}

TEST(ImageCacheLoadData, None_GetImageDataReturnsNullptr) {
  auto data = ImageCacheLoadData::None();
  EXPECT_EQ(data.GetImageData(), nullptr);
}

TEST(ImageCacheLoadData, None_GetImageDataListIsEmpty) {
  auto data = ImageCacheLoadData::None();
  EXPECT_TRUE(data.GetImageDataList().empty());
}

TEST(ImageCacheLoadData, None_GetFrameContextReturnsNullptr) {
  auto data = ImageCacheLoadData::None();
  EXPECT_EQ(data.GetFrameContext(), nullptr);
}

TEST(ImageCacheLoadData, From_WithImageDataOnly_IsImageData) {
  auto pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  auto cacheData = std::make_shared<ImageCacheData>(pixelmap);
  auto data = ImageCacheLoadData::From({cacheData});
  EXPECT_TRUE(data.IsImageData());
  EXPECT_FALSE(data.IsNone());
  EXPECT_FALSE(data.IsFrameDecodeData());
}

TEST(ImageCacheLoadData, From_WithImageData_HasNoFrameContext) {
  auto pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  auto cacheData = std::make_shared<ImageCacheData>(pixelmap);
  auto data = ImageCacheLoadData::From({cacheData});
  EXPECT_FALSE(data.HasFrameContext());
}

TEST(ImageCacheLoadData, From_WithImageData_GetImageDataReturnsFirst) {
  auto pixelmap = reinterpret_cast<OH_PixelmapNative*>(0x1234);
  auto cacheData = std::make_shared<ImageCacheData>(pixelmap);
  auto data = ImageCacheLoadData::From({cacheData});
  EXPECT_EQ(data.GetImageData(), cacheData);
}

TEST(ImageCacheLoadData, From_WithMultipleImages_GetImageDataReturnsFirst) {
  auto pm1 = reinterpret_cast<OH_PixelmapNative*>(0x100);
  auto pm2 = reinterpret_cast<OH_PixelmapNative*>(0x200);
  auto d1 = std::make_shared<ImageCacheData>(pm1);
  auto d2 = std::make_shared<ImageCacheData>(pm2);
  auto data = ImageCacheLoadData::From({d1, d2});
  EXPECT_EQ(data.GetImageData(), d1);
  EXPECT_EQ(data.GetImageDataList().size(), 2u);
}

TEST(ImageCacheLoadData, From_WithFrameContext_IsFrameDecodeData) {
  auto ctx = std::make_shared<FrameDecodeContext>();
  auto data = ImageCacheLoadData::From({}, ctx);
  EXPECT_TRUE(data.IsFrameDecodeData());
  EXPECT_FALSE(data.IsImageData());
  EXPECT_FALSE(data.IsNone());
}

TEST(ImageCacheLoadData, From_WithFrameContext_HasFrameContext) {
  auto ctx = std::make_shared<FrameDecodeContext>();
  auto data = ImageCacheLoadData::From({}, ctx);
  EXPECT_TRUE(data.HasFrameContext());
  EXPECT_EQ(data.GetFrameContext(), ctx);
}

TEST(ImageCacheLoadData, From_EmptyDataList_GetImageDataReturnsNullptr) {
  auto data = ImageCacheLoadData::From({});
  EXPECT_EQ(data.GetImageData(), nullptr);
  EXPECT_TRUE(data.GetImageDataList().empty());
}

// ---------------------------------------------------------------------------
// ImageCacheLoadResult
// ---------------------------------------------------------------------------

TEST(ImageCacheLoadResult, Ok_Default_IsSuccess) {
  auto result = ImageCacheLoadResult::Ok();
  EXPECT_TRUE(result.IsSuccess());
}

TEST(ImageCacheLoadResult, Ok_WithFrameIndex_ReturnsFrameIndex) {
  auto result = ImageCacheLoadResult::Ok(5);
  EXPECT_TRUE(result.IsSuccess());
  EXPECT_EQ(result.GetFrameIndex(), 5u);
}

TEST(ImageCacheLoadResult, Ok_FrameIndexZero_ReturnsZero) {
  auto result = ImageCacheLoadResult::Ok(0);
  EXPECT_EQ(result.GetFrameIndex(), 0u);
}

TEST(ImageCacheLoadResult, Failure_IsNotSuccess) {
  auto result =
      ImageCacheLoadResult::Failure(ImageCacheErrorCode::DECODE_FAILED);
  EXPECT_FALSE(result.IsSuccess());
}

TEST(ImageCacheLoadResult, Failure_WithEmptyBuffer_ReturnsCorrectCode) {
  auto result =
      ImageCacheLoadResult::Failure(ImageCacheErrorCode::EMPTY_IMAGE_BUFFER);
  EXPECT_FALSE(result.IsSuccess());
}

TEST(ImageCacheLoadResult, Failure_DefaultFrameIndex_IsZero) {
  auto result =
      ImageCacheLoadResult::Failure(ImageCacheErrorCode::DECODE_FAILED);
  EXPECT_EQ(result.GetFrameIndex(), 0u);
}

// ---------------------------------------------------------------------------
// FrameDecodeRequest
// ---------------------------------------------------------------------------

TEST(FrameDecodeRequest, DefaultConstructor_IsNotBatch) {
  FrameDecodeRequest req;
  EXPECT_FALSE(req.IsBatch());
}

TEST(FrameDecodeRequest, SingleFrame_IsNotBatch) {
  FrameDecodeRequest req;
  req.startFrame = 0;
  req.fetchCount = 1;
  EXPECT_FALSE(req.IsBatch());
}

TEST(FrameDecodeRequest, MultipleFrames_IsBatch) {
  FrameDecodeRequest req;
  req.startFrame = 0;
  req.fetchCount = 5;
  EXPECT_TRUE(req.IsBatch());
}

TEST(FrameDecodeRequest, TwoFrames_IsBatch) {
  FrameDecodeRequest req;
  req.fetchCount = 2;
  EXPECT_TRUE(req.IsBatch());
}

// ---------------------------------------------------------------------------
// ImageCacheFileTypeInfo
// ---------------------------------------------------------------------------

TEST(ImageCacheFileTypeInfo, DefaultConstructor_HasUnknownFormat) {
  ImageCacheFileTypeInfo info;
  EXPECT_EQ(info.format, ImageCacheFormat::UNKNOWN);
}

TEST(ImageCacheFileTypeInfo, DefaultConstructor_HasDefaultValues) {
  ImageCacheFileTypeInfo info;
  EXPECT_EQ(info.width, 0u);
  EXPECT_EQ(info.height, 0u);
  EXPECT_EQ(info.frameCount, 1u);
}
