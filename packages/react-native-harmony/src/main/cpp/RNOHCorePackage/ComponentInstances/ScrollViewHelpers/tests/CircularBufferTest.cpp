/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "CircularBuffer.h"
#include <gtest/gtest.h>

using namespace rnoh;

TEST(CircularBuffer, Constructor_CreatesEmptyBuffer) {
  CircularBuffer<int> buffer(5);
  EXPECT_EQ(buffer.size(), 0u);
}

TEST(CircularBuffer, Push_SingleElement_SizeIsOne) {
  CircularBuffer<int> buffer(5);
  buffer.push(42);
  EXPECT_EQ(buffer.size(), 1u);
}

TEST(CircularBuffer, Push_MultipleElements_SizeIsCorrect) {
  CircularBuffer<int> buffer(5);
  buffer.push(1);
  buffer.push(2);
  buffer.push(3);
  EXPECT_EQ(buffer.size(), 3u);
}

TEST(CircularBuffer, Push_ExceedingCapacity_SizeEqualsCapacity) {
  CircularBuffer<int> buffer(3);
  buffer.push(1);
  buffer.push(2);
  buffer.push(3);
  buffer.push(4);
  buffer.push(5);
  EXPECT_EQ(buffer.size(), 3u);
}

TEST(CircularBuffer, Get_BeforeWraparound_ReturnsCorrectElements) {
  CircularBuffer<int> buffer(5);
  buffer.push(10);
  buffer.push(20);
  buffer.push(30);
  EXPECT_EQ(buffer.get(0), 10);
  EXPECT_EQ(buffer.get(1), 20);
  EXPECT_EQ(buffer.get(2), 30);
}

TEST(CircularBuffer, Get_AfterWraparound_ReturnsOldestToNewest) {
  CircularBuffer<int> buffer(3);
  buffer.push(1);
  buffer.push(2);
  buffer.push(3);
  buffer.push(4);
  buffer.push(5);
  EXPECT_EQ(buffer.get(0), 3);
  EXPECT_EQ(buffer.get(1), 4);
  EXPECT_EQ(buffer.get(2), 5);
}

TEST(CircularBuffer, Get_OverwritesOldestElement) {
  CircularBuffer<int> buffer(2);
  buffer.push(100);
  buffer.push(200);
  EXPECT_EQ(buffer.get(0), 100);
  EXPECT_EQ(buffer.get(1), 200);
  buffer.push(300);
  EXPECT_EQ(buffer.get(0), 200);
  EXPECT_EQ(buffer.get(1), 300);
}

TEST(CircularBuffer, Clear_ResetsSizeToZero) {
  CircularBuffer<int> buffer(5);
  buffer.push(1);
  buffer.push(2);
  buffer.push(3);
  buffer.clear();
  EXPECT_EQ(buffer.size(), 0u);
}

TEST(CircularBuffer, Clear_AllowsReuseAfterClear) {
  CircularBuffer<int> buffer(3);
  buffer.push(1);
  buffer.push(2);
  buffer.clear();
  buffer.push(10);
  buffer.push(20);
  EXPECT_EQ(buffer.size(), 2u);
  EXPECT_EQ(buffer.get(0), 10);
  EXPECT_EQ(buffer.get(1), 20);
}

TEST(CircularBuffer, CapacityOne_AlwaysReturnsLatestElement) {
  CircularBuffer<int> buffer(1);
  buffer.push(1);
  EXPECT_EQ(buffer.size(), 1u);
  EXPECT_EQ(buffer.get(0), 1);
  buffer.push(2);
  EXPECT_EQ(buffer.size(), 1u);
  EXPECT_EQ(buffer.get(0), 2);
  buffer.push(3);
  EXPECT_EQ(buffer.size(), 1u);
  EXPECT_EQ(buffer.get(0), 3);
}

TEST(CircularBuffer, Constructor_ZeroCapacity_CreatesEmptyBuffer) {
  CircularBuffer<int> buffer(0);
  EXPECT_EQ(buffer.size(), 0u);
}

TEST(CircularBuffer, Push_OnZeroCapacity_IsUndefinedBehavior) {
  CircularBuffer<int> buffer(0);
  EXPECT_EQ(buffer.size(), 0u);
  EXPECT_DEATH_IF_SUPPORTED(buffer.push(42), ".*");
}

TEST(CircularBuffer, Push_DifferentTypes_WorksWithDouble) {
  CircularBuffer<double> buffer(3);
  buffer.push(1.5);
  buffer.push(2.7);
  buffer.push(3.14);
  EXPECT_DOUBLE_EQ(buffer.get(0), 1.5);
  EXPECT_DOUBLE_EQ(buffer.get(1), 2.7);
  EXPECT_DOUBLE_EQ(buffer.get(2), 3.14);
}

struct TestEvent {
  double x;
  double y;
  int64_t time;
};

TEST(CircularBuffer, Push_DifferentTypes_WorksWithStruct) {
  CircularBuffer<TestEvent> buffer(2);
  buffer.push({1.0, 2.0, 100});
  buffer.push({3.0, 4.0, 200});
  EXPECT_EQ(buffer.size(), 2u);
  EXPECT_DOUBLE_EQ(buffer.get(0).x, 1.0);
  EXPECT_DOUBLE_EQ(buffer.get(1).y, 4.0);
  buffer.push({5.0, 6.0, 300});
  EXPECT_EQ(buffer.size(), 2u);
  EXPECT_DOUBLE_EQ(buffer.get(0).x, 3.0);
  EXPECT_DOUBLE_EQ(buffer.get(1).x, 5.0);
}

TEST(CircularBuffer, Get_PartiallyFilledDouble_ReturnsInsertedElement) {
  CircularBuffer<double> buffer(3);
  buffer.push(1.5);

  EXPECT_DOUBLE_EQ(buffer.get(0), 1.5);
}

TEST(CircularBuffer, Get_PartiallyFilledStruct_ReturnsInsertedElement) {
  CircularBuffer<TestEvent> buffer(2);
  buffer.push({1.0, 2.0, 100});

  EXPECT_DOUBLE_EQ(buffer.get(0).x, 1.0);
  EXPECT_DOUBLE_EQ(buffer.get(0).y, 2.0);
  EXPECT_EQ(buffer.get(0).time, 100);
}

TEST(CircularBuffer, Push_FillsCapacity_TakesWraparoundReadPath) {
  CircularBuffer<int> buffer(3);
  buffer.push(1);
  buffer.push(2);
  buffer.push(3);

  EXPECT_EQ(buffer.size(), 3u);
  EXPECT_EQ(buffer.get(0), 1);
  EXPECT_EQ(buffer.get(1), 2);
  EXPECT_EQ(buffer.get(2), 3);
}
