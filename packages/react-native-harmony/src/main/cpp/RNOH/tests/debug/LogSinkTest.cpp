/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests the REAL LogSink.cpp via #include .cpp pattern (P8 compliant).
// Dependencies are stubbed: hilog/log.h, pthread.h, glog/logging.h.
// pthread stub supports g_testThreadName for testing different thread names.

#include "LogSink.cpp" // NOLINT: #include .cpp pattern — tests real source
#include <gtest/gtest.h>
#include <cstring>
#include <string>

// getThreadSymbol() reads thread name via pthread_getname_np.
// The pthread stub uses g_testThreadName global (char[256] to avoid
// <string> include in pthread.h which would cause <atomic> circular
// dependency).
extern char g_testThreadName[];

class LogSinkTest : public ::testing::Test {
 protected:
  void SetUp() override {
    g_testThreadName[0] = '\0';
  }
  void TearDown() override {
    g_testThreadName[0] = '\0';
  }
};

// ============================================================================
// getThreadSymbol (reads thread name via pthread_getname_np)
// ============================================================================

TEST_F(LogSinkTest, GetThreadSymbol_RNOH_JS_ReturnsJsSymbol) {
  std::strncpy(g_testThreadName, "RNOH_JS", 255);
  EXPECT_EQ(getThreadSymbol(), "__\xe2\x96\x88__");
}

TEST_F(LogSinkTest, GetThreadSymbol_RNOH_BACKGROUND_ReturnsBackgroundSymbol) {
  std::strncpy(g_testThreadName, "RNOH_BACKGROUND", 255);
  EXPECT_EQ(getThreadSymbol(), "_\xe2\x96\x88___");
}

TEST_F(LogSinkTest, GetThreadSymbol_RNOH_CLEANUP_ReturnsCleanupSymbol) {
  std::strncpy(g_testThreadName, "RNOH_CLEANUP", 255);
  EXPECT_EQ(getThreadSymbol(), "____\xe2\x96\x88");
}

TEST_F(LogSinkTest, GetThreadSymbol_WorkerThread_ReturnsWorkerSymbol) {
  std::strncpy(g_testThreadName, "WorkerThread", 255);
  EXPECT_EQ(getThreadSymbol(), "___\xe2\x96\x88_");
}

TEST_F(LogSinkTest, GetThreadSymbol_EventRunner_ReturnsEventRunnerSymbol) {
  std::strncpy(g_testThreadName, "EventRunner", 255);
  EXPECT_EQ(getThreadSymbol(), "_____\xe2\x96\x88 (EventRunner)");
}

TEST_F(
    LogSinkTest,
    GetThreadSymbol_EventRunnerWithSuffix_ReturnsEventRunnerSymbol) {
  std::strncpy(g_testThreadName, "EventRunner123", 255);
  EXPECT_EQ(getThreadSymbol(), "_____\xe2\x96\x88 (EventRunner123)");
}

TEST_F(LogSinkTest, GetThreadSymbol_EmptyString_ReturnsDefault) {
  g_testThreadName[0] = '\0';
  EXPECT_EQ(getThreadSymbol(), "\xe2\x96\x88____");
}

TEST_F(LogSinkTest, GetThreadSymbol_UnknownName_ReturnsDefault) {
  std::strncpy(g_testThreadName, "unknown", 255);
  EXPECT_EQ(getThreadSymbol(), "\xe2\x96\x88____");
}

TEST_F(LogSinkTest, GetThreadSymbol_CaseSensitive_LowercaseReturnsDefault) {
  std::strncpy(g_testThreadName, "rnoh_js", 255);
  EXPECT_EQ(getThreadSymbol(), "\xe2\x96\x88____");
}

TEST_F(LogSinkTest, GetThreadSymbol_AllKnownSymbols_Different) {
  std::string names[] = {
      "RNOH_JS",
      "RNOH_BACKGROUND",
      "RNOH_CLEANUP",
      "WorkerThread",
      "EventRunner",
      "unknown"};
  std::string prev;
  for (const auto& name : names) {
    std::strncpy(g_testThreadName, name.c_str(), 255);
    g_testThreadName[255] = '\0';
    auto result = getThreadSymbol();
    if (!prev.empty()) {
      EXPECT_NE(result, prev);
    }
    prev = result;
  }
}

// ============================================================================
// LogSink::initializeLogging + send (instance methods)
// ============================================================================

TEST_F(LogSinkTest, InitializeLogging_DoesNotCrash) {
  LogSink::initializeLogging();
  SUCCEED();
}

TEST_F(LogSinkTest, InitializeLogging_CalledTwice_NoCrash) {
  LogSink::initializeLogging();
  LogSink::initializeLogging();
  SUCCEED();
}

TEST_F(LogSinkTest, Send_InfoSeverity_DoesNotCrash) {
  LogSink::initializeLogging();
  LogSink sink;
  std::string msg = "test info message";
  sink.send(
      google::GLOG_INFO,
      "file.cpp",
      "file.cpp",
      42,
      nullptr,
      msg.c_str(),
      msg.size());
  SUCCEED();
}

TEST_F(LogSinkTest, Send_WarningSeverity_DoesNotCrash) {
  LogSink sink;
  std::string msg = "test warning";
  sink.send(
      google::GLOG_WARNING,
      "file.cpp",
      "file.cpp",
      10,
      nullptr,
      msg.c_str(),
      msg.size());
  SUCCEED();
}

TEST_F(LogSinkTest, Send_ErrorSeverity_DoesNotCrash) {
  LogSink sink;
  std::string msg = "test error";
  sink.send(
      google::GLOG_ERROR,
      "file.cpp",
      "file.cpp",
      20,
      nullptr,
      msg.c_str(),
      msg.size());
  SUCCEED();
}

TEST_F(LogSinkTest, Send_FatalSeverity_DoesNotCrash) {
  LogSink sink;
  std::string msg = "test fatal";
  sink.send(
      google::GLOG_FATAL,
      "file.cpp",
      "file.cpp",
      30,
      nullptr,
      msg.c_str(),
      msg.size());
  SUCCEED();
}

TEST_F(LogSinkTest, Send_DefaultSeverity_DoesNotCrash) {
  LogSink sink;
  std::string msg = "test default";
  sink.send(
      static_cast<google::LogSeverity>(99),
      "file.cpp",
      "file.cpp",
      40,
      nullptr,
      msg.c_str(),
      msg.size());
  SUCCEED();
}

TEST_F(LogSinkTest, Send_EmptyMessage_DoesNotCrash) {
  LogSink sink;
  sink.send(google::GLOG_INFO, "file.cpp", "file.cpp", 1, nullptr, "", 0);
  SUCCEED();
}
