/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheSource.h"
#include <gtest/gtest.h>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — only <string> + ImageCacheType.h (ohresmgr stub)
// Module: RNOH/ImageCache/src/ImageCacheSource
//
// Tested class: ImageCacheSource (image source descriptor: string URL or
// resource)
// ============================================================================

TEST(ImageCacheSource, DefaultConstructor_IsEmpty_ReturnsUndefined) {
  ImageCacheSource source;
  EXPECT_TRUE(source.IsEmpty());
  EXPECT_EQ(source.GetType(), ImageCacheDataType::UNDEFINED);
}

TEST(ImageCacheSource, SetString_StoresUrl_TypeIsString) {
  ImageCacheSource source;
  source.SetString("http://example.com/img.png");
  EXPECT_FALSE(source.IsEmpty());
  EXPECT_EQ(source.GetType(), ImageCacheDataType::STRING);
}

TEST(ImageCacheSource, SetString_GetString_ReturnsUrl) {
  ImageCacheSource source;
  source.SetString("http://example.com/img.png");
  std::string url;
  EXPECT_TRUE(source.GetString(url));
  EXPECT_EQ(url, "http://example.com/img.png");
}

TEST(ImageCacheSource, SetString_GetUrl_ReturnsUrl) {
  ImageCacheSource source;
  source.SetString("asset://icon.png");
  EXPECT_EQ(source.GetUrl(), "asset://icon.png");
}

TEST(ImageCacheSource, SetResource_StoresResource_TypeIsResource) {
  ImageCacheSource source;
  ImageCacheResource res{"com.app", "module", 42, "param"};
  source.SetResource(res);
  EXPECT_EQ(source.GetType(), ImageCacheDataType::RESOURCE);
  EXPECT_FALSE(source.IsEmpty());
}

TEST(ImageCacheSource, SetResource_GetResource_ReturnsResource) {
  ImageCacheSource source;
  ImageCacheResource res{"com.app", "module", 42, "param"};
  source.SetResource(res);
  ImageCacheResource got;
  EXPECT_TRUE(source.GetResource(got));
  EXPECT_EQ(got.bundleName, "com.app");
  EXPECT_EQ(got.moduleName, "module");
  EXPECT_EQ(got.id, 42);
  EXPECT_EQ(got.param, "param");
}

TEST(ImageCacheSource, GetString_WhenUndefined_ReturnsFalse) {
  ImageCacheSource source;
  std::string url;
  EXPECT_FALSE(source.GetString(url));
}

TEST(ImageCacheSource, GetResource_WhenStringType_ReturnsFalse) {
  ImageCacheSource source;
  source.SetString("http://example.com/img.png");
  ImageCacheResource res;
  EXPECT_FALSE(source.GetResource(res));
}

TEST(ImageCacheSource, GetString_WhenResourceType_ReturnsFalse) {
  ImageCacheSource source;
  source.SetResource({"com.app", "module", 1, ""});
  std::string url;
  EXPECT_FALSE(source.GetString(url));
  EXPECT_EQ(source.GetType(), ImageCacheDataType::RESOURCE);
}

TEST(ImageCacheSource, SetUndefined_AfterSetString_BecomesEmptyAndUndefined) {
  ImageCacheSource source;
  source.SetString("http://example.com/img.png");
  source.SetUndefined();
  EXPECT_TRUE(source.IsEmpty());
  EXPECT_EQ(source.GetType(), ImageCacheDataType::UNDEFINED);
  std::string url;
  EXPECT_FALSE(source.GetString(url));
}

TEST(ImageCacheSource, SetString_OverwritesPreviousString) {
  ImageCacheSource source;
  source.SetString("http://old.png");
  source.SetString("http://new.png");
  std::string url;
  EXPECT_TRUE(source.GetString(url));
  EXPECT_EQ(url, "http://new.png");
}

TEST(ImageCacheSource, SetResource_OverwritesString) {
  ImageCacheSource source;
  source.SetString("http://example.com/img.png");
  source.SetResource({"com.app", "module", 1, ""});
  EXPECT_EQ(source.GetType(), ImageCacheDataType::RESOURCE);
  std::string url;
  EXPECT_FALSE(source.GetString(url));
}

TEST(ImageCacheSource, GetUrl_WhenUndefined_ReturnsEmpty) {
  ImageCacheSource source;
  EXPECT_TRUE(source.GetUrl().empty());
}

TEST(ImageCacheSource, GetUrl_WhenResourceType_ReturnsEmpty) {
  ImageCacheSource source;
  source.SetResource({"com.app", "module", 1, ""});
  EXPECT_TRUE(source.GetUrl().empty());
  EXPECT_EQ(source.GetType(), ImageCacheDataType::RESOURCE);
}
