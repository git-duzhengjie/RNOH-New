/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "../../DisplayMetricsManager.h"
#include <folly/dynamic.h>
#include <gtest/gtest.h>

using namespace rnoh;

static folly::dynamic makePayload() {
  folly::dynamic d = folly::dynamic::object;
  d["top"] = 10.0;
  d["left"] = 5.0;
  d["width"] = 1080.0;
  d["height"] = 2400.0;
  d["decorHeight"] = 20.0;
  d["scale"] = 3.0;
  d["fontScale"] = 1.2;
  d["densityDpi"] = 480.0;
  d["xDpi"] = 440.0;
  d["yDpi"] = 440.0;
  return d;
}

TEST(PhysicalPixels, FromDynamic_CompletePayload_AllFieldsCorrect) {
  auto px = PhysicalPixels::fromDynamic(makePayload());
  EXPECT_DOUBLE_EQ(px.top, 10.0);
  EXPECT_DOUBLE_EQ(px.left, 5.0);
  EXPECT_DOUBLE_EQ(px.width, 1080.0);
  EXPECT_DOUBLE_EQ(px.height, 2400.0);
  EXPECT_DOUBLE_EQ(px.decorHeight, 20.0);
  EXPECT_DOUBLE_EQ(px.scale, 3.0);
  EXPECT_DOUBLE_EQ(px.fontScale, 1.2);
  EXPECT_DOUBLE_EQ(px.densityDpi, 480.0);
  EXPECT_DOUBLE_EQ(px.xDpi, 440.0);
  EXPECT_DOUBLE_EQ(px.yDpi, 440.0);
}

TEST(PhysicalPixels, FromDynamic_EmptyPayload_ThrowsForWidth) {
  folly::dynamic payload = folly::dynamic::object;
  try {
    PhysicalPixels::fromDynamic(payload);
    FAIL() << "Expected exception for missing width";
  } catch (const std::exception& e) {
    EXPECT_TRUE(std::string(e.what()).find("width") != std::string::npos);
  }
}

TEST(PhysicalPixels, FromDynamic_TopMissing_DefaultsToZero) {
  folly::dynamic payload = folly::dynamic::object;
  payload["left"] = 5.0;
  payload["width"] = 1080.0;
  payload["height"] = 2400.0;
  payload["decorHeight"] = 20.0;
  payload["scale"] = 3.0;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.top, 0.0);
}

TEST(PhysicalPixels, FromDynamic_LeftMissing_DefaultsToZero) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 10.0;
  payload["width"] = 1080.0;
  payload["height"] = 2400.0;
  payload["decorHeight"] = 20.0;
  payload["scale"] = 3.0;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.left, 0.0);
}

TEST(PhysicalPixels, FromDynamic_DecorHeightMissing_DefaultsToZero) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 10.0;
  payload["left"] = 5.0;
  payload["width"] = 1080.0;
  payload["height"] = 2400.0;
  payload["scale"] = 3.0;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.decorHeight, 0.0);
}

TEST(PhysicalPixels, FromDynamic_WidthMissing_ThrowsException) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 10.0;
  payload["left"] = 5.0;
  payload["height"] = 2400.0;
  payload["decorHeight"] = 20.0;
  payload["scale"] = 3.0;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  try {
    PhysicalPixels::fromDynamic(payload);
    FAIL() << "Expected exception for missing width";
  } catch (const std::exception& e) {
    EXPECT_TRUE(std::string(e.what()).find("width") != std::string::npos);
    EXPECT_EQ(payload.count("height"), 1);
  }
}

TEST(PhysicalPixels, FromDynamic_IntValue_FallsBackToDefault) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 10.0;
  payload["left"] = 5.0;
  payload["width"] = 1080;
  payload["height"] = 2400.0;
  payload["decorHeight"] = 20.0;
  payload["scale"] = 3;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.width, 0.0);
  EXPECT_DOUBLE_EQ(px.scale, 1.0);
}

TEST(PhysicalPixels, FromDynamic_StringValue_FallsBackToDefault) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 10.0;
  payload["left"] = 5.0;
  payload["width"] = "1080.0";
  payload["height"] = 2400.0;
  payload["decorHeight"] = 20.0;
  payload["scale"] = 3.0;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.width, 0.0);
  EXPECT_DOUBLE_EQ(px.height, 2400.0);
}

TEST(PhysicalPixels, FromDynamic_OnlyScaleAndFontScale_ThrowsForWidth) {
  folly::dynamic payload = folly::dynamic::object;
  payload["scale"] = 2.5;
  payload["fontScale"] = 1.5;
  try {
    PhysicalPixels::fromDynamic(payload);
    FAIL()
        << "Expected exception: only scale/fontScale provided, width missing";
  } catch (const std::exception& e) {
    EXPECT_TRUE(std::string(e.what()).find("width") != std::string::npos);
    EXPECT_EQ(payload.count("scale"), 1);
  }
}

TEST(PhysicalPixels, FromDynamic_ZeroScale_ThrowsForWidth) {
  folly::dynamic payload = folly::dynamic::object;
  payload["scale"] = 0.0;
  try {
    PhysicalPixels::fromDynamic(payload);
    FAIL() << "Expected exception: zero scale, width still missing";
  } catch (const std::exception& e) {
    EXPECT_TRUE(std::string(e.what()).find("width") != std::string::npos);
    EXPECT_EQ(payload.count("scale"), 1);
  }
}

TEST(PhysicalPixels, FromDynamic_NegativeDecorHeight_KeepsNegative) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 10.0;
  payload["left"] = 5.0;
  payload["width"] = 1080.0;
  payload["height"] = 2400.0;
  payload["decorHeight"] = -1.0;
  payload["scale"] = 3.0;
  payload["fontScale"] = 1.2;
  payload["densityDpi"] = 480.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.decorHeight, -1.0);
}

TEST(PhysicalPixels, FromDynamic_PartialFields_MixedDefaults) {
  folly::dynamic payload = folly::dynamic::object;
  payload["top"] = 0.0;
  payload["left"] = 0.0;
  payload["width"] = 720.0;
  payload["height"] = 1280.0;
  payload["decorHeight"] = 0.0;
  payload["scale"] = 1.0;
  payload["fontScale"] = 1.0;
  payload["densityDpi"] = 320.0;
  payload["xDpi"] = 440.0;
  payload["yDpi"] = 440.0;
  auto px = PhysicalPixels::fromDynamic(payload);
  EXPECT_DOUBLE_EQ(px.width, 720.0);
  EXPECT_DOUBLE_EQ(px.height, 1280.0);
  EXPECT_DOUBLE_EQ(px.densityDpi, 320.0);
  EXPECT_DOUBLE_EQ(px.top, 0.0);
  EXPECT_DOUBLE_EQ(px.scale, 1.0);
  EXPECT_DOUBLE_EQ(px.fontScale, 1.0);
  EXPECT_DOUBLE_EQ(px.xDpi, 440.0);
  EXPECT_DOUBLE_EQ(px.yDpi, 440.0);
}
