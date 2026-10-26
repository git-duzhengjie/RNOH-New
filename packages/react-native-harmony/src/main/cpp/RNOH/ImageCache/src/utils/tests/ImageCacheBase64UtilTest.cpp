/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheBase64Util.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — zero platform dependencies (only <cstdint>/<string>/<vector>)
// Module: RNOH/ImageCache/src/utils/ImageCacheBase64Util
//
// Tested class: ImageCacheBase64Util (static Base64 decoder)
// Tested method: Decode(const std::string&) -> std::vector<uint8_t>
//
// Test vectors from RFC 4648 §10 (f/fo/foo/foob/fooba/foobar).
// ============================================================================

TEST(ImageCacheBase64Util, Decode_EmptyString_ReturnsEmpty) {
  EXPECT_TRUE(ImageCacheBase64Util::Decode("").empty());
}

TEST(ImageCacheBase64Util, Decode_SingleChar_ReturnsEmpty) {
  // 1 Base64 char = 6 bits < 8 bits, cannot produce a full byte
  EXPECT_TRUE(ImageCacheBase64Util::Decode("Q").empty());
}

TEST(ImageCacheBase64Util, Decode_TwoCharWithPadding_ReturnsSingleByte) {
  // "QQ==" decodes to single byte 0x41 = 'A'
  std::vector<uint8_t> expected = {'A'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("QQ=="), expected);
}

TEST(ImageCacheBase64Util, Decode_RFC4648_f_ReturnsSingleByte) {
  // RFC 4648: "f" -> "Zg=="
  std::vector<uint8_t> expected = {'f'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("Zg=="), expected);
}

TEST(ImageCacheBase64Util, Decode_RFC4648_fo_ReturnsTwoBytes) {
  // RFC 4648: "fo" -> "Zm8="
  std::vector<uint8_t> expected = {'f', 'o'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("Zm8="), expected);
}

TEST(ImageCacheBase64Util, Decode_RFC4648_foo_ReturnsThreeBytes) {
  // RFC 4648: "foo" -> "Zm9v" (no padding needed)
  std::vector<uint8_t> expected = {'f', 'o', 'o'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("Zm9v"), expected);
}

TEST(ImageCacheBase64Util, Decode_RFC4648_foobar_ReturnsSixBytes) {
  // RFC 4648: "foobar" -> "Zm9vYmFy"
  std::vector<uint8_t> expected = {'f', 'o', 'o', 'b', 'a', 'r'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("Zm9vYmFy"), expected);
}

TEST(ImageCacheBase64Util, Decode_NoPadding_ReturnsSameAsWithPadding) {
  // "SGVsbG8" (no '=') should decode same as "SGVsbG8=" -> "Hello"
  std::vector<uint8_t> expected = {'H', 'e', 'l', 'l', 'o'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("SGVsbG8"), expected);
  EXPECT_EQ(ImageCacheBase64Util::Decode("SGVsbG8="), expected);
}

TEST(ImageCacheBase64Util, Decode_StringWithWhitespace_SkipsAndDecodes) {
  // Whitespace chars (\n \r \t space) are skipped, '=' also skipped
  std::vector<uint8_t> expected = {'H', 'e', 'l', 'l', 'o'};
  EXPECT_EQ(ImageCacheBase64Util::Decode("SG\nVs\tb G8="), expected);
}

TEST(ImageCacheBase64Util, Decode_IllegalChar_ReturnsEmpty) {
  // '@' is not in Base64 alphabet (A-Za-z0-9+/)
  EXPECT_TRUE(ImageCacheBase64Util::Decode("SGVsbG8@").empty());
}

TEST(ImageCacheBase64Util, Decode_AllPaddingChars_ReturnsEmpty) {
  // "====" -> all chars skipped -> empty output
  EXPECT_TRUE(ImageCacheBase64Util::Decode("====").empty());
}
