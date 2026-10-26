/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "LRUCache11.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace lru11;

// Helper functor for cwalk: collects keys via pointer to external vector.
// (cwalk passes the functor by value into std::for_each, so a pointer
// indirection is required for the external side-effect to be observable.)
struct KeyCollector {
  std::vector<int>* out;
  void operator()(const KeyValuePair<int, int>& kv) {
    out->push_back(kv.key);
  }
};

// Aggregate struct for template variant test.
struct Point {
  int x;
  int y;
};

// ============================================================================
// Tier 1 — zero platform dependencies (header-only, std library only)
// Module: RNOH/ImageCache/src/MemoryCache/LRUCache11.h
//
// Tested class: lru11::Cache<Key, Value, Lock, Map> (3rd-party header-only LRU)
// Public methods under test: constructor, size, empty, clear, insert (×2),
// emplace, tryGet, tryGetCopy, tryGetRef, get/getCopy, getRef, remove,
// contains, getMaxSize, getElasticity, getMaxAllowedSize, getTail, resize,
// cwalk.
// ============================================================================

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

TEST(LRUCache, Constructor_DefaultParams_CreatesEmptyCache) {
  Cache<int, int> c;
  EXPECT_EQ(c.size(), 0u);
  EXPECT_TRUE(c.empty());
}

TEST(LRUCache, Constructor_MaxSizeOne_HoldsSingleElement) {
  Cache<int, int> c(1, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  EXPECT_EQ(c.size(), 1u);
  EXPECT_FALSE(c.contains(1));
  EXPECT_TRUE(c.contains(2));
}

TEST(LRUCache, Constructor_UnboundedMaxSizeZero_NeverPrunes) {
  Cache<int, int> c(0, 0);
  for (int i = 0; i < 100; ++i) {
    c.insert(i, i * 10);
  }
  EXPECT_EQ(c.size(), 100u);
}

// ---------------------------------------------------------------------------
// Insert
// ---------------------------------------------------------------------------

TEST(LRUCache, Insert_NewKey_IncreasesSize) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  EXPECT_EQ(c.size(), 1u);
  c.insert(2, 20);
  EXPECT_EQ(c.size(), 2u);
}

TEST(LRUCache, Insert_ExistingKey_UpdatesValueKeepsSize) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  c.insert(1, 99);
  EXPECT_EQ(c.size(), 1u);
  EXPECT_EQ(c.getCopy(1), 99);
}

TEST(LRUCache, Insert_ExceedingMaxSize_PrunesOldest) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(3, 30);
  EXPECT_EQ(c.size(), 2u);
  EXPECT_FALSE(c.contains(1));
  EXPECT_TRUE(c.contains(2));
  EXPECT_TRUE(c.contains(3));
}

TEST(LRUCache, Insert_NoPruneOverload_DoesNotPrune) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10, true);
  c.insert(2, 20, true);
  c.insert(3, 30, true);
  EXPECT_EQ(c.size(), 3u);
}

TEST(LRUCache, Emplace_MoveValue_InsertsAndPrunes) {
  Cache<int, std::string> c(2, 0);
  c.emplace(1, std::string("alpha"));
  c.emplace(2, std::string("beta"));
  c.emplace(3, std::string("gamma"));
  EXPECT_EQ(c.size(), 2u);
  EXPECT_FALSE(c.contains(1));
  EXPECT_TRUE(c.contains(3));
}

// ---------------------------------------------------------------------------
// TryGet / Get
// ---------------------------------------------------------------------------

TEST(LRUCache, TryGet_ExistingKey_ReturnsTrueAndRefreshesOrder) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  int out = 0;
  EXPECT_TRUE(c.tryGet(1, out));
  EXPECT_EQ(out, 10);
  c.insert(3, 30);
  EXPECT_TRUE(c.contains(1));
  EXPECT_FALSE(c.contains(2));
}

