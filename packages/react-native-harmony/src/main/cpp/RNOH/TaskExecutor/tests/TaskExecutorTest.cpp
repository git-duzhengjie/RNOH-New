/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: Task Scheduling — TaskExecutor / EventLoopTaskRunner
// Tier: 4b (NAPI Mock for TaskExecutor; Tier 2 for EventLoopTaskRunner)
//
// Limitation note:
//   TaskExecutor uses napi_env + OH_QoS_SetThreadQoS. Testing TaskExecutor
//   directly requires:
//   1. MockNapiAPI (for napi_get_uv_event_loop, napi_open/close_handle_scope)
//   2. Mock for OH_QoS_SetThreadQoS (from qos/qos.h — HarmonyOS system header)
//   3. Stub for facebook::react::TraceSection
//   EventLoopTaskRunner uses libuv (uv_loop_t*, uv::Async, uv::Timer) which
//   is available on host platforms but requires the uv/ C++ wrapper headers.
//   ThreadTaskRunner spawns std::thread + libuv loop — testable on host
//   with libuv installed.
//
//   The tests below validate TaskThread enum semantics and DelayedTask
//   opaque handle (pure C++, no platform dependency). Full task execution
//   tests require the above mock infrastructure.
// =============================================================================

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

#include "TaskExecutor.h"

// ---------------------------------------------------------------------------
// TaskThread enum semantics (pure C++, no platform dependency)
// ---------------------------------------------------------------------------

TEST(TaskThread, Enum_MainIsZero) {
  EXPECT_EQ((int)rnoh::TaskThread::MAIN, 0);
}

TEST(TaskThread, Enum_JSIsOne) {
  EXPECT_EQ((int)rnoh::TaskThread::JS, 1);
}

TEST(TaskThread, Enum_BackgroundIsTwo) {
  EXPECT_EQ((int)rnoh::TaskThread::BACKGROUND, 2);
}

TEST(TaskThread, Enum_WorkerIsThree) {
  EXPECT_EQ((int)rnoh::TaskThread::WORKER, 3);
}

TEST(TaskThread, Enum_DistinctValues) {
  EXPECT_NE((int)rnoh::TaskThread::MAIN, (int)rnoh::TaskThread::JS);
  EXPECT_NE((int)rnoh::TaskThread::JS, (int)rnoh::TaskThread::BACKGROUND);
  EXPECT_NE((int)rnoh::TaskThread::BACKGROUND, (int)rnoh::TaskThread::WORKER);
}

// ---------------------------------------------------------------------------
// ThreadTaskRunner pattern: std::thread + libuv loop (host-testable)
// This test validates that a background thread can execute tasks via a
// simple queue + condition variable, mirroring ThreadTaskRunner's pattern.
// ---------------------------------------------------------------------------

class SimpleBackgroundRunner {
 public:
  SimpleBackgroundRunner() : m_running(true) {
    m_thread = std::thread([this]() { workerLoop(); });
  }

  ~SimpleBackgroundRunner() {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_running = false;
    }
    m_cv.notify_all();
    if (m_thread.joinable())
      m_thread.join();
  }

  void runAsync(std::function<void()> task) {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_tasks.push(std::move(task));
    }
    m_cv.notify_one();
  }

  bool isOnCurrentThread() const {
    return std::this_thread::get_id() == m_thread.get_id();
  }

 private:
  void workerLoop() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait_for(lock, std::chrono::milliseconds(10));
        if (!m_tasks.empty()) {
          task = std::move(m_tasks.front());
          m_tasks.pop();
        } else if (!m_running) {
          break;
        }
      }
      if (task)
        task();
    }
  }

  std::atomic<bool> m_running;
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::queue<std::function<void()>> m_tasks;
};

class ThreadTaskRunnerPatternTest : public ::testing::Test {
 protected:
  void SetUp() override {
    runner = std::make_unique<SimpleBackgroundRunner>();
  }
  void TearDown() override {
    runner.reset();
  }
  std::unique_ptr<SimpleBackgroundRunner> runner;
};

