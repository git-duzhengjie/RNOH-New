/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheMetrics.h"
#include <gtest/gtest.h>

#include <string>

using namespace ImageCachePro;

// ============================================================================
// Tier 1 — pure C++ (atomic, mutex, map)
// Module: RNOH/ImageCache/include/ImageCacheMetrics
//
// Tested classes:
//   - ImageCacheObjectCounter (Increment/Decrement/GetCount/GetAllCounts)
//   - ImageCacheMetrics (singleton: Increment/Load/Snapshot/Reset/Objects)
//
// Note: ImageCacheMetrics is a process-wide singleton. Reset() is called
// in SetUp to ensure deterministic state.
// ============================================================================

class ImageCacheMetricsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ImageCacheMetrics::Instance().Reset();
  }
};

// ---------------------------------------------------------------------------
// ImageCacheObjectCounter
// ---------------------------------------------------------------------------

TEST_F(ImageCacheMetricsTest, GetCount_UntrackedClass_ReturnsZero) {
  ImageCacheObjectCounter counter;
  EXPECT_EQ(counter.GetCount("NonExistent"), 0);
}

TEST_F(ImageCacheMetricsTest, Increment_NewClass_CountIsOne) {
  ImageCacheObjectCounter counter;
  counter.Increment("MyClass", reinterpret_cast<const void*>(0x1));
  EXPECT_EQ(counter.GetCount("MyClass"), 1);
}

TEST_F(ImageCacheMetricsTest, Increment_Twice_CountIsTwo) {
  ImageCacheObjectCounter counter;
  counter.Increment("MyClass", reinterpret_cast<const void*>(0x1));
  counter.Increment("MyClass", reinterpret_cast<const void*>(0x2));
  EXPECT_EQ(counter.GetCount("MyClass"), 2);
}

TEST_F(ImageCacheMetricsTest, Decrement_AfterIncrement_CountIsZero) {
  ImageCacheObjectCounter counter;
  counter.Increment("MyClass", reinterpret_cast<const void*>(0x1));
  counter.Decrement("MyClass", reinterpret_cast<const void*>(0x1));
  EXPECT_EQ(counter.GetCount("MyClass"), 0);
}

TEST_F(ImageCacheMetricsTest, Decrement_BelowZero_CountIsNegative) {
  ImageCacheObjectCounter counter;
  counter.Decrement("MyClass", reinterpret_cast<const void*>(0x1));
  EXPECT_EQ(counter.GetCount("MyClass"), -1);
}

TEST_F(ImageCacheMetricsTest, GetAllCounts_MultipleClasses_ReturnsAll) {
  ImageCacheObjectCounter counter;
  counter.Increment("ClassA", reinterpret_cast<const void*>(0x1));
  counter.Increment("ClassB", reinterpret_cast<const void*>(0x2));
  counter.Increment("ClassA", reinterpret_cast<const void*>(0x3));
  auto counts = counter.GetAllCounts();
  EXPECT_EQ(counts.size(), 2u);
  EXPECT_EQ(counts["ClassA"], 2);
  EXPECT_EQ(counts["ClassB"], 1);
}

TEST_F(ImageCacheMetricsTest, GetAllCounts_Empty_ReturnsEmpty) {
  ImageCacheObjectCounter counter;
  auto counts = counter.GetAllCounts();
  EXPECT_TRUE(counts.empty());
}

TEST_F(ImageCacheMetricsTest, Increment_SamePtrTwice_CountIsTwo) {
  ImageCacheObjectCounter counter;
  const void* ptr = reinterpret_cast<const void*>(0xABCD);
  counter.Increment("Obj", ptr);
  counter.Increment("Obj", ptr);
  EXPECT_EQ(counter.GetCount("Obj"), 2);
}

// ---------------------------------------------------------------------------
// ImageCacheMetrics singleton
// ---------------------------------------------------------------------------

TEST_F(ImageCacheMetricsTest, Instance_ReturnsSameSingleton) {
  auto& a = ImageCacheMetrics::Instance();
  auto& b = ImageCacheMetrics::Instance();
  EXPECT_EQ(&a, &b);
}

TEST_F(ImageCacheMetricsTest, Increment_TotalRequest_LoadsCorrectValue) {
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalRequest);
  EXPECT_EQ(
      ImageCacheMetrics::Instance().Load(MetricsCounter::TotalRequest), 1u);
}

TEST_F(ImageCacheMetricsTest, Increment_WithN_AddsN) {
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalCompleted, 5);
  EXPECT_EQ(
      ImageCacheMetrics::Instance().Load(MetricsCounter::TotalCompleted), 5u);
}

TEST_F(ImageCacheMetricsTest, Increment_MultipleCounters_Independent) {
  ImageCacheMetrics::Instance().Increment(MetricsCounter::CacheHits);
  ImageCacheMetrics::Instance().Increment(MetricsCounter::CacheMisses);
  ImageCacheMetrics::Instance().Increment(MetricsCounter::CacheHits);
  EXPECT_EQ(ImageCacheMetrics::Instance().Load(MetricsCounter::CacheHits), 2u);
  EXPECT_EQ(
      ImageCacheMetrics::Instance().Load(MetricsCounter::CacheMisses), 1u);
}

TEST_F(ImageCacheMetricsTest, Reset_ZerosAllCounters) {
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalRequest, 10);
  ImageCacheMetrics::Instance().Increment(MetricsCounter::CacheHits, 5);
  ImageCacheMetrics::Instance().Reset();
  EXPECT_EQ(
      ImageCacheMetrics::Instance().Load(MetricsCounter::TotalRequest), 0u);
  EXPECT_EQ(ImageCacheMetrics::Instance().Load(MetricsCounter::CacheHits), 0u);
}

TEST_F(ImageCacheMetricsTest, Snapshot_CapturesAllCounters) {
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalRequest, 3);
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalFailed, 2);
  auto snap = ImageCacheMetrics::Instance().Snapshot();
  EXPECT_EQ(snap.Get(MetricsCounter::TotalRequest), 3u);
  EXPECT_EQ(snap.Get(MetricsCounter::TotalFailed), 2u);
  EXPECT_EQ(snap.Get(MetricsCounter::TotalCompleted), 0u);
}

TEST_F(ImageCacheMetricsTest, Snapshot_IncludesObjectCounts) {
  ImageCacheMetrics::Instance().Objects().Increment(
      "TestClass", reinterpret_cast<const void*>(0x1));
  auto snap = ImageCacheMetrics::Instance().Snapshot();
  auto it = snap.objects.find("TestClass");
  ASSERT_NE(it, snap.objects.end());
  EXPECT_EQ(it->second, 1);
}

TEST_F(ImageCacheMetricsTest, Objects_ReturnsCounterForIncrementDecrement) {
  ImageCacheMetrics::Instance().Objects().Increment(
      "Obj", reinterpret_cast<const void*>(0x1));
  ImageCacheMetrics::Instance().Objects().Increment(
      "Obj", reinterpret_cast<const void*>(0x2));
  EXPECT_EQ(ImageCacheMetrics::Instance().Objects().GetCount("Obj"), 2);
  ImageCacheMetrics::Instance().Objects().Decrement(
      "Obj", reinterpret_cast<const void*>(0x1));
  EXPECT_EQ(ImageCacheMetrics::Instance().Objects().GetCount("Obj"), 1);
}
