/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheMemoryCache.h"
#include <gtest/gtest.h>
#include "ImageCacheDataCacheInternal.h"
#include "MemCacheCallback.h"

#include <memory>

using namespace ImageCachePro;

class ImageCacheMemoryCacheTest : public ::testing::Test {
 protected:
  ImageCacheMemoryCache* cache;

  void SetUp() override {
    cache = ImageCacheMemoryCache::GetInstance();
    cache->Clear();
  }

  std::shared_ptr<ImageCacheDataCacheInternal> makeEntry(size_t size = 100) {
    return std::make_shared<ImageCacheDataCacheInternal>(
        size, nullptr, nullptr);
  }
};

TEST_F(ImageCacheMemoryCacheTest, Put_ThenGet_ReturnsValue) {
  auto entry = makeEntry();
  cache->Put("key1", entry);
  auto got = cache->Get("key1");
  EXPECT_EQ(got, entry);
}

TEST_F(ImageCacheMemoryCacheTest, Get_NonExistentKey_ReturnsNull) {
  EXPECT_EQ(cache->Get("nonexistent"), nullptr);
}

TEST_F(ImageCacheMemoryCacheTest, Contains_ExistingKey_ReturnsTrue) {
  cache->Put("key1", makeEntry());
  EXPECT_TRUE(cache->Contains("key1"));
}

TEST_F(ImageCacheMemoryCacheTest, Contains_NonExistentKey_ReturnsFalse) {
  EXPECT_FALSE(cache->Contains("nonexistent"));
}

TEST_F(ImageCacheMemoryCacheTest, Remove_ExistingKey_ReturnsTrue) {
  cache->Put("key1", makeEntry());
  EXPECT_TRUE(cache->Remove("key1"));
  EXPECT_FALSE(cache->Contains("key1"));
}

TEST_F(ImageCacheMemoryCacheTest, Remove_NonExistentKey_ReturnsTrue) {
  EXPECT_TRUE(cache->Remove("nonexistent"));
}

TEST_F(ImageCacheMemoryCacheTest, Remove_ExistingKey_DecrementsCurrentMemory) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("key1", makeEntry(50));
  EXPECT_EQ(cache->GetCurrentMemory(), 50u);
  cache->Remove("key1");
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, Clear_RemovesAllEntries) {
  cache->Put("key1", makeEntry());
  cache->Put("key2", makeEntry());
  cache->Put("key3", makeEntry());
  cache->Clear();
  EXPECT_FALSE(cache->Contains("key1"));
  EXPECT_FALSE(cache->Contains("key2"));
  EXPECT_FALSE(cache->Contains("key3"));
}

