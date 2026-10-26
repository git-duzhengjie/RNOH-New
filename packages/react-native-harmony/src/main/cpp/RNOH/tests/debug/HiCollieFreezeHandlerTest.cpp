/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

// Reimplement pure functions from HiCollieFreezeHandler.cpp (anonymous
// namespace). The .cpp includes the instance internal header which pulls in the full
// RNOH upstream-core dependency chain, making it impractical to include directly.
// Keep in sync with RNOH/HiCollieFreezeHandler.cpp.
namespace {
constexpr int OH_THREAD_BLOCK_3S = 0;
constexpr int OH_THREAD_BLOCK_6S = 1;

bool isUiThreadFreezeType(int type) {
  return type == OH_THREAD_BLOCK_3S || type == OH_THREAD_BLOCK_6S;
}

std::string fitStackToLimit(
    std::string_view stack,
    size_t maxBytes,
    std::string_view truncatedSuffix) {
  if (maxBytes == 0) {
    return "";
  }
  if (stack.size() <= maxBytes) {
    return std::string(stack);
  }
  const auto suffixSize = truncatedSuffix.size();
  const auto prefixSize =
      maxBytes > suffixSize ? maxBytes - suffixSize : static_cast<size_t>(0);
  std::string fitted(stack.substr(0, prefixSize));
  if (suffixSize > 0) {
    fitted.append(truncatedSuffix.data(), suffixSize);
  }
  return fitted;
}
} // namespace

TEST(HiCollieFreezeHandler, IsUiThreadFreezeType_3S_ReturnsTrue) {
  EXPECT_TRUE(isUiThreadFreezeType(0));
}

TEST(HiCollieFreezeHandler, IsUiThreadFreezeType_6S_ReturnsTrue) {
  EXPECT_TRUE(isUiThreadFreezeType(1));
}

TEST(HiCollieFreezeHandler, IsUiThreadFreezeType_OtherValue_ReturnsFalse) {
  EXPECT_FALSE(isUiThreadFreezeType(2));
  EXPECT_FALSE(isUiThreadFreezeType(-1));
  EXPECT_FALSE(isUiThreadFreezeType(42));
  EXPECT_FALSE(isUiThreadFreezeType(100));
}

TEST(HiCollieFreezeHandler, FitStackToLimit_SmallStack_ReturnsUnchanged) {
  std::string stack = "hello";
  auto result = fitStackToLimit(stack, 1024, "\n...[truncated]\n");
  EXPECT_EQ(result, "hello");
}

TEST(HiCollieFreezeHandler, FitStackToLimit_ExactLimit_ReturnsUnchanged) {
  std::string stack(64, 'x');
  auto result = fitStackToLimit(stack, 64, "\n...[truncated]\n");
  EXPECT_EQ(result.size(), 64u);
  EXPECT_EQ(result, stack);
}

TEST(
    HiCollieFreezeHandler,
    FitStackToLimit_ExceedsLimit_TruncatesAndAppendsSuffix) {
  std::string stack(128, 'x');
  std::string_view suffix = "\n...[truncated]\n";
  auto result = fitStackToLimit(stack, 64, suffix);
  EXPECT_EQ(result.size(), 64u);
  EXPECT_TRUE(result.find(suffix) != std::string::npos);
  EXPECT_EQ(
      result.substr(0, 64 - suffix.size()),
      std::string(64 - suffix.size(), 'x'));
}

TEST(HiCollieFreezeHandler, FitStackToLimit_ZeroMaxBytes_ReturnsEmpty) {
  std::string stack = "hello";
  auto result = fitStackToLimit(stack, 0, "\n...[truncated]\n");
  EXPECT_TRUE(result.empty());
}

TEST(HiCollieFreezeHandler, FitStackToLimit_EmptyStack_ReturnsEmpty) {
  std::string stack;
  auto result = fitStackToLimit(stack, 1024, "\n...[truncated]\n");
  EXPECT_EQ(result.size(), 0u);
}

TEST(
    HiCollieFreezeHandler,
    FitStackToLimit_EmptySuffix_TruncatesWithoutSuffix) {
  std::string stack(128, 'x');
  auto result = fitStackToLimit(stack, 64, "");
  EXPECT_EQ(result.size(), 64u);
  EXPECT_EQ(result, std::string(64, 'x'));
}

TEST(
    HiCollieFreezeHandler,
    FitStackToLimit_SuffixLargerThanLimit_ReturnsFullSuffix) {
  std::string stack(128, 'x');
  std::string longSuffixStr(80, 'S');
  std::string_view longSuffix(longSuffixStr);
  // When suffix > maxBytes, prefixSize=0, so result = suffix (can exceed
  // maxBytes)
  auto result = fitStackToLimit(stack, 64, longSuffix);
  EXPECT_EQ(result, std::string(longSuffix));
}
