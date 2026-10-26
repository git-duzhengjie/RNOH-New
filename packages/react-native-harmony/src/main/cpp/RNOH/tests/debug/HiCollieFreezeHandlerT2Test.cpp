/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Reimplement T2 logic from HiCollieFreezeHandler.cpp (anonymous namespace).
// T1 already tested isUiThreadFreezeType + fitStackToLimit.
// T2 tests: getReusableFreezeStack (time window), collectStacksFromInstances
// (composition), freezeLogCallback (protocol dispatch + buffer filling).
// Keep in sync with RNOH/HiCollieFreezeHandler.cpp.

namespace {

constexpr int OH_THREAD_BLOCK_3S = 0;
constexpr int OH_THREAD_BLOCK_6S = 1;
constexpr size_t MAX_FREEZE_STACK_BYTES = 64 * 1024;
constexpr std::string_view FREEZE_STACK_TRUNCATED_SUFFIX =
    "\n...[truncated by RNOH freeze handler]\n";
constexpr auto LAST_FREEZE_STACK_REUSE_WINDOW = std::chrono::seconds(10);

bool isUiThreadFreezeType(int type) {
  return type == OH_THREAD_BLOCK_3S || type == OH_THREAD_BLOCK_6S;
}

std::string fitStackToLimit(
    std::string_view stack,
    size_t maxBytes,
    std::string_view truncatedSuffix) {
  if (maxBytes == 0)
    return "";
  if (stack.size() <= maxBytes)
    return std::string(stack);
  const auto suffixSize = truncatedSuffix.size();
  const auto prefixSize =
      maxBytes > suffixSize ? maxBytes - suffixSize : static_cast<size_t>(0);
  std::string fitted(stack.substr(0, prefixSize));
  if (suffixSize > 0)
    fitted.append(truncatedSuffix.data(), suffixSize);
  return fitted;
}

// Fake instance for collectStacksFromInstances
struct FakeInstance {
  size_t id;
  std::string jsStack;
  bool isValid() const {
    return !jsStack.empty();
  }
};

// getReusableFreezeStack: time window reuse logic
std::string getReusableFreezeStack(
    const std::string& lastStack,
    std::optional<std::chrono::steady_clock::time_point> capturedAt,
    std::chrono::steady_clock::time_point now) {
  if (lastStack.empty())
    return "";
  if (!capturedAt.has_value())
    return "";
  if (now - *capturedAt > LAST_FREEZE_STACK_REUSE_WINDOW)
    return "";
  return lastStack;
}

// collectStacksFromInstances: composition logic
std::string collectStacksFromInstances(
    const std::vector<FakeInstance>& instances) {
  std::vector<std::pair<size_t, std::string>> capturedStacks;
  capturedStacks.reserve(instances.size());

  for (const auto& instance : instances) {
    if (!instance.isValid())
      continue;
    if (instance.jsStack.empty())
      continue;
    capturedStacks.emplace_back(instance.id, instance.jsStack);
  }

  if (capturedStacks.empty())
    return "";
  if (capturedStacks.size() == 1)
    return std::move(capturedStacks.front().second);

  std::string combinedStack;
  for (const auto& [rnInstanceId, stack] : capturedStacks) {
    if (!combinedStack.empty() && combinedStack.back() != '\n')
      combinedStack.push_back('\n');
    combinedStack.append("[RNInstance ");
    combinedStack.append(std::to_string(rnInstanceId));
    combinedStack.append("]\n");
    combinedStack.append(stack);
  }
  return combinedStack;
}

// freezeLogCallback: protocol dispatch + buffer filling
// Returns bytes written. Injects reusableStack + instances for testability.
size_t freezeLogCallback(
    int type,
    char* buffer,
    size_t size,
    const std::string& reusableStack,
    const std::vector<FakeInstance>& instances) {
  if (buffer == nullptr || size == 0 || !isUiThreadFreezeType(type))
    return 0;

  std::string stackRecord;
  if (type == OH_THREAD_BLOCK_6S) {
    stackRecord = reusableStack;
  }

  if (stackRecord.empty()) {
    stackRecord = collectStacksFromInstances(instances);
  }

  auto fittedStackRecord = fitStackToLimit(
      stackRecord, MAX_FREEZE_STACK_BYTES, FREEZE_STACK_TRUNCATED_SUFFIX);
  const auto bytesToWrite = std::min(fittedStackRecord.size(), size);

  if (bytesToWrite > 0) {
    std::memcpy(buffer, fittedStackRecord.data(), bytesToWrite);
  }
  if (bytesToWrite < size) {
    buffer[bytesToWrite] = '\0';
  }

  return bytesToWrite;
}

} // namespace

