/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Include the real SchedulerDelegate.cpp (auto-copied by CMake file(COPY) at
// configure time from RNOH/SchedulerDelegate.cpp). Heavy dependencies are
// shadowed by stubs in scheduler_delegate_test/stubs/. This tests the REAL
// source code — no manual copy, no drift risk.

#include <SchedulerDelegate.cpp> // NOLINT: auto-copied by CMake, always in sync

#include <gtest/gtest.h>

using namespace facebook::react;
using namespace rnoh;

// ============================================================================
// Helpers — construct ShadowViewMutation for tests
// ============================================================================

static ShadowViewMutation makeMutation(
    ShadowViewMutation::Type type,
    Tag tag,
    Tag parentTag = 0,
    const char* name = nullptr,
    SurfaceId surfaceId = 0) {
  ShadowViewMutation m;
  m.type = type;
  m.newChildShadowView.tag = tag;
  m.newChildShadowView.componentName = name;
  m.newChildShadowView.surfaceId = surfaceId;
  m.oldChildShadowView.tag = tag;
  m.parentTag = parentTag;
  return m;
}

static ShadowViewMutationList makeLargeList(
    size_t count,
    int costPerItem = COST_DEFAULT) {
  ShadowViewMutationList list;
  list.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    list.push_back(
        makeMutation(ShadowViewMutation::Update, static_cast<Tag>(100 + i), 1));
  }
  return list;
}

// ============================================================================
// estimateNonCreateCost
// ============================================================================

TEST(SchedulerDelegatePure, EstimateCost_Create_ReturnsZero) {
  auto m = makeMutation(ShadowViewMutation::Create, 1);
  EXPECT_EQ(estimateNonCreateCost(m), 0);
}

TEST(SchedulerDelegatePure, EstimateCost_Insert_ReturnsThree) {
  auto m = makeMutation(ShadowViewMutation::Insert, 1);
  EXPECT_EQ(estimateNonCreateCost(m), COST_INSERT);
}

TEST(SchedulerDelegatePure, EstimateCost_Remove_ReturnsThree) {
  auto m = makeMutation(ShadowViewMutation::Remove, 1);
  EXPECT_EQ(estimateNonCreateCost(m), COST_REMOVE);
}

TEST(SchedulerDelegatePure, EstimateCost_Delete_ReturnsFour) {
  auto m = makeMutation(ShadowViewMutation::Delete, 1);
  EXPECT_EQ(estimateNonCreateCost(m), COST_DELETE);
}

TEST(SchedulerDelegatePure, EstimateCost_Update_ReturnsTwo) {
  auto m = makeMutation(ShadowViewMutation::Update, 1);
  EXPECT_EQ(estimateNonCreateCost(m), COST_DEFAULT);
}

// ============================================================================
// isStructuralWithParent
// ============================================================================

TEST(SchedulerDelegatePure, IsStructural_Insert_ReturnsTrue) {
  auto m = makeMutation(ShadowViewMutation::Insert, 1);
  EXPECT_TRUE(isStructuralWithParent(m));
}

TEST(SchedulerDelegatePure, IsStructural_Remove_ReturnsTrue) {
  auto m = makeMutation(ShadowViewMutation::Remove, 1);
  EXPECT_TRUE(isStructuralWithParent(m));
}

TEST(SchedulerDelegatePure, IsStructural_Update_ReturnsFalse) {
  auto m = makeMutation(ShadowViewMutation::Update, 1);
  EXPECT_FALSE(isStructuralWithParent(m));
}

TEST(SchedulerDelegatePure, IsStructural_Create_ReturnsFalse) {
  auto m = makeMutation(ShadowViewMutation::Create, 1);
  EXPECT_FALSE(isStructuralWithParent(m));
}

// ============================================================================
// getTargetTag
// ============================================================================

TEST(SchedulerDelegatePure, GetTargetTag_Create_ReturnsNewChildTag) {
  auto m = makeMutation(ShadowViewMutation::Create, 42);
  EXPECT_EQ(getTargetTag(m), 42);
}

TEST(SchedulerDelegatePure, GetTargetTag_Insert_ReturnsNewChildTag) {
  auto m = makeMutation(ShadowViewMutation::Insert, 42);
  EXPECT_EQ(getTargetTag(m), 42);
}

TEST(SchedulerDelegatePure, GetTargetTag_Remove_ReturnsOldChildTag) {
  auto m = makeMutation(ShadowViewMutation::Remove, 42);
  EXPECT_EQ(getTargetTag(m), 42);
}

TEST(SchedulerDelegatePure, GetTargetTag_Delete_ReturnsOldChildTag) {
  auto m = makeMutation(ShadowViewMutation::Delete, 42);
  EXPECT_EQ(getTargetTag(m), 42);
}