TEST(LRUCache, TryGet_MissingKey_ReturnsFalse) {
  Cache<int, int> c(10);
  int out = 0;
  EXPECT_FALSE(c.tryGet(99, out));
}

TEST(LRUCache, TryGetCopy_ExistingKey_ReturnsTrueAndValue) {
  Cache<int, int> c(10);
  c.insert(5, 55);
  int out = 0;
  EXPECT_TRUE(c.tryGetCopy(5, out));
  EXPECT_EQ(out, 55);
}

TEST(LRUCache, TryGetRef_ExistingKey_ReturnsTrueAndValue) {
  Cache<int, int> c(10);
  c.insert(7, 77);
  int out = 0;
  EXPECT_TRUE(c.tryGetRef(7, out));
  EXPECT_EQ(out, 77);
}

TEST(LRUCache, Get_BackwardCompat_ReturnsValue) {
  Cache<int, int> c(10);
  c.insert(9, 99);
  EXPECT_EQ(c.get(9), 99);
}

TEST(LRUCache, GetCopy_ExistingKey_ReturnsValue) {
  Cache<int, int> c(10);
  c.insert(1, 42);
  EXPECT_EQ(c.getCopy(1), 42);
}

TEST(LRUCache, GetCopy_MissingKey_ThrowsKeyNotFound) {
  Cache<int, int> c(10);
  EXPECT_THROW(c.getCopy(99), KeyNotFound);
}

TEST(LRUCache, GetRef_ExistingKey_ReturnsValue) {
  Cache<int, int> c(10);
  c.insert(1, 100);
  const auto& ref = c.getRef(1);
  EXPECT_EQ(ref, 100);
}

// ---------------------------------------------------------------------------
// Remove / Contains
// ---------------------------------------------------------------------------

TEST(LRUCache, Remove_ExistingKey_ReturnsTrueAndDecreasesSize) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  c.insert(2, 20);
  EXPECT_TRUE(c.remove(1));
  EXPECT_EQ(c.size(), 1u);
  EXPECT_FALSE(c.contains(1));
}

TEST(LRUCache, Remove_MissingKey_ReturnsFalse) {
  Cache<int, int> c(10);
  EXPECT_FALSE(c.remove(99));
}

TEST(LRUCache, Contains_ExistingKey_ReturnsTrue) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  EXPECT_TRUE(c.contains(1));
}

TEST(LRUCache, Contains_MissingKey_ReturnsFalse) {
  Cache<int, int> c(10);
  EXPECT_FALSE(c.contains(99));
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(LRUCache, Clear_ResetsSizeToZero) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  c.insert(2, 20);
  EXPECT_EQ(c.size(), 2u);
  c.clear();
  EXPECT_EQ(c.size(), 0u);
  EXPECT_TRUE(c.empty());
}

TEST(LRUCache, Clear_ThenInsert_WorksAgain) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  c.clear();
  c.insert(2, 20);
  EXPECT_EQ(c.size(), 1u);
  EXPECT_TRUE(c.contains(2));
}

// ---------------------------------------------------------------------------
// GetTail
// ---------------------------------------------------------------------------

TEST(LRUCache, GetTail_NonEmptyCache_ReturnsOldestElement) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(3, 30);
  int k = 0, v = 0;
  EXPECT_TRUE(c.getTail(k, v));
  EXPECT_EQ(k, 1);
  EXPECT_EQ(v, 10);
}

TEST(LRUCache, GetTail_EmptyCache_ReturnsFalse) {
  Cache<int, int> c(10);
  int k = 0, v = 0;
  EXPECT_FALSE(c.getTail(k, v));
}

// ---------------------------------------------------------------------------
// Resize (note: resize only updates maxSize_, does not trigger prune)
// ---------------------------------------------------------------------------

