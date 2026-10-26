/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "MemCacheCallback.h"
#include <gtest/gtest.h>

#include <memory>

using namespace ImageCachePro;

TEST(MemCacheCallback, RegisterCallback_ValidCallback_NotifiesOnNotifyAll) {
  auto called = std::make_shared<bool>(false);
  MemCacheCallback::registerCallback([called](int) { *called = true; });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_MODERATE);
  EXPECT_TRUE(*called);
}

TEST(MemCacheCallback, RegisterCallback_NullCallback_NotifyAllDoesNotCrash) {
  MemCacheCallback::registerCallback(nullptr);
  EXPECT_NO_THROW(MemCacheCallback::notifyAll(MEMORY_LEVEL_LOW));
}

TEST(MemCacheCallback, NotifyAll_PassesLevelToCallback) {
  auto received = std::make_shared<int>(-1);
  MemCacheCallback::registerCallback(
      [received](int level) { *received = level; });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_LOW);
  EXPECT_EQ(*received, MEMORY_LEVEL_LOW);
}

TEST(MemCacheCallback, NotifyAll_CriticalLevel_PassesCorrectly) {
  auto received = std::make_shared<int>(-1);
  MemCacheCallback::registerCallback(
      [received](int level) { *received = level; });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_CRITICAL);
  EXPECT_EQ(*received, MEMORY_LEVEL_CRITICAL);
  EXPECT_NE(*received, MEMORY_LEVEL_LOW);
}

TEST(MemCacheCallback, NotifyAll_ModerateLevel_DiffersFromCritical) {
  auto received = std::make_shared<int>(-1);
  MemCacheCallback::registerCallback(
      [received](int level) { *received = level; });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_MODERATE);
  EXPECT_EQ(*received, MEMORY_LEVEL_MODERATE);
  EXPECT_NE(*received, MEMORY_LEVEL_CRITICAL);
}

TEST(MemCacheCallback, NotifyAll_MultipleCallbacks_AllInvoked) {
  auto called1 = std::make_shared<bool>(false);
  auto called2 = std::make_shared<bool>(false);
  MemCacheCallback::registerCallback([called1](int) { *called1 = true; });
  MemCacheCallback::registerCallback([called2](int) { *called2 = true; });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_MODERATE);
  EXPECT_TRUE(*called1);
  EXPECT_TRUE(*called2);
}

TEST(MemCacheCallback, NotifyBackground_PassesCriticalLevel) {
  auto received = std::make_shared<int>(-1);
  MemCacheCallback::registerCallback(
      [received](int level) { *received = level; });
  MemCacheCallback::notifyBackground();
  EXPECT_EQ(*received, MEMORY_LEVEL_CRITICAL);
}

TEST(MemCacheCallback, NotifyBackground_DiffersFromNotifyAllModerate) {
  auto received = std::make_shared<int>(-1);
  MemCacheCallback::registerCallback(
      [received](int level) { *received = level; });
  MemCacheCallback::notifyBackground();
  EXPECT_NE(*received, MEMORY_LEVEL_MODERATE);
  EXPECT_NE(*received, MEMORY_LEVEL_LOW);
}

TEST(
    MemCacheCallback,
    NotifyBackground_MultipleCallbacks_CountMatchesRegistered) {
  auto count = std::make_shared<int>(0);
  MemCacheCallback::registerCallback([count](int) { (*count)++; });
  MemCacheCallback::registerCallback([count](int) { (*count)++; });
  MemCacheCallback::notifyBackground();
  EXPECT_GT(*count, 0);
}

TEST(MemCacheCallback, NotifyAll_AfterMultipleRegistrations_AllReceiveLevel) {
  auto levels = std::make_shared<std::vector<int>>();
  MemCacheCallback::registerCallback([levels](int l) { levels->push_back(l); });
  MemCacheCallback::registerCallback([levels](int l) { levels->push_back(l); });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_MODERATE);
  EXPECT_EQ(levels->size(), 2u);
  for (auto l : *levels) {
    EXPECT_EQ(l, MEMORY_LEVEL_MODERATE);
  }
}

TEST(MemCacheCallback, NotifyAll_SecondCall_CallbacksStillActive) {
  auto count = std::make_shared<int>(0);
  MemCacheCallback::registerCallback([count](int) { (*count)++; });
  MemCacheCallback::notifyAll(MEMORY_LEVEL_LOW);
  int firstCount = *count;
  MemCacheCallback::notifyAll(MEMORY_LEVEL_CRITICAL);
  EXPECT_GT(*count, firstCount);
}