TEST_F(ImageCacheMemoryCacheTest, Size_AfterPuts_ReturnsExactCount) {
  cache->Put("key1", makeEntry());
  cache->Put("key2", makeEntry());
  EXPECT_EQ(cache->Size(), 2u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_MultipleKeys_AllRetrievable) {
  cache->Put("a", makeEntry(10));
  cache->Put("b", makeEntry(20));
  cache->Put("c", makeEntry(30));
  EXPECT_EQ(cache->Get("a")->GetImageCacheSize(), 10u);
  EXPECT_EQ(cache->Get("b")->GetImageCacheSize(), 20u);
  EXPECT_EQ(cache->Get("c")->GetImageCacheSize(), 30u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_OverwriteExistingKey_UpdatesValue) {
  cache->Put("key1", makeEntry(10));
  cache->Put("key1", makeEntry(99));
  EXPECT_EQ(cache->Get("key1")->GetImageCacheSize(), 99u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_Overwrite_TracksMemoryCorrectly) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("key1", makeEntry(50));
  EXPECT_EQ(cache->GetCurrentMemory(), 50u);
  cache->Put("key1", makeEntry(80));
  EXPECT_EQ(cache->GetCurrentMemory(), 80u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_OverwriteSmaller_TracksMemoryCorrectly) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("key1", makeEntry(80));
  EXPECT_EQ(cache->GetCurrentMemory(), 80u);
  cache->Put("key1", makeEntry(30));
  EXPECT_EQ(cache->GetCurrentMemory(), 30u);
}

TEST_F(ImageCacheMemoryCacheTest, SetCacheLimit_SizeLimit_TriggersEviction) {
  cache->SetCacheLimit(3, 1000000);
  cache->Put("a", makeEntry(10));
  cache->Put("b", makeEntry(20));
  cache->Put("c", makeEntry(30));
  EXPECT_EQ(cache->Size(), 2u);
  cache->Put("d", makeEntry(40));
  EXPECT_EQ(cache->Size(), 2u);
}

TEST_F(ImageCacheMemoryCacheTest, SetCacheLimit_MemoryLimit_TriggersEviction) {
  cache->SetCacheLimit(100, 100);
  cache->Put("a", makeEntry(50));
  EXPECT_EQ(cache->Size(), 1u);
  cache->Put("b", makeEntry(50));
  EXPECT_EQ(cache->Size(), 1u);
}

TEST_F(
    ImageCacheMemoryCacheTest,
    SetCacheLimit_ThenPut_GetReturnsCorrectValue) {
  cache->SetCacheLimit(100, 100000);
  auto entry = makeEntry();
  cache->Put("key1", entry);
  auto got = cache->Get("key1");
  EXPECT_NE(got, nullptr);
  EXPECT_EQ(got, entry);
}

TEST_F(ImageCacheMemoryCacheTest, ClearHalf_RemovesExactlyHalf) {
  cache->SetCacheLimit(100, 1000000);
  cache->Put("k1", makeEntry(10));
  cache->Put("k2", makeEntry(20));
  cache->Put("k3", makeEntry(30));
  cache->Put("k4", makeEntry(40));
  EXPECT_EQ(cache->Size(), 4u);
  cache->ClearHalf();
  EXPECT_EQ(cache->Size(), 2u);
}

TEST_F(ImageCacheMemoryCacheTest, ClearHalf_SingleEntry_RemovesIt) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("only", makeEntry(10));
  EXPECT_EQ(cache->Size(), 1u);
  cache->ClearHalf();
  EXPECT_EQ(cache->Size(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, ClearHalf_EmptyCache_SizeStaysZero) {
  EXPECT_EQ(cache->Size(), 0u);
  cache->ClearHalf();
  EXPECT_EQ(cache->Size(), 0u);
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, ClearHalf_TracksMemoryCorrectly) {
  cache->SetCacheLimit(100, 1000000);
  cache->Put("k1", makeEntry(50));
  cache->Put("k2", makeEntry(50));
  cache->Put("k3", makeEntry(50));
  cache->Put("k4", makeEntry(50));
  EXPECT_EQ(cache->GetCurrentMemory(), 200u);
  cache->ClearHalf();
  EXPECT_EQ(cache->GetCurrentMemory(), 100u);
}

TEST_F(
    ImageCacheMemoryCacheTest,
    GetCurrentMemory_AfterPutAndRemove_TracksCorrectly) {
  cache->SetCacheLimit(100, 100000);
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
  cache->Put("a", makeEntry(30));
  EXPECT_EQ(cache->GetCurrentMemory(), 30u);
  cache->Put("b", makeEntry(40));
  EXPECT_EQ(cache->GetCurrentMemory(), 70u);
  cache->Remove("a");
  EXPECT_EQ(cache->GetCurrentMemory(), 40u);
}

TEST_F(ImageCacheMemoryCacheTest, GetInstance_ReturnsSameSingleton) {
  auto* inst1 = ImageCacheMemoryCache::GetInstance();
  auto* inst2 = ImageCacheMemoryCache::GetInstance();
  EXPECT_EQ(inst1, inst2);
}

// ---------------------------------------------------------------------------
// GetAllowMaxSizeLimit / GetAllowMaxMemory
// ---------------------------------------------------------------------------

TEST_F(
    ImageCacheMemoryCacheTest,
    GetAllowMaxSizeLimit_AfterSetCacheLimit_ReturnsLimit) {
  cache->SetCacheLimit(50, 100000);
  EXPECT_EQ(cache->GetAllowMaxSizeLimit(), 50u);
}

TEST_F(
    ImageCacheMemoryCacheTest,
    GetAllowMaxMemory_AfterSetCacheLimit_ReturnsMemory) {
  cache->SetCacheLimit(50, 200000);
  EXPECT_EQ(cache->GetAllowMaxMemory(), 200000u);
}

TEST_F(ImageCacheMemoryCacheTest, GetAllowMaxSizeLimit_ZeroMeansUnbounded) {
  cache->SetCacheLimit(0, 100000);
  EXPECT_EQ(cache->GetAllowMaxSizeLimit(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, GetAllowMaxMemory_ZeroUsesDefault) {
  cache->SetCacheLimit(10, 0);
  EXPECT_GT(cache->GetAllowMaxMemory(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, SetCacheLimit_CapsAt4096) {
  cache->SetCacheLimit(9999, 100000);
  EXPECT_EQ(cache->GetAllowMaxSizeLimit(), 4096u);
}

TEST_F(ImageCacheMemoryCacheTest, SetCacheLimit_CapsMemoryAt1GB) {
  cache->SetCacheLimit(10, 2ull * 1024 * 1024 * 1024);
  EXPECT_LE(cache->GetAllowMaxMemory(), 1024ull * 1024 * 1024);
}

// ---------------------------------------------------------------------------
// Put edge cases (IMAGE_CACHE_CHECK early returns)
// ---------------------------------------------------------------------------

TEST_F(ImageCacheMemoryCacheTest, Put_EmptyKey_DoesNotInsert) {
  auto entry = makeEntry();
  cache->Put("", entry);
  EXPECT_EQ(cache->Size(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_NullValue_DoesNotInsert) {
  cache->Put("key1", nullptr);
  EXPECT_EQ(cache->Size(), 0u);
}

// ---------------------------------------------------------------------------
// Remove with entrySize > currentMemory_ (clamp to 0 branch)
// ---------------------------------------------------------------------------

TEST_F(ImageCacheMemoryCacheTest, Remove_AfterClear_DecrementsToZero) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("k1", makeEntry(50));
  cache->Clear();
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
  cache->Remove("k1");
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
}

// ---------------------------------------------------------------------------
// RemoveTailEntry when cache is empty
// ---------------------------------------------------------------------------

TEST_F(ImageCacheMemoryCacheTest, SetCacheLimit_OnEmptyCache_NoCrash) {
  EXPECT_EQ(cache->Size(), 0u);
  cache->SetCacheLimit(1, 1);
  EXPECT_EQ(cache->Size(), 0u);
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
}

// ---------------------------------------------------------------------------
// GetMemoryCacheMaxEntrySize (free function)
// ---------------------------------------------------------------------------

TEST_F(
    ImageCacheMemoryCacheTest,
    GetMemoryCacheMaxEntrySize_ReturnsQuarterOfMaxMemory) {
  cache->SetCacheLimit(100, 400000);
  auto maxEntry = GetMemoryCacheMaxEntrySize();
  EXPECT_EQ(maxEntry, 100000u);
}

// ---------------------------------------------------------------------------
// Overwrite with different sizes (existing size > currentMemory_ branch)
// ---------------------------------------------------------------------------

TEST_F(
    ImageCacheMemoryCacheTest,
    Put_OverwriteLarger_AfterClear_TracksZeroMemory) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("k1", makeEntry(50));
  cache->Clear();
  // currentMemory_ is now 0, but entry still in LRU? No, Clear removes all.
  // Put new entry → hadExisting is false (cache was cleared)
  cache->Put("k1", makeEntry(30));
  EXPECT_EQ(cache->GetCurrentMemory(), 30u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_OverwriteWithLarger_IncreasesMemory) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("k1", makeEntry(30));
  EXPECT_EQ(cache->GetCurrentMemory(), 30u);
  cache->Put("k1", makeEntry(50));
  EXPECT_EQ(cache->GetCurrentMemory(), 50u);
}

TEST_F(ImageCacheMemoryCacheTest, Put_AfterEviction_CurrentMemoryCorrect) {
  cache->SetCacheLimit(3, 1000000);
  cache->Put("a", makeEntry(10));
  cache->Put("b", makeEntry(20));
  cache->Put("c", makeEntry(30));
  // Limit 3: after 3 puts, size is 2 (one evicted)
  EXPECT_EQ(cache->Size(), 2u);
  // "a" (oldest) should have been evicted
  EXPECT_FALSE(cache->Contains("a"));
  EXPECT_TRUE(cache->Contains("c"));
}

// ---------------------------------------------------------------------------
// MemCacheCallback integration (constructor registers a callback)
// ---------------------------------------------------------------------------

TEST_F(ImageCacheMemoryCacheTest, NotifyAll_CriticalLevel_ClearsCache) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("k1", makeEntry(10));
  cache->Put("k2", makeEntry(20));
  EXPECT_EQ(cache->Size(), 2u);
  MemCacheCallback::notifyAll(MEMORY_LEVEL_CRITICAL);
  EXPECT_EQ(cache->Size(), 0u);
  EXPECT_EQ(cache->GetCurrentMemory(), 0u);
}

TEST_F(ImageCacheMemoryCacheTest, NotifyAll_ModerateLevel_ClearsHalf) {
  cache->SetCacheLimit(100, 1000000);
  cache->Put("k1", makeEntry(10));
  cache->Put("k2", makeEntry(20));
  cache->Put("k3", makeEntry(30));
  cache->Put("k4", makeEntry(40));
  EXPECT_EQ(cache->Size(), 4u);
  EXPECT_EQ(cache->GetCurrentMemory(), 100u);
  MemCacheCallback::notifyAll(MEMORY_LEVEL_MODERATE);
  EXPECT_EQ(cache->Size(), 2u);
  EXPECT_EQ(cache->GetCurrentMemory(), 70u);
}

TEST_F(ImageCacheMemoryCacheTest, NotifyAll_LowLevel_ClearsHalfAndKeepsNewest) {
  cache->SetCacheLimit(100, 1000000);
  cache->Put("k1", makeEntry(10));
  cache->Put("k2", makeEntry(20));
  cache->Put("k3", makeEntry(30));
  cache->Put("k4", makeEntry(40));
  EXPECT_EQ(cache->Size(), 4u);
  MemCacheCallback::notifyAll(MEMORY_LEVEL_LOW);
  EXPECT_EQ(cache->Size(), 2u);
  EXPECT_TRUE(cache->Contains("k4"));
  EXPECT_FALSE(cache->Contains("k1"));
}

TEST_F(ImageCacheMemoryCacheTest, NotifyBackground_ClearsCache) {
  cache->SetCacheLimit(100, 100000);
  cache->Put("k1", makeEntry(10));
  cache->Put("k2", makeEntry(20));
  EXPECT_EQ(cache->Size(), 2u);
  MemCacheCallback::notifyBackground();
  EXPECT_EQ(cache->Size(), 0u);
}
