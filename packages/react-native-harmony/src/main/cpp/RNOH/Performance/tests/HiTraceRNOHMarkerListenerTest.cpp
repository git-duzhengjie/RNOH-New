/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <functional>
#include <string>

// Reimplement pure functions from HiTraceRNOHMarkerListener.cpp (private
// methods). Keep in sync with RNOH/Performance/HiTraceRNOHMarkerListener.cpp.
namespace {

std::string makeMessage(const std::string& marker, const std::string& tag) {
  std::string message = "ReactMarker::" + marker;
  if (!tag.empty()) {
    message += "::Tag::";
    message += tag;
  }
  return message;
}

int32_t getMessageId(const std::string& message) {
  std::hash<std::string> hasher;
  size_t hash = hasher(message);
  int32_t small_hash = static_cast<int32_t>(hash ^ (hash >> 32));
  return small_hash;
}

} // namespace

TEST(HiTraceRNOHMarkerListener, MakeMessage_NoTag_ProducesReactMarkerPrefix) {
  EXPECT_EQ(makeMessage("RUN_JS_BUNDLE", ""), "ReactMarker::RUN_JS_BUNDLE");
}

TEST(HiTraceRNOHMarkerListener, MakeMessage_WithTag_AppendsTagSuffix) {
  EXPECT_EQ(
      makeMessage("RUN_JS_BUNDLE", "instance_0"),
      "ReactMarker::RUN_JS_BUNDLE::Tag::instance_0");
}

TEST(HiTraceRNOHMarkerListener, MakeMessage_EmptyMarker_OnlyPrefix) {
  EXPECT_EQ(makeMessage("", ""), "ReactMarker::");
}

TEST(
    HiTraceRNOHMarkerListener,
    MakeMessage_EmptyMarkerWithTag_PrefixAndTagOnly) {
  EXPECT_EQ(makeMessage("", "tag1"), "ReactMarker::::Tag::tag1");
}

TEST(
    HiTraceRNOHMarkerListener,
    GetMessageId_DifferentMessages_ReturnsDifferentIds) {
  EXPECT_NE(
      getMessageId("ReactMarker::RUN_JS_BUNDLE"),
      getMessageId("ReactMarker::CREATE_REACT_CONTEXT"));
}

TEST(HiTraceRNOHMarkerListener, GetMessageId_EmptyMessage_DoesNotCrash) {
  EXPECT_NO_THROW(getMessageId(""));
}

TEST(HiTraceRNOHMarkerListener, GetMessageId_TaggedVsUntagged_DifferentIds) {
  auto id1 = getMessageId("ReactMarker::RUN_JS_BUNDLE");
  auto id2 = getMessageId("ReactMarker::RUN_JS_BUNDLE::Tag::instance_0");
  EXPECT_NE(id1, id2);
}
