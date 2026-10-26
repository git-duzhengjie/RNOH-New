/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "JSBigStringHelpers.h"

using namespace rnoh::JSBigStringHelpers;
using facebook::react::JSBigString;

// ============================================================================
// fromBuffer 鈥?branches: try (normal) + catch (exception)
// ============================================================================

TEST(JSBigStringHelpersReal, FromBuffer_SmallBuffer_ReturnsValidString) {
  std::vector<uint8_t> buf = {'h', 'e', 'l', 'l', 'o'};
  auto result = fromBuffer(std::move(buf));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->size(), 5u);
  const auto* data = reinterpret_cast<const uint8_t*>(result->c_str());
  EXPECT_EQ(data[0], 'h');
  EXPECT_EQ(data[1], 'e');
  EXPECT_EQ(data[2], 'l');
  EXPECT_EQ(data[3], 'l');
  EXPECT_EQ(data[4], 'o');
}

TEST(JSBigStringHelpersReal, FromBuffer_EmptyBuffer_ReturnsValidEmptyString) {
  std::vector<uint8_t> buf;
  auto result = fromBuffer(std::move(buf));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->size(), 0u);
}

TEST(JSBigStringHelpersReal, FromBuffer_LargeBuffer_ReturnsValidString) {
  std::vector<uint8_t> buf(10000, 'x');
  auto result = fromBuffer(std::move(buf));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->size(), 10000u);
}

TEST(JSBigStringHelpersReal, FromBuffer_BinaryData_PreservesBytes) {
  std::vector<uint8_t> buf = {0x00, 0xFF, 0x80, 0x7F, 0x01};
  auto result = fromBuffer(std::move(buf));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->size(), 5u);
  const auto* data = reinterpret_cast<const uint8_t*>(result->c_str());
  EXPECT_EQ(data[0], 0x00);
  EXPECT_EQ(data[1], 0xFF);
  EXPECT_EQ(data[2], 0x80);
  EXPECT_EQ(data[3], 0x7F);
  EXPECT_EQ(data[4], 0x01);
}

TEST(JSBigStringHelpersReal, FromBuffer_ExtremelyLargeBuffer_ReturnsValidString) {
  std::vector<uint8_t> buf(100000, 'A');
  auto result = fromBuffer(std::move(buf));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->size(), 100000u);
}

// ============================================================================
// fromRawFilePath 鈥?branch: if (!bundleRawFile) 鈫?true (stub returns nullptr)
// ============================================================================

TEST(JSBigStringHelpersReal, FromRawFilePath_NullManager_ThrowsRuntimeError) {
  g_rawfile_stub_return = nullptr;
  EXPECT_THROW(
      fromRawFilePath("nonexistent.rawfile", nullptr),
      std::runtime_error);
}

TEST(JSBigStringHelpersReal, FromRawFilePath_StubReturnsNull_ThrowsRuntimeError) {
  g_rawfile_stub_return = nullptr;
  EXPECT_THROW(
      fromRawFilePath("bundle.js", reinterpret_cast<NativeResourceManager*>(1)),
      std::runtime_error);
}

TEST(JSBigStringHelpersReal, FromRawFilePath_OpenSucceedsButDescriptorFails_ThrowsRuntimeError) {
  g_rawfile_stub_return = reinterpret_cast<RawFile*>(0xDEAD);
  g_rawfile_desc_stub_success = false;
  EXPECT_THROW(
      fromRawFilePath("bundle.js", reinterpret_cast<NativeResourceManager*>(1)),
      std::runtime_error);
}

// ============================================================================
// fromFilePath 鈥?no branches, delegates to JSBigFileString::fromPath
// ============================================================================

TEST(JSBigStringHelpersReal, FromFilePath_ReturnsNonNull) {
  auto result = fromFilePath("/some/path/bundle.js");
  EXPECT_NE(result, nullptr);
}