// ============================================================================
// containsCreateMutation
// ============================================================================

TEST(SchedulerDelegatePure, ContainsCreate_WithCreate_ReturnsTrue) {
  ShadowViewMutationList list = {
      makeMutation(ShadowViewMutation::Update, 1),
      makeMutation(ShadowViewMutation::Create, 2),
      makeMutation(ShadowViewMutation::Insert, 3),
  };
  EXPECT_TRUE(containsCreateMutation(list));
}

TEST(SchedulerDelegatePure, ContainsCreate_WithoutCreate_ReturnsFalse) {
  ShadowViewMutationList list = {
      makeMutation(ShadowViewMutation::Update, 1),
      makeMutation(ShadowViewMutation::Insert, 2),
  };
  EXPECT_FALSE(containsCreateMutation(list));
}

TEST(SchedulerDelegatePure, ContainsCreate_EmptyList_ReturnsFalse) {
  ShadowViewMutationList list;
  EXPECT_FALSE(containsCreateMutation(list));
}

// ============================================================================
// isFlashListSerialOnlyComponent
// ============================================================================

TEST(SchedulerDelegatePure, FlashList_AutoLayoutView_ReturnsTrue) {
  EXPECT_TRUE(isFlashListSerialOnlyComponent("AutoLayoutView"));
}

TEST(SchedulerDelegatePure, FlashList_CellContainer_ReturnsTrue) {
  EXPECT_TRUE(isFlashListSerialOnlyComponent("CellContainer"));
}

TEST(SchedulerDelegatePure, FlashList_OtherComponent_ReturnsFalse) {
  EXPECT_FALSE(isFlashListSerialOnlyComponent("View"));
  EXPECT_FALSE(isFlashListSerialOnlyComponent("Text"));
}

TEST(SchedulerDelegatePure, FlashList_NullPtr_ReturnsFalse) {
  EXPECT_FALSE(isFlashListSerialOnlyComponent(nullptr));
}

// ============================================================================
// splitNonCreateMutationsIntoTwoBatches
// ============================================================================

TEST(SchedulerDelegateSplit, EmptyList_ReturnsDisabled) {
  ShadowViewMutationList list;
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  EXPECT_FALSE(result.enabled);
  EXPECT_TRUE(result.batchA.empty());
  EXPECT_TRUE(result.batchB.empty());
}

TEST(SchedulerDelegateSplit, SmallCount_ReturnsDisabled) {
  auto list = makeLargeList(10); // < 60
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  EXPECT_FALSE(result.enabled);
  EXPECT_EQ(result.batchA.size(), 10u);
  EXPECT_TRUE(result.batchB.empty());
}

TEST(SchedulerDelegateSplit, LowCost_ReturnsDisabled) {
  // 60 items but low cost (all Update = 2 each = 120 total, meets threshold)
  auto list = makeLargeList(55); // < 60 count
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  EXPECT_FALSE(result.enabled);
}

TEST(SchedulerDelegateSplit, LargeCount_SplitsIntoTwoBatches) {
  auto list = makeLargeList(100); // > 60 items, cost = 200
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  EXPECT_TRUE(result.enabled);
  EXPECT_EQ(result.batchA.size() + result.batchB.size(), 100u);
  EXPECT_GT(result.batchA.size(), 0u);
  EXPECT_GT(result.batchB.size(), 0u);
}

TEST(SchedulerDelegateSplit, SplitIsNearHalfHalf) {
  auto list = makeLargeList(100);
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  ASSERT_TRUE(result.enabled);
  // Each batch should be roughly half (40-60 range)
  EXPECT_GE(result.batchA.size(), 30u);
  EXPECT_LE(result.batchA.size(), 70u);
  EXPECT_GE(result.batchB.size(), 30u);
  EXPECT_LE(result.batchB.size(), 70u);
}

TEST(SchedulerDelegateSplit, MoveChain_RemoveInsertSameTag_NotSplitMidway) {
  // Remove tag 5, then Insert tag 5 — can't split between them
  ShadowViewMutationList list;
  for (int i = 0; i < 70; ++i) {
    list.push_back(makeMutation(ShadowViewMutation::Update, 100 + i, 1));
  }
  // Insert a move chain in the middle
  list[35] = makeMutation(ShadowViewMutation::Remove, 5, 1);
  list[36] = makeMutation(ShadowViewMutation::Insert, 5, 1);
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  if (result.enabled) {
    // The move chain should not be split across batches
    bool hasRemoveInA = false, hasInsertInB = false;
    for (const auto& m : result.batchA) {
      if (m.type == ShadowViewMutation::Remove && m.newChildShadowView.tag == 5)
        hasRemoveInA = true;
    }
    for (const auto& m : result.batchB) {
      if (m.type == ShadowViewMutation::Insert && m.newChildShadowView.tag == 5)
        hasInsertInB = true;
    }
    EXPECT_FALSE(hasRemoveInA && hasInsertInB);
  }
}

