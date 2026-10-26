/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include <queue>
#include "ComponentInstancePreallocationRequestQueue.h"
#include "RNOH/ParallelCheck.h"

using namespace rnoh;
using namespace facebook::react;

namespace rnoh::test {

class MockPreallocationDelegate
    : public ComponentInstancePreallocationRequestQueue::Delegate {
 public:
  int onPushCount = 0;
  int processCount = 0;
  Tag lastProcessedTag = -1;

  void onPushPreallocationRequest() override { onPushCount++; }

  void processPreallocationRequest(
      ComponentInstancePreallocationRequestQueue::Request const& req) override {
    processCount++;
    lastProcessedTag = req.tag;
  }
};

static ComponentInstancePreallocationRequestQueue::Request
makeRequest(Tag tag = 1, ComponentHandle handle = 100,
            ComponentName name = "View") {
  ComponentInstancePreallocationRequestQueue::Request req;
  req.tag = tag;
  req.componentHandle = handle;
  req.componentName = name;
  return req;
}

} // namespace rnoh::test

// ============================================================================
// push 鈥?non-parallel path (IsParallelizationEnabled() returns false)
// ============================================================================

TEST(PreallocationQueueReal, Push_SingleRequest_QueueNotEmpty) {
  ComponentInstancePreallocationRequestQueue queue;
  queue.push(test::makeRequest(1));
  EXPECT_FALSE(queue.isEmpty());
}

TEST(PreallocationQueueReal, Push_DelegateNotified) {
  ComponentInstancePreallocationRequestQueue queue;
  auto delegate = std::make_shared<test::MockPreallocationDelegate>();
  queue.setDelegate(delegate);
  queue.push(test::makeRequest(1));
  EXPECT_EQ(delegate->onPushCount, 1);
}

TEST(PreallocationQueueReal, Push_NoDelegate_DoesNotThrow) {
  ComponentInstancePreallocationRequestQueue queue;
  EXPECT_NO_THROW(queue.push(test::makeRequest(1)));
}

TEST(PreallocationQueueReal, Push_MultipleRequests_AllEnqueued) {
  ComponentInstancePreallocationRequestQueue queue;
  queue.push(test::makeRequest(1));
  queue.push(test::makeRequest(2));
  queue.push(test::makeRequest(3));
  auto r1 = queue.pop();
  auto r2 = queue.pop();
  auto r3 = queue.pop();
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());
  ASSERT_TRUE(r3.has_value());
  EXPECT_EQ(r1->tag, 1);
  EXPECT_EQ(r2->tag, 2);
  EXPECT_EQ(r3->tag, 3);
}

TEST(PreallocationQueueReal, Push_DelegateExceptionCaught) {
  ComponentInstancePreallocationRequestQueue queue;
  class ThrowingDelegate
      : public ComponentInstancePreallocationRequestQueue::Delegate {
   public:
    void onPushPreallocationRequest() override {
      throw std::runtime_error("test exception");
    }
    void processPreallocationRequest(
        ComponentInstancePreallocationRequestQueue::Request const&) override {}
  };
  auto delegate = std::make_shared<ThrowingDelegate>();
  queue.setDelegate(delegate);
  EXPECT_NO_THROW(queue.push(test::makeRequest(1)));
  EXPECT_FALSE(queue.isEmpty());
}

// ============================================================================
// pop
// ============================================================================

TEST(PreallocationQueueReal, Pop_EmptyQueue_ReturnsNullopt) {
  ComponentInstancePreallocationRequestQueue queue;
  auto result = queue.pop();
  EXPECT_FALSE(result.has_value());
}

TEST(PreallocationQueueReal, Pop_WithItems_ReturnsFIFO) {
  ComponentInstancePreallocationRequestQueue queue;
  queue.push(test::makeRequest(10));
  queue.push(test::makeRequest(20));
  auto r1 = queue.pop();
  auto r2 = queue.pop();
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());
  EXPECT_EQ(r1->tag, 10);
  EXPECT_EQ(r2->tag, 20);
}

// ============================================================================
// isEmpty / clear / setDelegate
// ============================================================================

TEST(PreallocationQueueReal, IsEmpty_InitiallyTrue) {
  ComponentInstancePreallocationRequestQueue queue;
  EXPECT_TRUE(queue.isEmpty());
}

TEST(PreallocationQueueReal, Clear_RemovesAllRequests) {
  ComponentInstancePreallocationRequestQueue queue;
  queue.push(test::makeRequest(1));
  queue.push(test::makeRequest(2));
  EXPECT_FALSE(queue.isEmpty());
  queue.clear();
  EXPECT_TRUE(queue.isEmpty());
}

TEST(PreallocationQueueReal, SetDelegate_WeakPtr) {
  ComponentInstancePreallocationRequestQueue queue;
  auto delegate = std::make_shared<test::MockPreallocationDelegate>();
  queue.setDelegate(delegate);
  queue.push(test::makeRequest(1));
  EXPECT_EQ(delegate->onPushCount, 1);
  delegate.reset();
  EXPECT_NO_THROW(queue.push(test::makeRequest(2)));
}

// === Parallel path tests ===

TEST(PreallocationQueueReal, Push_ParallelMode_NoDelegate_DoesNotThrow) {
  rnoh::test::setParallelizationEnabled(true);
  ComponentInstancePreallocationRequestQueue queue;
  EXPECT_NO_THROW(queue.push(test::makeRequest(1)));
  rnoh::test::setParallelizationEnabled(false);
}

TEST(PreallocationQueueReal, Push_ParallelMode_ParallelComponent_ProcessedByDelegate) {
  rnoh::test::setParallelizationEnabled(true);
  ComponentInstancePreallocationRequestQueue queue;
  auto delegate = std::make_shared<test::MockPreallocationDelegate>();
  queue.setDelegate(delegate);
  queue.push(test::makeRequest(1, 100, "View"));
  EXPECT_EQ(delegate->processCount, 1);
  EXPECT_EQ(delegate->lastProcessedTag, 1);
  rnoh::test::setParallelizationEnabled(false);
}

TEST(PreallocationQueueReal, Push_ParallelMode_ParallelComponent_DelegateExceptionCaught) {
  rnoh::test::setParallelizationEnabled(true);
  ComponentInstancePreallocationRequestQueue queue;
  class ThrowingProcessDelegate
      : public ComponentInstancePreallocationRequestQueue::Delegate {
   public:
    void onPushPreallocationRequest() override {}
    void processPreallocationRequest(
        ComponentInstancePreallocationRequestQueue::Request const&) override {
      throw std::runtime_error("process error");
    }
  };
  auto delegate = std::make_shared<ThrowingProcessDelegate>();
  queue.setDelegate(delegate);
  EXPECT_NO_THROW(queue.push(test::makeRequest(1, 100, "View")));
  rnoh::test::setParallelizationEnabled(false);
}
