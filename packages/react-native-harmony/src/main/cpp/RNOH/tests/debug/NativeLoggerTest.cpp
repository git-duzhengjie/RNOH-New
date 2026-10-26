/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests the REAL NativeLogger.cpp via #include .cpp pattern (P8 compliant).
// Dependency: hilog/log.h is stubbed with call recording.

#include "NativeLogger.cpp" // NOLINT: #include .cpp pattern — tests real source
#include <gtest/gtest.h>
#include <string>

using namespace rnoh;

class NativeLoggerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetHilogCallLog();
  }
};

TEST_F(NativeLoggerTest, LogLevel0_Debug_CallsDebugLevel) {
  nativeLogger("debug message", 0);
  EXPECT_EQ(hilogCallLog().size(), 1u);
  EXPECT_EQ(hilogCallLog()[0].level, LOG_LEVEL_DEBUG);
}

TEST_F(NativeLoggerTest, LogLevel1_Info_CallsInfoLevel) {
  nativeLogger("info message", 1);
  EXPECT_EQ(hilogCallLog().size(), 1u);
  EXPECT_EQ(hilogCallLog()[0].level, LOG_LEVEL_INFO);
}

TEST_F(NativeLoggerTest, LogLevel2_Warn_CallsWarnLevel) {
  nativeLogger("warn message", 2);
  EXPECT_EQ(hilogCallLog().size(), 1u);
  EXPECT_EQ(hilogCallLog()[0].level, LOG_LEVEL_WARN);
}

TEST_F(NativeLoggerTest, LogLevel3_Error_CallsErrorLevel) {
  nativeLogger("error message", 3);
  EXPECT_EQ(hilogCallLog().size(), 1u);
  EXPECT_EQ(hilogCallLog()[0].level, LOG_LEVEL_ERROR);
}

TEST_F(NativeLoggerTest, LogLevelDefault_FallsBackToInfo) {
  nativeLogger("default message", 99);
  EXPECT_EQ(hilogCallLog().size(), 1u);
  EXPECT_EQ(hilogCallLog()[0].level, LOG_LEVEL_INFO);
}

TEST_F(NativeLoggerTest, EmptyMessage_ProducesOneCall) {
  nativeLogger("", 1);
  EXPECT_EQ(hilogCallLog().size(), 1u);
}

TEST_F(NativeLoggerTest, LongMessage_ProducesOneCall) {
  std::string longMsg(1000, 'x');
  nativeLogger(longMsg, 1);
  EXPECT_EQ(hilogCallLog().size(), 1u);
  EXPECT_GT(hilogCallLog()[0].fmt.size(), 0u);
}

TEST_F(NativeLoggerTest, EachLevel_ProducesExactlyOneCall) {
  for (unsigned int lvl = 0; lvl <= 3; ++lvl) {
    resetHilogCallLog();
    nativeLogger("msg", lvl);
    EXPECT_EQ(hilogCallLog().size(), 1u);
  }
}

TEST_F(NativeLoggerTest, DebugVsError_LogsDifferentLevels) {
  nativeLogger("dbg", 0);
  nativeLogger("err", 3);
  EXPECT_EQ(hilogCallLog().size(), 2u);
  EXPECT_NE(hilogCallLog()[0].level, hilogCallLog()[1].level);
  EXPECT_EQ(hilogCallLog()[0].level, LOG_LEVEL_DEBUG);
  EXPECT_EQ(hilogCallLog()[1].level, LOG_LEVEL_ERROR);
}