// === getReusableFreezeStack ===

TEST(HiCollieFreezeHandlerT2, ReuseStack_WithinWindow_ReturnsStack) {
  auto now = std::chrono::steady_clock::now();
  auto captured = now - std::chrono::seconds(5);
  EXPECT_EQ(getReusableFreezeStack("stack", captured, now), "stack");
}

TEST(HiCollieFreezeHandlerT2, ReuseStack_ExactlyAtWindow_ReturnsStack) {
  auto now = std::chrono::steady_clock::now();
  auto captured = now - std::chrono::seconds(10);
  EXPECT_EQ(getReusableFreezeStack("stack", captured, now), "stack");
}

TEST(HiCollieFreezeHandlerT2, ReuseStack_BeyondWindow_ReturnsEmpty) {
  auto now = std::chrono::steady_clock::now();
  auto captured = now - std::chrono::seconds(11);
  EXPECT_EQ(getReusableFreezeStack("stack", captured, now), "");
}

TEST(HiCollieFreezeHandlerT2, ReuseStack_EmptyLastStack_ReturnsEmpty) {
  auto now = std::chrono::steady_clock::now();
  EXPECT_EQ(getReusableFreezeStack("", now - std::chrono::seconds(1), now), "");
}

TEST(HiCollieFreezeHandlerT2, ReuseStack_NoCaptureTime_ReturnsEmpty) {
  auto now = std::chrono::steady_clock::now();
  EXPECT_EQ(getReusableFreezeStack("stack", std::nullopt, now), "");
}

// === collectStacksFromInstances ===

TEST(HiCollieFreezeHandlerT2, Collect_NoInstances_ReturnsEmpty) {
  EXPECT_TRUE(collectStacksFromInstances({}).empty());
}

TEST(HiCollieFreezeHandlerT2, Collect_AllEmptyStacks_ReturnsEmpty) {
  std::vector<FakeInstance> instances = {
      {1, ""},
      {2, ""},
  };
  EXPECT_TRUE(collectStacksFromInstances(instances).empty());
}

TEST(HiCollieFreezeHandlerT2, Collect_SingleInstance_ReturnsStackDirectly) {
  std::vector<FakeInstance> instances = {{1, "at foo() at bar.js:1"}};
  auto result = collectStacksFromInstances(instances);
  EXPECT_EQ(result, "at foo() at bar.js:1");
}

TEST(HiCollieFreezeHandlerT2, Collect_MultipleInstances_PrefixedWithHeaders) {
  std::vector<FakeInstance> instances = {
      {1, "stack1"},
      {2, "stack2"},
  };
  auto result = collectStacksFromInstances(instances);
  EXPECT_TRUE(result.find("[RNInstance 1]") != std::string::npos);
  EXPECT_TRUE(result.find("[RNInstance 2]") != std::string::npos);
  EXPECT_TRUE(result.find("stack1") != std::string::npos);
  EXPECT_TRUE(result.find("stack2") != std::string::npos);
}

TEST(HiCollieFreezeHandlerT2, Collect_MultipleInstances_NewlineBetweenEntries) {
  std::vector<FakeInstance> instances = {
      {1, "stack1\n"},
      {2, "stack2\n"},
  };
  auto result = collectStacksFromInstances(instances);
  // stack1 ends with \n, so no extra \n needed before [RNInstance 2]
  auto pos1 = result.find("[RNInstance 2]");
  EXPECT_NE(pos1, std::string::npos);
  // Check the character before [RNInstance 2] is \n
  EXPECT_GT(pos1, 0u);
  EXPECT_EQ(result[pos1 - 1], '\n');
}

TEST(
    HiCollieFreezeHandlerT2,
    Collect_MultipleInstances_NoNewline_InsertsNewline) {
  std::vector<FakeInstance> instances = {
      {1, "stack1_no_newline"},
      {2, "stack2"},
  };
  auto result = collectStacksFromInstances(instances);
  // stack1 has no trailing \n, so one is inserted before [RNInstance 2]
  auto pos2 = result.find("[RNInstance 2]");
  EXPECT_NE(pos2, std::string::npos);
  EXPECT_GT(pos2, 0u);
  EXPECT_EQ(result[pos2 - 1], '\n');
}

