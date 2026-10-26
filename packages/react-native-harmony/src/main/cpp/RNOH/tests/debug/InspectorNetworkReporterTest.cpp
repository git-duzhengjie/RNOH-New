/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <string>

// Reimplement the pure function limitRequestBodySize from
// InspectorNetworkReporter.cpp Keep in sync with the anonymous-namespace
// function in RNOH/InspectorNetworkReporter.cpp This avoids pulling in
// napi/ArkJS/NetworkReporter dependencies.
namespace {
std::string limitRequestBodySize(std::string body) {
  constexpr std::size_t kMaxBodySize = 1024 * 1024;
  auto bodyLength = body.size();
  auto bytesToRead = std::min(bodyLength, kMaxBodySize);

  body.resize(bytesToRead);

  if (bytesToRead < bodyLength) {
    body += "\n... [truncated, showing " + std::to_string(bytesToRead) +
        " of " + std::to_string(bodyLength) + " bytes]";
  }

  return body;
}
} // namespace

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_SmallBody_ReturnsUnchanged) {
  std::string body = "hello world";
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result, "hello world");
}

TEST(InspectorNetworkReporter, LimitRequestBodySize_ExactMax_ReturnsUnchanged) {
  std::string body(1024 * 1024, 'x');
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result.size(), 1024u * 1024u);
  EXPECT_EQ(result, body);
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_ExceedsMax_TruncatesAndAppendsSuffix) {
  std::string body(1024 * 1024 + 500, 'x');
  auto result = limitRequestBodySize(body);
  EXPECT_GT(result.size(), 1024u * 1024u);
  EXPECT_NE(result.find("[truncated"), std::string::npos);
  EXPECT_NE(result.find("1048576 of 1049076 bytes"), std::string::npos);
}

TEST(InspectorNetworkReporter, LimitRequestBodySize_EmptyBody_ReturnsEmpty) {
  std::string body;
  auto result = limitRequestBodySize(body);
  EXPECT_TRUE(result.empty());
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_OneByteBody_ReturnsUnchanged) {
  std::string body = "a";
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result, "a");
}

TEST(InspectorNetworkReporter, LimitRequestBodySize_JustOverMax_Truncates) {
  std::string body(1024 * 1024 + 1, 'x');
  auto result = limitRequestBodySize(body);
  EXPECT_NE(result.find("[truncated"), std::string::npos);
  EXPECT_NE(result.find("1048576 of 1048577 bytes"), std::string::npos);
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_BinaryDataWithNullBytes_PreservesData) {
  std::string body;
  body.push_back('h');
  body.push_back('\0');
  body.push_back('i');
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0], 'h');
  EXPECT_EQ(result[1], '\0');
  EXPECT_EQ(result[2], 'i');
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_JustUnderMax_ReturnsUnchanged) {
  std::string body(1024 * 1024 - 1, 'x');
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result.size(), 1024u * 1024u - 1u);
  EXPECT_EQ(result, body);
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_BodyWithNewlines_PreservesData) {
  std::string body = "line1\nline2\nline3";
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result, "line1\nline2\nline3");
  EXPECT_EQ(result.size(), 17u);
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_OnlyNullBytes_PreservesData) {
  std::string body(5, '\0');
  auto result = limitRequestBodySize(body);
  EXPECT_EQ(result.size(), 5u);
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_EQ(result[i], '\0');
  }
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_VeryLargeBody_TruncatesCorrectly) {
  std::string body(5 * 1024 * 1024, 'x');
  auto result = limitRequestBodySize(body);
  EXPECT_NE(result.find("5242880 bytes"), std::string::npos);
  EXPECT_NE(result.find("1048576 of 5242880 bytes"), std::string::npos);
  EXPECT_EQ(result.substr(0, 5), "xxxxx");
}

TEST(
    InspectorNetworkReporter,
    LimitRequestBodySize_TruncationSuffix_HasCorrectFormat) {
  std::string body(1024 * 1024 + 100, 'y');
  auto result = limitRequestBodySize(body);
  EXPECT_NE(result.find("\n... [truncated, showing "), std::string::npos);
  EXPECT_NE(result.find(" of "), std::string::npos);
  EXPECT_NE(result.find(" bytes]"), std::string::npos);
}
