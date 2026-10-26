#include "FreezeStackCollector.h"
#include <gtest/gtest.h>

using namespace rnoh;

class FreezeStackCollectorTest : public ::testing::Test {
 protected:
  static constexpr size_t TEST_INSTANCE_ID = 10042;
  static constexpr size_t UNREGISTERED_ID = 77777;

  void SetUp() override {
    auto& collector = FreezeStackCollector::getInstance();
    collector.registerRuntime({TEST_INSTANCE_ID, reinterpret_cast<void*>(0x1)});
    collector.unregisterRuntime(TEST_INSTANCE_ID);
  }
};

TEST_F(FreezeStackCollectorTest, GetInstance_ReturnsSameInstance) {
  auto& instance1 = FreezeStackCollector::getInstance();
  auto& instance2 = FreezeStackCollector::getInstance();
  EXPECT_EQ(&instance1, &instance2);
}

TEST_F(FreezeStackCollectorTest, CacheStackResult_EmptyStack_DoesNotCache) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.cacheStackResult(TEST_INSTANCE_ID, "");
  EXPECT_TRUE(collector.getCachedStack(TEST_INSTANCE_ID).empty());
}

TEST_F(FreezeStackCollectorTest, CacheStackResult_ValidStack_StoresStack) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.cacheStackResult(TEST_INSTANCE_ID, "stack1");
  EXPECT_EQ(collector.getCachedStack(TEST_INSTANCE_ID), "stack1");
}

TEST_F(
    FreezeStackCollectorTest,
    CacheStackResult_OverwriteExisting_UpdatesStack) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.cacheStackResult(TEST_INSTANCE_ID, "old");
  collector.cacheStackResult(TEST_INSTANCE_ID, "new");
  EXPECT_EQ(collector.getCachedStack(TEST_INSTANCE_ID), "new");
}

TEST_F(
    FreezeStackCollectorTest,
    GetCachedStackWithAge_NotCached_ReturnsEmptyAndNegativeAge) {
  auto& collector = FreezeStackCollector::getInstance();
  auto result = collector.getCachedStackWithAge(UNREGISTERED_ID);
  EXPECT_TRUE(result.stack.empty());
  EXPECT_EQ(result.ageMs, -1);
}

TEST_F(
    FreezeStackCollectorTest,
    GetCachedStackWithAge_Cached_ReturnsStackAndNonNegativeAge) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.cacheStackResult(TEST_INSTANCE_ID, "test_stack");
  auto result = collector.getCachedStackWithAge(TEST_INSTANCE_ID);
  EXPECT_EQ(result.stack, "test_stack");
  EXPECT_GE(result.ageMs, 0);
}

TEST_F(FreezeStackCollectorTest, GetCachedStack_NotCached_ReturnsEmptyString) {
  auto& collector = FreezeStackCollector::getInstance();
  EXPECT_TRUE(collector.getCachedStack(UNREGISTERED_ID).empty());
}

TEST_F(FreezeStackCollectorTest, GetCachedStack_Cached_ReturnsStack) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.cacheStackResult(TEST_INSTANCE_ID, "cached_stack");
  EXPECT_EQ(collector.getCachedStack(TEST_INSTANCE_ID), "cached_stack");
}

TEST_F(
    FreezeStackCollectorTest,
    RegisterRuntime_WithNullHermes_ErasesEntryAndClearsCache) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.registerRuntime({TEST_INSTANCE_ID, reinterpret_cast<void*>(0x1)});
  collector.cacheStackResult(TEST_INSTANCE_ID, "cached_data");
  EXPECT_EQ(collector.getCachedStack(TEST_INSTANCE_ID), "cached_data");
  collector.registerRuntime({TEST_INSTANCE_ID, nullptr});
  EXPECT_TRUE(collector.getCachedStack(TEST_INSTANCE_ID).empty());
}

TEST_F(
    FreezeStackCollectorTest,
    RegisterRuntime_WithNonNullHermes_RegistersAndClearsCache) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.cacheStackResult(TEST_INSTANCE_ID, "pre_cache");
  EXPECT_EQ(collector.getCachedStack(TEST_INSTANCE_ID), "pre_cache");
  collector.registerRuntime({TEST_INSTANCE_ID, reinterpret_cast<void*>(0x1)});
  EXPECT_TRUE(collector.getCachedStack(TEST_INSTANCE_ID).empty());
}

TEST_F(FreezeStackCollectorTest, UnregisterRuntime_NotRegistered_IsNoOp) {
  auto& collector = FreezeStackCollector::getInstance();
  EXPECT_NO_THROW(collector.unregisterRuntime(UNREGISTERED_ID));
  EXPECT_TRUE(collector.getCachedStack(UNREGISTERED_ID).empty());
}

TEST_F(
    FreezeStackCollectorTest,
    UnregisterRuntime_Registered_ErasesEntryAndClearsCache) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.registerRuntime({TEST_INSTANCE_ID, reinterpret_cast<void*>(0x1)});
  collector.cacheStackResult(TEST_INSTANCE_ID, "to_be_cleared");
  EXPECT_EQ(collector.getCachedStack(TEST_INSTANCE_ID), "to_be_cleared");
  collector.unregisterRuntime(TEST_INSTANCE_ID);
  EXPECT_TRUE(collector.getCachedStack(TEST_INSTANCE_ID).empty());
}

TEST_F(
    FreezeStackCollectorTest,
    CaptureStackFromProfiler_NotRegistered_ReturnsEmpty) {
  auto& collector = FreezeStackCollector::getInstance();
  EXPECT_TRUE(collector.captureStackFromProfiler(UNREGISTERED_ID).empty());
}

TEST_F(
    FreezeStackCollectorTest,
    CaptureStackFromProfiler_RegisteredWithNullHermes_ReturnsEmpty) {
  auto& collector = FreezeStackCollector::getInstance();
  collector.registerRuntime({TEST_INSTANCE_ID, nullptr});
  EXPECT_TRUE(collector.captureStackFromProfiler(TEST_INSTANCE_ID).empty());
  EXPECT_TRUE(collector.getCachedStack(TEST_INSTANCE_ID).empty());
}
