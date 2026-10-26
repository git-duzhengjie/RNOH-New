/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheUriUtil.h"
#include <gtest/gtest.h>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — zero platform dependencies (only <string>)
// Module: RNOH/ImageCache/src/utils/ImageCacheUriUtil
//
// Tested class: ImageCacheUriUtil (static URI classifier)
// Public methods under test:
//   - static ImageCacheRnohSourceKind ClassifyUri(const std::string&)
//   - static bool IsRemoteUri(const std::string&)
//   - static bool IsLocalUri(const std::string&)
// ============================================================================

// ---------------------------------------------------------------------------
// ClassifyUri
// ---------------------------------------------------------------------------

TEST(ImageCacheUriUtil, ClassifyUri_EmptyString_ReturnsUnknown) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri(""), ImageCacheRnohSourceKind::Unknown);
}

TEST(ImageCacheUriUtil, ClassifyUri_HttpScheme_ReturnsRemote) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("http://example.com/a.png"),
      ImageCacheRnohSourceKind::Remote);
}

TEST(ImageCacheUriUtil, ClassifyUri_HttpsScheme_ReturnsRemote) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("https://example.com/a.png"),
      ImageCacheRnohSourceKind::Remote);
}

TEST(ImageCacheUriUtil, ClassifyUri_DataUriScheme_ReturnsDataUri) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("data:image/png;base64,iVBOR"),
      ImageCacheRnohSourceKind::DataUri);
}

TEST(ImageCacheUriUtil, ClassifyUri_AssetScheme_ReturnsAsset) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("asset://icon.png"),
      ImageCacheRnohSourceKind::Asset);
}

TEST(ImageCacheUriUtil, ClassifyUri_RawfileScheme_ReturnsRawfile) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("resource://RAWFILE/assets/icon.png"),
      ImageCacheRnohSourceKind::Rawfile);
}

TEST(ImageCacheUriUtil, ClassifyUri_FileScheme_ReturnsFile) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("file:///data/a.png"),
      ImageCacheRnohSourceKind::File);
}

TEST(ImageCacheUriUtil, ClassifyUri_BundleStoragePath_ReturnsBundleStorage) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("/data/storage/el2/a.png"),
      ImageCacheRnohSourceKind::BundleStorage);
}

TEST(ImageCacheUriUtil, ClassifyUri_UnknownScheme_ReturnsUnknown) {
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("ftp://example.com/a.png"),
      ImageCacheRnohSourceKind::Unknown);
}

TEST(ImageCacheUriUtil, ClassifyUri_SchemeInMiddle_ReturnsUnknown) {
  // rfind(prefix, 0) only matches at start; a scheme appearing later must NOT
  // be recognised as that kind.
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("prefix http://example.com"),
      ImageCacheRnohSourceKind::Unknown);
}

TEST(ImageCacheUriUtil, ClassifyUri_UppercaseHttpScheme_ReturnsUnknown) {
  // Classification is case-sensitive: only lowercase "http(s)://" matches.
  EXPECT_EQ(
      ImageCacheUriUtil::ClassifyUri("HTTP://EXAMPLE.COM/a.png"),
      ImageCacheRnohSourceKind::Unknown);
}

// ---------------------------------------------------------------------------
// IsRemoteUri
// ---------------------------------------------------------------------------

TEST(ImageCacheUriUtil, IsRemoteUri_EmptyString_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsRemoteUri(""));
}

TEST(ImageCacheUriUtil, IsRemoteUri_HttpScheme_ReturnsTrue) {
  EXPECT_TRUE(ImageCacheUriUtil::IsRemoteUri("http://example.com"));
}

TEST(ImageCacheUriUtil, IsRemoteUri_HttpsScheme_ReturnsTrue) {
  EXPECT_TRUE(ImageCacheUriUtil::IsRemoteUri("https://example.com"));
}

TEST(ImageCacheUriUtil, IsRemoteUri_UppercaseScheme_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsRemoteUri("HTTPS://EXAMPLE.COM"));
}

TEST(ImageCacheUriUtil, IsRemoteUri_SchemeNotAtStart_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsRemoteUri("xhttp://example.com"));
}

TEST(ImageCacheUriUtil, IsRemoteUri_FtpScheme_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsRemoteUri("ftp://example.com"));
}

// ---------------------------------------------------------------------------
// IsLocalUri
// ---------------------------------------------------------------------------

TEST(ImageCacheUriUtil, IsLocalUri_EmptyString_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsLocalUri(""));
}

TEST(ImageCacheUriUtil, IsLocalUri_RemoteUri_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsLocalUri("https://example.com/a.png"));
}

TEST(ImageCacheUriUtil, IsLocalUri_AssetScheme_ReturnsTrue) {
  EXPECT_TRUE(ImageCacheUriUtil::IsLocalUri("asset://icon.png"));
}

TEST(ImageCacheUriUtil, IsLocalUri_DataUriScheme_ReturnsTrue) {
  EXPECT_TRUE(ImageCacheUriUtil::IsLocalUri("data:image/png;base64,iVBOR"));
}

TEST(ImageCacheUriUtil, IsLocalUri_FileScheme_ReturnsTrue) {
  EXPECT_TRUE(ImageCacheUriUtil::IsLocalUri("file:///data/a.png"));
}

TEST(ImageCacheUriUtil, IsLocalUri_RawfileScheme_ReturnsTrue) {
  EXPECT_TRUE(
      ImageCacheUriUtil::IsLocalUri("resource://RAWFILE/assets/icon.png"));
}

TEST(ImageCacheUriUtil, IsLocalUri_BundleStoragePath_ReturnsTrue) {
  EXPECT_TRUE(ImageCacheUriUtil::IsLocalUri("/data/storage/el2/a.png"));
}

TEST(ImageCacheUriUtil, IsLocalUri_UnknownScheme_ReturnsFalse) {
  EXPECT_FALSE(ImageCacheUriUtil::IsLocalUri("ftp://example.com/a.png"));
}