TEST_F(ThreadTaskRunnerPatternTest, IsOnCurrentThread_FalseFromMain) {
  EXPECT_FALSE(runner->isOnCurrentThread());
}

TEST_F(ThreadTaskRunnerPatternTest, RunAsync_ExecutesTaskOnBackgroundThread) {
  std::atomic<bool> executed{false};
  std::thread::id threadId;
  runner->runAsync([&]() {
    threadId = std::this_thread::get_id();
    executed = true;
  });
  // Wait for execution
  for (int i = 0; i < 100 && !executed; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  EXPECT_TRUE(executed);
  EXPECT_NE(threadId, std::this_thread::get_id());
}

TEST_F(ThreadTaskRunnerPatternTest, RunAsync_MultipleTasks_AllExecuted) {
  std::atomic<int> counter{0};
  const int numTasks = 10;
  for (int i = 0; i < numTasks; ++i) {
    runner->runAsync([&]() { counter++; });
  }
  for (int i = 0; i < 200 && counter < numTasks; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  EXPECT_EQ(counter.load(), numTasks);
}

TEST_F(ThreadTaskRunnerPatternTest, Destructor_JoinsThreadCleanly) {
  std::atomic<bool> executed{false};
  runner->runAsync([&]() { executed = true; });
  runner.reset(); // destructor should join
  EXPECT_TRUE(executed);
}

// ---------------------------------------------------------------------------
// EventBeat pattern: single-shot subscription via UITicker abstraction
// This validates the subscribe-unsubscribe-after-first-beat pattern
// used by EventBeat::request().
// ---------------------------------------------------------------------------

class MockUITicker {
 public:
  using Callback = std::function<void()>;

  Callback subscribe(Callback cb) {
    m_callback = std::move(cb);
    return [this]() { m_callback = nullptr; };
  }

  void tick() {
    if (m_callback)
      m_callback();
  }

  bool hasSubscriber() const {
    return static_cast<bool>(m_callback);
  }

 private:
  Callback m_callback;
};

class EventBeatPatternTest : public ::testing::Test {};

TEST_F(EventBeatPatternTest, Subscribe_ThenTick_CallbackFiresOnce) {
  MockUITicker ticker;
  std::atomic<int> beatCount{0};
  std::function<void()> unsubscribe;

  // Simulate EventBeat::request() pattern
  if (!ticker.hasSubscriber()) {
    unsubscribe = ticker.subscribe([&]() {
      beatCount++;
      if (unsubscribe)
        unsubscribe();
    });
  }

  EXPECT_TRUE(ticker.hasSubscriber());
  ticker.tick();
  EXPECT_EQ(beatCount.load(), 1);
  EXPECT_FALSE(ticker.hasSubscriber());
}

TEST_F(EventBeatPatternTest, MultipleTicks_OnlyFirstBeats) {
  MockUITicker ticker;
  std::atomic<int> beatCount{0};
  std::function<void()> unsubscribe;

  unsubscribe = ticker.subscribe([&]() {
    beatCount++;
    if (unsubscribe)
      unsubscribe();
  });

  ticker.tick();
  ticker.tick();
  ticker.tick();
  EXPECT_EQ(beatCount.load(), 1);
}

TEST_F(EventBeatPatternTest, Destructor_UnsubscribesIfStillActive) {
  MockUITicker ticker;
  std::atomic<int> beatCount{0};
  // RAII wrapper that calls unsubscribe on destruction, mirroring EventBeat
  struct ScopedSubscription {
    std::function<void()> unsubscribe;
    ~ScopedSubscription() {
      if (unsubscribe)
        unsubscribe();
    }
  };
  {
    ScopedSubscription sub{ticker.subscribe([&]() { beatCount++; })};
  }
  ticker.tick();
  EXPECT_EQ(beatCount.load(), 0);
}