TEST(LRUCache, Resize_Smaller_NextInsertPrunesToNewMax) {
  Cache<int, int> c(10, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(3, 30);
  EXPECT_EQ(c.size(), 3u);
  c.resize(2);
  EXPECT_EQ(c.size(), 3u);
  c.insert(4, 40);
  EXPECT_EQ(c.size(), 2u);
  EXPECT_FALSE(c.contains(1));
  EXPECT_FALSE(c.contains(2));
  EXPECT_TRUE(c.contains(3));
  EXPECT_TRUE(c.contains(4));
}

TEST(LRUCache, Resize_Larger_DoesNotPrune) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  EXPECT_EQ(c.size(), 2u);
  c.resize(10);
  c.insert(3, 30);
  EXPECT_EQ(c.size(), 3u);
  EXPECT_TRUE(c.contains(1));
}

// ---------------------------------------------------------------------------
// Config getters
// ---------------------------------------------------------------------------

TEST(LRUCache, GetMaxSize_GetElasticity_GetMaxAllowedSize_ReturnConfig) {
  Cache<int, int> c(5, 3);
  EXPECT_EQ(c.getMaxSize(), 5u);
  EXPECT_EQ(c.getElasticity(), 3u);
  EXPECT_EQ(c.getMaxAllowedSize(), 8u);
}

// ---------------------------------------------------------------------------
// LRU order
// ---------------------------------------------------------------------------

TEST(LRUCache, LRUOrder_AfterGet_MovesToFront) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  int out = 0;
  c.tryGet(1, out);
  c.insert(3, 30);
  EXPECT_TRUE(c.contains(1));
  EXPECT_FALSE(c.contains(2));
  EXPECT_TRUE(c.contains(3));
}

TEST(LRUCache, Insert_ExistingKey_RefreshesToFront) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(1, 99);
  c.insert(3, 30);
  EXPECT_TRUE(c.contains(1));
  EXPECT_FALSE(c.contains(2));
  EXPECT_EQ(c.getCopy(1), 99);
}

// ---------------------------------------------------------------------------
// Elasticity
// ---------------------------------------------------------------------------

TEST(LRUCache, Elasticity_AllowsGrowthBeforePrune) {
  Cache<int, int> c(2, 1);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(3, 30);
  EXPECT_EQ(c.size(), 3u);
  c.insert(4, 40);
  EXPECT_EQ(c.size(), 2u);
  EXPECT_FALSE(c.contains(1));
  EXPECT_FALSE(c.contains(2));
  EXPECT_TRUE(c.contains(3));
  EXPECT_TRUE(c.contains(4));
}

// ---------------------------------------------------------------------------
// Cwalk
// ---------------------------------------------------------------------------

TEST(LRUCache, Cwalk_VisitsAllEntries) {
  Cache<int, int> c(10);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(3, 30);
  std::vector<int> visited;
  KeyCollector col{&visited};
  c.cwalk(col);
  EXPECT_EQ(visited.size(), 3u);
}

// ---------------------------------------------------------------------------
// Batch insert / prune count
// ---------------------------------------------------------------------------

TEST(LRUCache, Insert_ManyKeys_PruneCountCorrect) {
  Cache<int, int> c(5, 0);
  for (int i = 0; i < 20; ++i) {
    c.insert(i, i * 10);
  }
  EXPECT_EQ(c.size(), 5u);
  EXPECT_FALSE(c.contains(0));
  EXPECT_FALSE(c.contains(14));
  EXPECT_TRUE(c.contains(15));
  EXPECT_TRUE(c.contains(19));
}

// ---------------------------------------------------------------------------
// Template variants (P4: at least two instantiation types)
// ---------------------------------------------------------------------------

TEST(LRUCache, TemplateVariant_StringKeyStringValue_Works) {
  Cache<std::string, std::string> c(3, 0);
  c.insert("a", "alpha");
  c.insert("b", "beta");
  EXPECT_EQ(c.size(), 2u);
  EXPECT_EQ(c.getCopy("a"), "alpha");
  c.insert("c", "gamma");
  c.insert("d", "delta");
  // getCopy("a") refreshed "a" to front; oldest is "b" → evicted
  EXPECT_TRUE(c.contains("a"));
  EXPECT_FALSE(c.contains("b"));
  EXPECT_TRUE(c.contains("d"));
}

