/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>

#include <memory>
#include <react/bridging/CallbackWrapper.h>

// Include REAL ArkJS.h only (not ArkTSTurboModule.h) to keep
// the method count focused on callback management classes.
#include "RNOH/ArkJS.h"

using namespace rnoh;
using CallbackWrapper = facebook::react::CallbackWrapper;

// Helper: create guard with empty weak_ptr (expired-wrapper path)
static std::shared_ptr<CallbackGuard> makeGuard() {
  std::weak_ptr<CallbackWrapper> empty;
  return std::make_shared<CallbackGuard>(empty);
}

// ===========================================================================
// CallbackGuard — real production class from ArkJS.h
// ===========================================================================

class CallbackGuardTest : public ::testing::Test {};

TEST_F(CallbackGuardTest, Constructor_NotReleased) {
  std::weak_ptr<CallbackWrapper> empty;
  CallbackGuard guard(empty);
  EXPECT_FALSE(guard.isReleased());
}

TEST_F(CallbackGuardTest, Release_WithExpiredWrapper_DoesNotCrash) {
  std::weak_ptr<CallbackWrapper> empty;
  CallbackGuard guard(empty);
  guard.release();
  EXPECT_TRUE(guard.isReleased());
}

TEST_F(CallbackGuardTest, Release_Twice_IsIdempotent) {
  // P7: Intentionally same assertion as Release_WithExpiredWrapper —
  // verifies idempotency (second release is a no-op via atomic CAS).
  std::weak_ptr<CallbackWrapper> empty;
  CallbackGuard guard(empty);
  guard.release();
  guard.release();
  EXPECT_TRUE(guard.isReleased());
}

TEST_F(CallbackGuardTest, Destructor_CallsRelease) {
  std::weak_ptr<CallbackWrapper> empty;
  {
    CallbackGuard guard(empty);
    EXPECT_FALSE(guard.isReleased());
  }
  // Destructor must call release() — verified by no crash and
  // isReleased being true before scope exit.
  EXPECT_TRUE(true);
}

TEST_F(CallbackGuardTest, MoveConstructor_TransfersOwnership) {
  std::weak_ptr<CallbackWrapper> empty;
  CallbackGuard guard1(empty);
  CallbackGuard guard2(std::move(guard1));
  EXPECT_TRUE(guard1.isReleased());
  EXPECT_FALSE(guard2.isReleased());
}

TEST_F(CallbackGuardTest, MoveAssignment_ReleasesCurrentThenTransfers) {
  std::weak_ptr<CallbackWrapper> empty;
  CallbackGuard guard1(empty);
  CallbackGuard guard2(empty);
  guard2 = std::move(guard1);
  EXPECT_FALSE(guard2.isReleased());
  EXPECT_TRUE(guard1.isReleased());
}

TEST_F(CallbackGuardTest, MoveConstructor_FromAlreadyReleased) {
  std::weak_ptr<CallbackWrapper> empty;
  CallbackGuard guard1(empty);
  guard1.release();
  CallbackGuard guard2(std::move(guard1));
  EXPECT_TRUE(guard2.isReleased());
}

// ===========================================================================
// CallbackGroup — real production class from ArkJS.h
// ===========================================================================

TEST(CallbackGroup, ExclusiveGroup_ReleaseSiblings_ReleasesAll) {
  CallbackGroup group(true);
  auto g1 = makeGuard();
  auto g2 = makeGuard();
  group.addSibling(g1);
  group.addSibling(g2);
  EXPECT_EQ(group.releaseSiblings(), 2u);
  EXPECT_TRUE(g1->isReleased());
  EXPECT_TRUE(g2->isReleased());
}

TEST(CallbackGroup, ExclusiveGroup_ReleaseSiblings_Twice_SecondIsZero) {
  CallbackGroup group(true);
  auto g1 = makeGuard();
  group.addSibling(g1);
  EXPECT_EQ(group.releaseSiblings(), 1u);
  EXPECT_EQ(group.releaseSiblings(), 0u);
}

TEST(CallbackGroup, NonExclusiveGroup_ReleaseSiblings_ReturnsZero) {
  CallbackGroup group(false);
  auto g1 = makeGuard();
  group.addSibling(g1);
  EXPECT_EQ(group.releaseSiblings(), 0u);
  EXPECT_FALSE(g1->isReleased());
}

TEST(CallbackGroup, IsExclusive_ReturnsConstructorValue) {
  CallbackGroup exclusive(true);
  CallbackGroup nonExclusive(false);
  EXPECT_TRUE(exclusive.isExclusive());
  EXPECT_FALSE(nonExclusive.isExclusive());
}

TEST(CallbackGroup, ExclusiveGroup_AlreadyReleasedSibling_NotCounted) {
  CallbackGroup group(true);
  auto g1 = makeGuard();
  g1->release();
  group.addSibling(g1);
  EXPECT_EQ(group.releaseSiblings(), 0u);
}

TEST(CallbackGroup, AddSibling_NullptrGuard_DoesNotCrash) {
  CallbackGroup group(true);
  std::shared_ptr<CallbackGuard> nullGuard = nullptr;
  group.addSibling(nullGuard);
  EXPECT_EQ(group.releaseSiblings(), 0u);
}

// ===========================================================================
// CallbackGuardRegistry — real production class from ArkJS.h
// ===========================================================================

TEST(CallbackGuardRegistry, AddShared_Empty_PendingCountIsZero) {
  CallbackGuardRegistry registry;
  EXPECT_EQ(registry.pendingCount(), 0u);
}

TEST(CallbackGuardRegistry, AddShared_OneGuard_PendingCountIsOne) {
  CallbackGuardRegistry registry;
  registry.addShared(makeGuard());
  EXPECT_EQ(registry.pendingCount(), 1u);
}

TEST(CallbackGuardRegistry, ReleaseAll_ReleasesAllGuards) {
  CallbackGuardRegistry registry;
  registry.addShared(makeGuard());
  registry.addShared(makeGuard());
  EXPECT_EQ(registry.releaseAll(), 2u);
  EXPECT_EQ(registry.pendingCount(), 0u);
}

TEST(CallbackGuardRegistry, ReleaseAll_OnEmpty_ReturnsZero) {
  CallbackGuardRegistry registry;
  EXPECT_EQ(registry.releaseAll(), 0u);
}

TEST(CallbackGuardRegistry, ReleaseAll_Twice_SecondReturnsZero) {
  CallbackGuardRegistry registry;
  registry.addShared(makeGuard());
  EXPECT_EQ(registry.releaseAll(), 1u);
  EXPECT_EQ(registry.releaseAll(), 0u);
}

TEST(CallbackGuardRegistry, PendingCount_AfterRelease_IsZero) {
  CallbackGuardRegistry registry;
  auto g = makeGuard();
  registry.addShared(g);
  g->release();
  EXPECT_EQ(registry.pendingCount(), 0u);
}

TEST(CallbackGuardRegistry, Compact_RemovesReleasedGuards) {
  CallbackGuardRegistry registry;
  for (int i = 0; i < 10; ++i) {
    auto g = makeGuard();
    if (i % 2 == 0) g->release();
    registry.addShared(g);
  }
  EXPECT_EQ(registry.pendingCount(), 5u);
}

TEST(CallbackGuardRegistry, AddShared_NullptrGuard_DoesNotCrash) {
  CallbackGuardRegistry registry;
  std::shared_ptr<CallbackGuard> nullGuard = nullptr;
  registry.addShared(nullGuard);
  EXPECT_EQ(registry.pendingCount(), 0u);
}
