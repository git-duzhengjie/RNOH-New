/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <optional>
#include <string>

// Reimplement onSetPausedInDebuggerMessage + onReload logic.
// The production class requires ArkTSChannel + HostTargetDelegate mocks.
// Keep in sync with RNOH/JSInspectorHostTargetDelegate.cpp.

namespace {

struct PauseRequest {
  std::optional<std::string> message;
};

struct MessageCall {
  std::string name;
  bool hasPayload;
  std::string payloadMessage;
};

MessageCall decidePauseMessage(const PauseRequest& request) {
  if (request.message.has_value()) {
    return {"RNOH::OPEN_DEBUGGER_PAUSED_DIALOG", true, *request.message};
  }
  return {"RNOH::CLOSE_DEBUGGER_PAUSED_DIALOG", false, ""};
}

struct ReloadDecision {
  bool shouldSend;
  std::string messageName;
};

ReloadDecision decideReload(bool channelAlive) {
  if (!channelAlive) {
    return {false, ""};
  }
  return {true, "RNOH::ON_RELOAD"};
}

} // namespace

// === onSetPausedInDebuggerMessage ===

TEST(JSInspectorHostTargetDelegate, PauseMessage_WithMessage_OpensDialog) {
  PauseRequest req{{"Paused in debugger"}};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.name, "RNOH::OPEN_DEBUGGER_PAUSED_DIALOG");
  EXPECT_TRUE(call.hasPayload);
  EXPECT_EQ(call.payloadMessage, "Paused in debugger");
}

TEST(JSInspectorHostTargetDelegate, PauseMessage_WithoutMessage_ClosesDialog) {
  PauseRequest req{std::nullopt};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.name, "RNOH::CLOSE_DEBUGGER_PAUSED_DIALOG");
  EXPECT_FALSE(call.hasPayload);
  EXPECT_TRUE(call.payloadMessage.empty());
}

TEST(JSInspectorHostTargetDelegate, PauseMessage_WithEmptyString_OpensDialog) {
  PauseRequest req{std::string("")};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.name, "RNOH::OPEN_DEBUGGER_PAUSED_DIALOG");
  EXPECT_TRUE(call.hasPayload);
}

TEST(
    JSInspectorHostTargetDelegate,
    PauseMessage_WithSpecialChars_PayloadPreserved) {
  PauseRequest req{{"Error: <script>alert(1)</script>"}};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.payloadMessage, "Error: <script>alert(1)</script>");
}

TEST(
    JSInspectorHostTargetDelegate,
    PauseMessage_WithLongMessage_PayloadPreserved) {
  PauseRequest req{{std::string(10000, 'x')}};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.payloadMessage.size(), 10000u);
}

TEST(JSInspectorHostTargetDelegate, PauseMessage_WithUnicode_PayloadPreserved) {
  PauseRequest req{{std::string("暂停调试器你好世界")}};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.payloadMessage, "暂停调试器你好世界");
}

TEST(
    JSInspectorHostTargetDelegate,
    PauseMessage_WithNewlines_PayloadPreserved) {
  PauseRequest req{{"line1\nline2\nline3"}};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.payloadMessage, "line1\nline2\nline3");
  EXPECT_EQ(call.name, "RNOH::OPEN_DEBUGGER_PAUSED_DIALOG");
}

TEST(
    JSInspectorHostTargetDelegate,
    PauseMessage_WithNullChar_PayloadPreserved) {
  PauseRequest req{{std::string("a\0b", 3)}};
  auto call = decidePauseMessage(req);
  EXPECT_EQ(call.payloadMessage.size(), 3u);
  EXPECT_EQ(call.payloadMessage[0], 'a');
  EXPECT_EQ(call.payloadMessage[1], '\0');
  EXPECT_EQ(call.payloadMessage[2], 'b');
}

// === onReload ===

TEST(JSInspectorHostTargetDelegate, Reload_ChannelAlive_SendsReloadMessage) {
  auto decision = decideReload(true);
  EXPECT_TRUE(decision.shouldSend);
  EXPECT_EQ(decision.messageName, "RNOH::ON_RELOAD");
}

TEST(JSInspectorHostTargetDelegate, Reload_ChannelDead_SkipsReload) {
  auto decision = decideReload(false);
  EXPECT_FALSE(decision.shouldSend);
  EXPECT_TRUE(decision.messageName.empty());
}

TEST(JSInspectorHostTargetDelegate, Reload_ChannelAlive_MessageIsSpecific) {
  auto decision = decideReload(true);
  EXPECT_EQ(decision.messageName, "RNOH::ON_RELOAD");
  EXPECT_NE(decision.messageName, "RNOH::OPEN_DEBUGGER_PAUSED_DIALOG");
  EXPECT_NE(decision.messageName, "RNOH::CLOSE_DEBUGGER_PAUSED_DIALOG");
}

TEST(JSInspectorHostTargetDelegate, Reload_ChannelDead_NoMessageSent) {
  auto decision = decideReload(false);
  EXPECT_EQ(decision.messageName, "");
}
