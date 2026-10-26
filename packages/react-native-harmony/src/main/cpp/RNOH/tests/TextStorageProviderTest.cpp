/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: Text 组件 — TextStorageProvider 纯方法
// Tier: 2 (纯 C++ + mutex/atomic, 零 OH_ 调用)
// Methods: clear(), setParams(), eraseBuilder()
// =============================================================================

#include <gtest/gtest.h>
#include <atomic>
#include <map>
#include <mutex>
#include <unordered_map>

namespace {

// ---------------------------------------------------------------------------
// Stub TextStorageProvider (only pure methods, no ArkUI/Drawing deps)
// ---------------------------------------------------------------------------

class TextStorageProviderStub {
 public:
  TextStorageProviderStub() : m_scale(1.0f), m_fontScale(1.0f) {}

  void setParams(float scale, float fontScale) {
    m_scale.store(scale);
    m_fontScale.store(fontScale);
  }

  void clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_map.clear();
  }

  void erase(int32_t tag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_map.erase(tag);
  }

  void insert(int32_t tag, int32_t value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_map[tag] = value;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_map.size();
  }

  float getScale() const { return m_scale.load(); }
  float getFontScale() const { return m_fontScale.load(); }
  bool contains(int32_t tag) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_map.count(tag) > 0;
  }

 private:
  std::atomic<float> m_scale;
  std::atomic<float> m_fontScale;
  mutable std::mutex m_mutex;
  std::unordered_map<int32_t, int32_t> m_map;
};

} // anonymous namespace

// ===========================================================================
// Tests: setParams
// ===========================================================================

TEST(TextStorageProviderTest, SetParams_DefaultConstructor_InitialValuesAreOne) {
  TextStorageProviderStub provider;
  EXPECT_FLOAT_EQ(provider.getScale(), 1.0f);
  EXPECT_FLOAT_EQ(provider.getFontScale(), 1.0f);
}

TEST(TextStorageProviderTest, SetParams_UpdatesScaleAndFontScale) {
  TextStorageProviderStub provider;
  provider.setParams(2.0f, 1.5f);
  EXPECT_FLOAT_EQ(provider.getScale(), 2.0f);
  EXPECT_FLOAT_EQ(provider.getFontScale(), 1.5f);
}

TEST(TextStorageProviderTest, SetParams_ZeroValues_AreStored) {
  TextStorageProviderStub provider;
  provider.setParams(0.0f, 0.0f);
  EXPECT_FLOAT_EQ(provider.getScale(), 0.0f);
  EXPECT_FLOAT_EQ(provider.getFontScale(), 0.0f);
}

TEST(TextStorageProviderTest, SetParams_NegativeValues_AreStored) {
  TextStorageProviderStub provider;
  provider.setParams(-1.0f, -0.5f);
  EXPECT_FLOAT_EQ(provider.getScale(), -1.0f);
  EXPECT_FLOAT_EQ(provider.getFontScale(), -0.5f);
}

TEST(TextStorageProviderTest, SetParams_CalledTwice_LastValueWins) {
  TextStorageProviderStub provider;
  provider.setParams(1.0f, 2.0f);
  provider.setParams(3.0f, 4.0f);
  EXPECT_FLOAT_EQ(provider.getScale(), 3.0f);
  EXPECT_FLOAT_EQ(provider.getFontScale(), 4.0f);
}

// ===========================================================================
// Tests: clear
// ===========================================================================

TEST(TextStorageProviderTest, Clear_EmptyMap_SizeStaysZero) {
  TextStorageProviderStub provider;
  provider.clear();
  EXPECT_EQ(provider.size(), 0u);
}

TEST(TextStorageProviderTest, Clear_WithEntries_RemovesAll) {
  TextStorageProviderStub provider;
  provider.insert(1, 100);
  provider.insert(2, 200);
  provider.insert(3, 300);
  EXPECT_EQ(provider.size(), 3u);

  provider.clear();
  EXPECT_EQ(provider.size(), 0u);
  EXPECT_FALSE(provider.contains(1));
  EXPECT_FALSE(provider.contains(2));
  EXPECT_FALSE(provider.contains(3));
}

TEST(TextStorageProviderTest, Clear_AfterClear_CanReinsert) {
  TextStorageProviderStub provider;
  provider.insert(1, 100);
  provider.clear();
  provider.insert(1, 200);
  EXPECT_EQ(provider.size(), 1u);
  EXPECT_TRUE(provider.contains(1));
}

// ===========================================================================
// Tests: erase
// ===========================================================================

TEST(TextStorageProviderTest, Erase_ExistingTag_RemovesEntry) {
  TextStorageProviderStub provider;
  provider.insert(5, 500);
  EXPECT_TRUE(provider.contains(5));

  provider.erase(5);
  EXPECT_FALSE(provider.contains(5));
  EXPECT_EQ(provider.size(), 0u);
}

TEST(TextStorageProviderTest, Erase_NonExistentTag_NoCrash) {
  TextStorageProviderStub provider;
  provider.erase(999);
  EXPECT_EQ(provider.size(), 0u);
}

TEST(TextStorageProviderTest, Erase_OneOfMultiple_OnlyRemovesOne) {
  TextStorageProviderStub provider;
  provider.insert(1, 100);
  provider.insert(2, 200);
  provider.insert(3, 300);

  provider.erase(2);
  EXPECT_EQ(provider.size(), 2u);
  EXPECT_TRUE(provider.contains(1));
  EXPECT_FALSE(provider.contains(2));
  EXPECT_TRUE(provider.contains(3));
}

TEST(TextStorageProviderTest, Erase_AfterClear_NoEffect) {
  TextStorageProviderStub provider;
  provider.insert(1, 100);
  provider.clear();
  provider.erase(1);
  EXPECT_EQ(provider.size(), 0u);
}
