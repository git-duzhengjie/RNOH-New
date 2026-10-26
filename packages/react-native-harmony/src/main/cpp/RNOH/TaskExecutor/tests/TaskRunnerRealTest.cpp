/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Tests real ThreadTaskRunner + EventLoopTaskRunner with libuv (Homebrew).
// Covers branches in EventLoopTaskRunner.cpp (14 branches).
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include "RNOH/TaskExecutor/ThreadTaskRunner.h"

using namespace rnoh;

class TaskRunnerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    runner_ = std::make_unique<ThreadTaskRunner>(
        "test-runner", [](std::exception_ptr) {});
  }
  void TearDown() override { runner_.reset(); }
  std::unique_ptr<ThreadTaskRunner> runner_;
};

TEST_F(TaskRunnerTest, Constructor_CreatesThreadAndLoop) {
  ASSERT_NE(runner_, nullptr);
}

TEST_F(TaskRunnerTest, IsOnCurrentThread_CalledFromExternalThread_ReturnsFalse) {
  EXPECT_FALSE(runner_->isOnCurrentThread());
}

TEST_F(TaskRunnerTest, IsOnCurrentThread_CalledFromRunnerThread_ReturnsTrue) {
  std::promise<bool> promise;
  auto fut = promise.get_future();
  runner_->runAsyncTask([&]() {
    promise.set_value(runner_->isOnCurrentThread());
  });
  EXPECT_TRUE(fut.get());
}

TEST_F(TaskRunnerTest, RunAsyncTask_ExecutesTask) {
  std::atomic<int> counter{0};
  runner_->runAsyncTask([&]() { counter.store(1); });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(counter.load(), 1);
}

TEST_F(TaskRunnerTest, RunSyncTask_ExecutesSynchronously) {
  std::atomic<int> value{0};
  runner_->runSyncTask([&]() { value.store(42); });
  EXPECT_EQ(value.load(), 42);
}

TEST_F(TaskRunnerTest, RunSyncTask_ReturnsValueViaPromise) {
  std::promise<int> promise;
  auto fut = promise.get_future();
  runner_->runSyncTask([&]() { promise.set_value(123); });
  EXPECT_EQ(fut.get(), 123);
}

TEST_F(TaskRunnerTest, RunDelayedTask_ExecutesAfterDelay) {
  std::atomic<int> counter{0};
  runner_->runDelayedTask([&]() { counter.store(1); }, 50);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(counter.load(), 1);
}

TEST_F(TaskRunnerTest, CancelDelayedTask_PreventsExecution) {
  std::atomic<int> counter{0};
  auto id = runner_->runDelayedTask([&]() { counter.store(1); }, 500);
  runner_->cancelDelayedTask(id);
  std::this_thread::sleep_for(std::chrono::milliseconds(600));
  EXPECT_EQ(counter.load(), 0);
}

TEST_F(TaskRunnerTest, RunDelayedTask_Repeat_ExecutesMultipleTimes) {
  std::atomic<int> counter{0};
  runner_->runDelayedTask([&]() { counter.fetch_add(1); }, 10, 10);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_GE(counter.load(), 2);
}

TEST_F(TaskRunnerTest, SetExceptionHandler_DoesNotCrash) {
  runner_->setExceptionHandler([](std::exception_ptr) {});
  SUCCEED();
}

TEST_F(TaskRunnerTest, MultipleTasks_AllExecute) {
  std::atomic<int> counter{0};
  for (int i = 0; i < 5; ++i) {
    runner_->runAsyncTask([&]() { counter.fetch_add(1); });
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_EQ(counter.load(), 5);
}