TEST(HiCollieFreezeHandlerT2, Collect_SkipsInvalidInstances) {
  std::vector<FakeInstance> instances = {
      {1, ""},
      {2, "valid_stack"},
      {3, ""},
  };
  auto result = collectStacksFromInstances(instances);
  EXPECT_TRUE(result.find("valid_stack") != std::string::npos);
  EXPECT_FALSE(result.find("[RNInstance 1]") != std::string::npos);
  EXPECT_FALSE(result.find("[RNInstance 3]") != std::string::npos);
  // Single valid instance → returned directly without header
  EXPECT_EQ(result, "valid_stack");
}

// === freezeLogCallback ===

TEST(HiCollieFreezeHandlerT2, FreezeCallback_NullBuffer_Returns0) {
  std::vector<FakeInstance> instances = {{1, "stack"}};
  EXPECT_EQ(freezeLogCallback(0, nullptr, 1024, "", instances), 0u);
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_ZeroSize_Returns0) {
  char buf[1];
  std::vector<FakeInstance> instances = {{1, "stack"}};
  EXPECT_EQ(freezeLogCallback(0, buf, 0, "", instances), 0u);
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_InvalidType_Returns0) {
  char buf[1024];
  std::vector<FakeInstance> instances = {{1, "stack"}};
  EXPECT_EQ(freezeLogCallback(42, buf, sizeof(buf), "", instances), 0u);
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_3S_CollectsFromInstances) {
  char buf[1024];
  std::vector<FakeInstance> instances = {{1, "at foo() at bar.js:1"}};
  auto written =
      freezeLogCallback(OH_THREAD_BLOCK_3S, buf, sizeof(buf), "", instances);
  EXPECT_GT(written, 0u);
  buf[written] = '\0';
  EXPECT_TRUE(
      std::string(buf, written).find("at foo() at bar.js:1") !=
      std::string::npos);
}

TEST(
    HiCollieFreezeHandlerT2,
    FreezeCallback_6S_WithReusableStack_UsesReusable) {
  char buf[1024];
  std::vector<FakeInstance> instances = {{1, "fresh_stack"}};
  auto written = freezeLogCallback(
      OH_THREAD_BLOCK_6S, buf, sizeof(buf), "reusable_stack", instances);
  EXPECT_GT(written, 0u);
  buf[written] = '\0';
  EXPECT_TRUE(
      std::string(buf, written).find("reusable_stack") != std::string::npos);
  EXPECT_FALSE(
      std::string(buf, written).find("fresh_stack") != std::string::npos);
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_6S_NoReusableStack_CollectsFresh) {
  char buf[1024];
  std::vector<FakeInstance> instances = {{1, "fresh_stack"}};
  auto written =
      freezeLogCallback(OH_THREAD_BLOCK_6S, buf, sizeof(buf), "", instances);
  EXPECT_GT(written, 0u);
  buf[written] = '\0';
  EXPECT_TRUE(
      std::string(buf, written).find("fresh_stack") != std::string::npos);
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_NoInstances_Returns0) {
  char buf[1024];
  auto written =
      freezeLogCallback(OH_THREAD_BLOCK_3S, buf, sizeof(buf), "", {});
  EXPECT_EQ(written, 0u);
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_BufferNullTerminated) {
  char buf[1024];
  std::memset(buf, 'X', sizeof(buf));
  std::vector<FakeInstance> instances = {{1, "short"}};
  auto written =
      freezeLogCallback(OH_THREAD_BLOCK_3S, buf, sizeof(buf), "", instances);
  ASSERT_GT(written, 0u);
  ASSERT_LT(written, sizeof(buf));
  EXPECT_EQ(buf[written], '\0');
}

TEST(HiCollieFreezeHandlerT2, FreezeCallback_StackExceedsBuffer_Truncates) {
  char buf[64];
  std::string longStack(200, 'x');
  std::vector<FakeInstance> instances = {{1, longStack}};
  auto written =
      freezeLogCallback(OH_THREAD_BLOCK_3S, buf, sizeof(buf), "", instances);
  EXPECT_LE(written, sizeof(buf));
}
