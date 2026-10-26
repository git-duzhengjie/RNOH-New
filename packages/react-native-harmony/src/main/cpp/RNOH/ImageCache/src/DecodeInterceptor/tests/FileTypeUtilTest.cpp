/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include "ImageCacheFileTypeUtil.h"
#include "ImageCacheTaskInternal.h"

#include <memory>
#include <vector>

using namespace ImageCachePro;

// ============================================================================
// Tier 3/4 — 鸿蒙 C API 函数级 Mock (link-time replacement)
// Module: RNOH/ImageCache/src/DecodeInterceptor/ImageCacheFileTypeUtil
//
// Tested method: ParseFromBuffer(task, outInfo)
// Mock C API: OH_ImageSourceInfo_Create/GetWidth/Height/Release,
//             OH_ImageSourceNative_GetImageInfo/GetFrameCount/CreateFromData
//             (return mock width=100, height=200, frameCount=1)
// ============================================================================

class FileTypeUtilTest : public ::testing::Test {
 protected:
  ImageCacheTaskInternal task;
  ImageCacheFileTypeInfo info;

  void SetUp() override {
    info = {};
  }

  // Helper: set task buffer from a byte vector
  void setBuffer(const std::vector<uint8_t>& bytes) {
    auto buf = std::shared_ptr<uint8_t[]>(new uint8_t[bytes.size()]);
    std::copy(bytes.begin(), bytes.end(), buf.get());
    task.product.imageBuffer = buf;
    task.product.imageLength = static_cast<uint32_t>(bytes.size());
  }
};

// ---------------------------------------------------------------------------
// Format detection (pure logic, no C API needed)
// ---------------------------------------------------------------------------

TEST_F(FileTypeUtilTest, ParseFromBuffer_JpgSignature_DetectsJpg) {
  setBuffer({0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::JPG);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_PngSignature_DetectsPng) {
  setBuffer({0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::PNG);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_GifSignature_DetectsGif) {
  setBuffer({0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::GIF);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_BmpSignature_DetectsBmp) {
  setBuffer({0x42, 0x4D, 0x00, 0x00, 0x00, 0x00});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::BMP);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_WebpSignature_DetectsWebp) {
  setBuffer(
      {0x52,
       0x49,
       0x46,
       0x46,
       0x00,
       0x00,
       0x00,
       0x00,
       0x57,
       0x45,
       0x42,
       0x50,
       0x00,
       0x00});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::WEBP);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_SvgXmlSignature_DetectsSvg) {
  const char* svg = "<?xml version=\"1.0\"?><svg></svg>";
  setBuffer(std::vector<uint8_t>(svg, svg + 20));
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::SVG);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_SvgTagSignature_DetectsSvg) {
  const char* svg = "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>";
  setBuffer(std::vector<uint8_t>(svg, svg + 20));
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::SVG);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_IcoSignature_DetectsIco) {
  setBuffer({0x00, 0x00, 0x01, 0x00, 0x01, 0x00});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::ICO);
}

// ---------------------------------------------------------------------------
// Error cases
// ---------------------------------------------------------------------------

TEST_F(FileTypeUtilTest, ParseFromBuffer_EmptyBuffer_ReturnsEmptyImageBuffer) {
  task.product.imageBuffer = nullptr;
  task.product.imageLength = 0;
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::EMPTY_IMAGE_BUFFER);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_UnknownFormat_ReturnsUnknownFileType) {
  setBuffer({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::UNKNOWN_FILE_TYPE);
}

// ---------------------------------------------------------------------------
// Image source info (mock C API returns width=100, height=200, frameCount=1)
// ---------------------------------------------------------------------------

TEST_F(FileTypeUtilTest, ParseFromBuffer_Jpg_FillsImageSourceInfo) {
  setBuffer({0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.width, 100u); // from mock OH_ImageSourceInfo_GetWidth
  EXPECT_EQ(info.height, 200u); // from mock OH_ImageSourceInfo_GetHeight
  EXPECT_EQ(info.frameCount, 1u);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_PreExistingSource_UsesExistingSource) {
  setBuffer({0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10});
  // Pre-set source (simulates cached source)
  task.m_sourceNative = reinterpret_cast<OH_ImageSourceNative*>(0xDEAD);
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.width, 100u);
}

// ---------------------------------------------------------------------------
// HEIC/HEIF format detection (ISOBMFF ftyp box + brand scanning)
// ---------------------------------------------------------------------------

TEST_F(FileTypeUtilTest, ParseFromBuffer_HeicFtypBrand_DetectsHeic) {
  // Minimal HEIC: 12 bytes, ftyp box at offset 4, brand "heic" at offset 8
  setBuffer({0x00, 0x00, 0x00, 0x00, 'f', 't', 'y', 'p', 'h', 'e', 'i', 'c'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::HEIC);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_HeifBrand_DetectsHeic) {
  setBuffer({0x00, 0x00, 0x00, 0x00, 'f', 't', 'y', 'p', 'h', 'e', 'i', 'f'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::HEIC);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_Mif1Brand_DetectsHeic) {
  setBuffer({0x00, 0x00, 0x00, 0x00, 'f', 't', 'y', 'p', 'm', 'i', 'f', '1'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::HEIC);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_FtypWithNonHeicBrand_NotHeic) {
  // ftyp box with brand "mp42" — not a HEIC brand
  setBuffer({0x00, 0x00, 0x00, 0x00, 'f', 't', 'y', 'p', 'm', 'p', '4', '2'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::UNKNOWN_FILE_TYPE);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_HeicBrandAtOffset_DetectsHeic) {
  // ftyp box not at offset 4 but at offset 6 (within scan limit of 64)
  setBuffer(
      {0x00,
       0x00,
       0x00,
       0x00,
       0x00,
       0x00,
       'f',
       't',
       'y',
       'p',
       'h',
       'e',
       'i',
       'c'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::HEIC);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_ShortBuffer_NotHeic) {
  // Buffer too short for HEIC detection (< 12 bytes)
  setBuffer({0x00, 0x00, 0x00, 0x00, 'f', 't', 'y', 'p'});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::UNKNOWN_FILE_TYPE);
}

// ---------------------------------------------------------------------------
// Edge cases for buffer handling
// ---------------------------------------------------------------------------

TEST_F(FileTypeUtilTest, ParseFromBuffer_OneByteBuffer_ReturnsUnknown) {
  setBuffer({0xFF});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::UNKNOWN_FILE_TYPE);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_OnlyJpgSignature_DetectsJpg) {
  setBuffer({0xFF, 0xD8});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::JPG);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_ZeroLengthBuffer_ReturnsEmpty) {
  task.product.imageBuffer = std::shared_ptr<uint8_t[]>(new uint8_t[1]);
  task.product.imageLength = 0;
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::EMPTY_IMAGE_BUFFER);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_WebpShortRiff_NotWebp) {
  // RIFF header but too short for full WEBP signature
  setBuffer({0x52, 0x49, 0x46, 0x46, 0x00, 0x00});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::UNKNOWN_FILE_TYPE);
}

TEST_F(FileTypeUtilTest, ParseFromBuffer_PngExactSignature_DetectsPng) {
  setBuffer({0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A});
  auto result = ImageCacheFileTypeUtil::ParseFromBuffer(task, info);
  EXPECT_EQ(result, ImageCacheErrorCode::OK);
  EXPECT_EQ(info.format, ImageCacheFormat::PNG);
}
