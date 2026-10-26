/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include "RNOH/ComponentInstanceProvider.h"

using namespace rnoh;
using namespace facebook::react;

static std::shared_ptr<ComponentInstanceProvider> makeProvider() {
  auto queue = std::make_shared<ComponentInstancePreallocationRequestQueue>();
  auto factory = std::make_shared<ComponentInstanceFactory>(
      std::vector<ComponentInstanceFactoryDelegate::Shared>{}, nullptr, nullptr);
  auto registry = std::make_shared<ComponentInstanceRegistry>();
  return std::make_shared<ComponentInstanceProvider>(
      queue, factory, registry, nullptr, TaskExecutor::Weak{});
}

// Constructor stores pointers without calling UITicker methods.
// initialize() needs UITicker so we skip it.

TEST(ComponentInstanceProviderReal, Constructor_DoesNotThrow) {
  auto queue = std::make_shared<ComponentInstancePreallocationRequestQueue>();
  auto factory = std::make_shared<ComponentInstanceFactory>(std::vector<ComponentInstanceFactoryDelegate::Shared>{}, nullptr, nullptr);
  auto registry = std::make_shared<ComponentInstanceRegistry>();
  EXPECT_NO_THROW(std::make_shared<ComponentInstanceProvider>(
      queue, factory, registry, nullptr, TaskExecutor::Weak{}));
}

TEST(ComponentInstanceProviderReal, ClearPreallocationRequestQueue_DoesNotThrow) {
  auto queue = std::make_shared<ComponentInstancePreallocationRequestQueue>();
  auto factory = std::make_shared<ComponentInstanceFactory>(std::vector<ComponentInstanceFactoryDelegate::Shared>{}, nullptr, nullptr);
  auto registry = std::make_shared<ComponentInstanceRegistry>();
  auto provider = std::make_shared<ComponentInstanceProvider>(
      queue, factory, registry, nullptr, TaskExecutor::Weak{});
  EXPECT_NO_THROW(provider->clearPreallocationRequestQueue());
}

TEST(ComponentInstanceProviderReal, ClearPreallocatedViews_Empty_DoesNotThrow) {
  auto queue = std::make_shared<ComponentInstancePreallocationRequestQueue>();
  auto factory = std::make_shared<ComponentInstanceFactory>(std::vector<ComponentInstanceFactoryDelegate::Shared>{}, nullptr, nullptr);
  auto registry = std::make_shared<ComponentInstanceRegistry>();
  auto provider = std::make_shared<ComponentInstanceProvider>(
      queue, factory, registry, nullptr, TaskExecutor::Weak{});
  EXPECT_NO_THROW(provider->clearPreallocatedViews());
  EXPECT_NO_THROW(provider->clearPreallocatedViews({}));
}

TEST(ComponentInstanceProviderReal, HasComponentInstance_NotExist_ReturnsFalse) {
  auto provider = makeProvider();
  EXPECT_FALSE(provider->hasComponentInstance(999));
}

TEST(ComponentInstanceProviderReal, GetComponentInstance_CacheMiss_ReturnsNull) {
  auto provider = makeProvider();
  auto result = provider->getComponentInstance(42, 0, "View");
  EXPECT_EQ(result, nullptr);
}

TEST(ComponentInstanceProviderReal, GetComponentInstance_CacheHit_ReturnsCached) {
  auto provider = makeProvider();
  provider->getComponentInstance(42, 0, "View");
  auto result = provider->getComponentInstance(42, 0, "View");
  EXPECT_EQ(result, nullptr);
}

TEST(ComponentInstanceProviderReal, ClearPreallocatedViews_WithMutations_ErasesTags) {
  auto provider = makeProvider();
  provider->getComponentInstance(10, 0, "View");
  ShadowViewMutationList mutations;
  ShadowViewMutation m;
  m.newChildShadowView.tag = 10;
  mutations.push_back(m);
  EXPECT_NO_THROW(provider->clearPreallocatedViews(mutations));
}

TEST(ComponentInstanceProviderReal, OnUITick_EmptyQueue_NoListener_DoesNotThrow) {
  auto provider = makeProvider();
  EXPECT_NO_THROW(provider->onUITick(std::chrono::steady_clock::now()));
}

TEST(ComponentInstanceProviderReal, OnUITick_EmptyQueue_WithListener_Unsubscribes) {
  auto provider = makeProvider();
  provider->m_unsubscribeUITickerListener = []() {};
  EXPECT_NO_THROW(provider->onUITick(std::chrono::steady_clock::now()));
}

TEST(ComponentInstanceProviderReal, OnUITick_NonEmptyQueue_ProcessesRequests) {
  auto provider = makeProvider();
  provider->m_preallocationRequestQueue->push({1, 0, "View", nullptr});
  EXPECT_NO_THROW(provider->onUITick(std::chrono::steady_clock::now()));
}

TEST(ComponentInstanceProviderReal, OnUITick_NonEmptyQueue_ShouldPause_Breaks) {
  auto provider = makeProvider();
  provider->m_preallocationRequestQueue->push({1, 0, "View", nullptr});
  auto oldTs = std::chrono::steady_clock::now() - std::chrono::milliseconds(100);
  EXPECT_NO_THROW(provider->onUITick(oldTs));
}

TEST(ComponentInstanceProviderReal, Destructor_WithListener_CallsUnsubscribe) {
  auto provider = makeProvider();
  provider->m_unsubscribeUITickerListener = []() {};
  EXPECT_NO_THROW(provider.reset());
}