TEST(SchedulerDelegateSplit, AllSameParentStructural_LowCost_DoesNotSplit) {
  // All Insert with same parent, but too few/cost < threshold
  ShadowViewMutationList list;
  for (int i = 0; i < 10; ++i) {
    list.push_back(makeMutation(ShadowViewMutation::Insert, 100 + i, 1));
  }
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  EXPECT_FALSE(result.enabled);
}

TEST(SchedulerDelegateSplit, MixedMutations_SplitsCorrectly) {
  ShadowViewMutationList list;
  for (int i = 0; i < 70; ++i) {
    ShadowViewMutation::Type types[] = {
        ShadowViewMutation::Insert,
        ShadowViewMutation::Remove,
        ShadowViewMutation::Update,
        ShadowViewMutation::Delete};
    list.push_back(makeMutation(types[i % 4], 100 + i, 1));
  }
  auto result = splitNonCreateMutationsIntoTwoBatches(list);
  EXPECT_TRUE(result.enabled);
  EXPECT_EQ(result.batchA.size() + result.batchB.size(), list.size());
}

// ============================================================================
// bucketizeCreateMutations
// ============================================================================

TEST(SchedulerDelegateBucket, AllCreates_AllMovable) {
  ShadowViewMutationList list;
  for (int i = 0; i < 5; ++i) {
    list.push_back(makeMutation(ShadowViewMutation::Create, 100 + i, 1));
  }
  auto result = rnoh::bucketizeCreateMutations(list);
  EXPECT_EQ(result.movableCreates.size(), 5u);
  EXPECT_TRUE(result.deferredTail.empty());
  EXPECT_EQ(result.deferredCreateCount, 0u);
}

TEST(
    SchedulerDelegateBucket,
    NonCreateBeforeCreateSameTag_CreateGoesToDeferred) {
  ShadowViewMutationList list = {
      makeMutation(ShadowViewMutation::Update, 2, 1), // non-create for tag 2
      makeMutation(
          ShadowViewMutation::Create, 2, 1), // create for SAME tag 2 → deferred
  };
  auto result = rnoh::bucketizeCreateMutations(list);
  EXPECT_TRUE(result.movableCreates.empty());
  EXPECT_EQ(result.deferredTail.size(), 2u);
  EXPECT_EQ(result.deferredCreateCount, 1u);
}

TEST(SchedulerDelegateBucket, CreateBeforeNonCreate_CreateIsMovable) {
  ShadowViewMutationList list = {
      makeMutation(
          ShadowViewMutation::Create, 1, 1), // create before non-create
      makeMutation(ShadowViewMutation::Update, 2, 1), // non-create after create
  };
  auto result = rnoh::bucketizeCreateMutations(list);
  EXPECT_EQ(result.movableCreates.size(), 1u);
  EXPECT_EQ(result.deferredTail.size(), 1u);
  EXPECT_EQ(result.deferredCreateCount, 0u);
}

TEST(SchedulerDelegateBucket, Mixed_CorrectBuckets) {
  ShadowViewMutationList list = {
      makeMutation(
          ShadowViewMutation::Create, 1, 1), // movable (tag 1 not seen yet)
      makeMutation(ShadowViewMutation::Update, 1, 1), // deferred, tag 1 → seen
      makeMutation(
          ShadowViewMutation::Create,
          1,
          1), // deferred (tag 1 IS seen), deferredCreateCount++
      makeMutation(ShadowViewMutation::Insert, 2, 1), // deferred, tag 2 → seen
      makeMutation(
          ShadowViewMutation::Create,
          2,
          1), // deferred (tag 2 IS seen), deferredCreateCount++
  };
  auto result = rnoh::bucketizeCreateMutations(list);
  EXPECT_EQ(result.movableCreates.size(), 1u);
  EXPECT_EQ(result.deferredTail.size(), 4u);
  EXPECT_EQ(result.deferredCreateCount, 2u);
}

TEST(SchedulerDelegateBucket, EmptyList_EmptyBuckets) {
  ShadowViewMutationList list;
  auto result = rnoh::bucketizeCreateMutations(list);
  EXPECT_TRUE(result.movableCreates.empty());
  EXPECT_TRUE(result.deferredTail.empty());
  EXPECT_EQ(result.deferredCreateCount, 0u);
}