TEST(LRUCache, TemplateVariant_IntKeyStructValue_Works) {
  Cache<int, Point> c(3, 0);
  c.insert(1, Point{1, 2});
  c.insert(2, Point{3, 4});
  EXPECT_EQ(c.size(), 2u);
  auto p = c.getCopy(1);
  EXPECT_EQ(p.x, 1);
  EXPECT_EQ(p.y, 2);
  c.insert(3, Point{5, 6});
  c.insert(4, Point{7, 8});
  EXPECT_TRUE(c.contains(1));
  EXPECT_FALSE(c.contains(2));
  EXPECT_TRUE(c.contains(4));
}

TEST(LRUCache, Clear_EmptyCache_NoCrash) {
  Cache<int, int> c;
  c.clear();
  EXPECT_EQ(c.size(), 0u);
}

TEST(LRUCache, Resize_ToZero_BecomesUnbounded) {
  Cache<int, int> c(2, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  c.insert(3, 30);
  EXPECT_EQ(c.size(), 2u);
  c.resize(0);
  c.insert(4, 40);
  c.insert(5, 50);
  EXPECT_EQ(c.size(), 4u);
}

TEST(LRUCache, Resize_ToSameSize_NoChange) {
  Cache<int, int> c(3, 0);
  c.insert(1, 10);
  c.insert(2, 20);
  c.resize(3);
  EXPECT_EQ(c.size(), 2u);
  EXPECT_TRUE(c.contains(1));
  EXPECT_TRUE(c.contains(2));
}

TEST(LRUCache, Emplace_ExistingKey_UpdatesValue) {
  Cache<int, int> c(10, 0);
  c.emplace(1, 10);
  c.emplace(1, 20);
  EXPECT_EQ(c.size(), 1u);
  EXPECT_EQ(c.getCopy(1), 20);
}

TEST(LRUCache, Insert_AfterRemove_SameKey_Works) {
  Cache<int, int> c(10, 0);
  c.insert(1, 10);
  c.remove(1);
  EXPECT_FALSE(c.contains(1));
  c.insert(1, 20);
  EXPECT_TRUE(c.contains(1));
  EXPECT_EQ(c.getCopy(1), 20);
}

TEST(LRUCache, TryGet_MissingKey_OutputUnchanged) {
  Cache<int, int> c;
  int vOut = 999;
  EXPECT_FALSE(c.tryGet(1, vOut));
  EXPECT_EQ(vOut, 999);
}

// ---------------------------------------------------------------------------
// KeyValuePair struct + cwalk with KeyValuePair
// ---------------------------------------------------------------------------

TEST(LRUCache, KeyValuePair_StructHoldsKeyAndValue) {
  KeyValuePair<int, std::string> kv{42, "hello"};
  EXPECT_EQ(kv.key, 42);
  EXPECT_EQ(kv.value, "hello");
}

TEST(LRUCache, GetMaxSize_AfterResize_ReturnsNewMax) {
  Cache<int, int> c(5, 0);
  EXPECT_EQ(c.getMaxSize(), 5u);
  c.resize(10);
  EXPECT_EQ(c.getMaxSize(), 10u);
  EXPECT_NE(c.getMaxSize(), 5u);
}

TEST(LRUCache, GetElasticity_DefaultZero_NoElasticity) {
  Cache<int, int> c(5, 0);
  EXPECT_EQ(c.getElasticity(), 0u);
}

TEST(LRUCache, GetMaxAllowedSize_EqualsMaxPlusElasticity) {
  Cache<int, int> c(5, 3);
  EXPECT_EQ(c.getMaxAllowedSize(), c.getMaxSize() + c.getElasticity());
}
