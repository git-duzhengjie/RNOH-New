/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ImageCacheIntrList.h"
#include <gtest/gtest.h>

using namespace ImageCachePro;

// Test item with intrusive list node member and priority.
// Uses default member initializers + designated initializers (C++20)
// because std::atomic in ImageCacheIntrListNode makes the type non-copyable.
struct TestItem {
  ImageCacheIntrListNode node;
  int value = 0;
  ImageCachePriority priority = ImageCachePriority::MEDIUM;

  ImageCachePriority GetPriority() const {
    return priority;
  }
};

using TestList = ImageCacheIntrList<TestItem, &TestItem::node>;

// Helper: create a TestItem with given value and priority.
#define MAKE_ITEM(name, val, pri) \
  TestItem name;                  \
  name.value = val;               \
  name.priority = pri

// Helper: create a TestItem with default (MEDIUM) priority.
#define MAKE_ITEM_MED(name, val) \
  TestItem name;                 \
  name.value = val

// ===========================================================================
// Tier 2 — glog stub + ohresmgr stub (host-compilable, no device)
// Module: RNOH/ImageCache/src/utils/ImageCacheIntrList
//
// Tested class: ImageCacheIntrList<T, NodeMember> (intrusive doubly-linked
// list with priority-ordered insertion)
// ===========================================================================

// ---------------------------------------------------------------------------
// Constructor / Empty / Size
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, Constructor_EmptyList_IsEmptyAndZeroSize) {
  TestList list;
  EXPECT_TRUE(list.Empty());
  EXPECT_EQ(list.Size(), 0u);
  EXPECT_EQ(list.Front(), nullptr);
}

TEST(ImageCacheIntrList, Empty_WhenNonEmpty_ReturnsFalse) {
  TestList list;
  MAKE_ITEM_MED(item, 42);
  list.PushBack(&item);
  EXPECT_FALSE(list.Empty());
}

// ---------------------------------------------------------------------------
// PushFront / PushBack / Front
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, PushFront_SingleItem_SizeIsOne) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushFront(&item);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &item);
}

TEST(ImageCacheIntrList, PushBack_SingleItem_SizeIsOne_FrontAndBack) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushBack(&item);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &item);
  EXPECT_TRUE(item.node.IsLinked());
}

TEST(ImageCacheIntrList, PushFront_MultipleItems_FrontReturnsLastPushed) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  MAKE_ITEM_MED(c, 3);
  list.PushFront(&a);
  list.PushFront(&b);
  list.PushFront(&c);
  EXPECT_EQ(list.Size(), 3u);
  EXPECT_EQ(list.Front()->value, 3);
}

TEST(ImageCacheIntrList, PushBack_MultipleItems_FrontReturnsFirstPushed) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  MAKE_ITEM_MED(c, 3);
  list.PushBack(&a);
  list.PushBack(&b);
  list.PushBack(&c);
  EXPECT_EQ(list.Size(), 3u);
  EXPECT_EQ(list.Front()->value, 1);
}

// ---------------------------------------------------------------------------
// Duplicate insertion guard
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, PushFront_DuplicateItem_NotDoubleInserted) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushFront(&item);
  list.PushFront(&item);
  EXPECT_EQ(list.Size(), 1u);
}

TEST(ImageCacheIntrList, PushBack_DuplicateItem_NotDoubleInserted) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushBack(&item);
  list.PushBack(&item);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &item);
}

TEST(ImageCacheIntrList, PushFront_AfterPushBack_NotDoubleInserted) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushBack(&item);
  list.PushFront(&item);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_TRUE(item.node.IsLinked());
}

// ---------------------------------------------------------------------------
// Remove
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, Remove_ExistingItem_SizeDecreases) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  list.PushBack(&a);
  list.PushBack(&b);
  list.Remove(&a);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &b);
}

TEST(ImageCacheIntrList, Remove_NotInList_NoChange) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  list.PushBack(&a);
  list.Remove(&b);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &a);
}

TEST(ImageCacheIntrList, Remove_AfterDetach_NoUnderflow) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  list.PushBack(&a);
  a.node.Detach();
  list.Remove(&a);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_FALSE(a.node.IsLinked());
}

TEST(ImageCacheIntrList, Remove_DoubleRemove_NoUnderflow) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  list.PushBack(&a);
  list.Remove(&a);
  list.Remove(&a);
  EXPECT_EQ(list.Size(), 0u);
}

// ---------------------------------------------------------------------------
// InsertByPriority
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, InsertByPriority_SingleItem_SizeIsOne_FrontIsItem) {
  TestList list;
  MAKE_ITEM(item, 1, ImageCachePriority::HIGH);
  list.InsertByPriority(&item, ImageCachePriority::HIGH);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &item);
  EXPECT_TRUE(item.node.IsLinked());
}

TEST(ImageCacheIntrList, InsertByPriority_MixedPriorities_SortedDescending) {
  TestList list;
  MAKE_ITEM(low, 1, ImageCachePriority::LOW);
  MAKE_ITEM(high, 2, ImageCachePriority::HIGH);
  MAKE_ITEM(medium, 3, ImageCachePriority::MEDIUM);
  list.InsertByPriority(&low, ImageCachePriority::LOW);
  list.InsertByPriority(&high, ImageCachePriority::HIGH);
  list.InsertByPriority(&medium, ImageCachePriority::MEDIUM);
  // InsertByPriority inserts before first existing item whose priority
  // value < new item's priority value. So larger values sort to front:
  // order is LOW(2) > MEDIUM(1) > HIGH(0) → Front is LOW
  EXPECT_EQ(list.Front()->value, 1); // LOW
}

TEST(ImageCacheIntrList, InsertByPriority_EqualPriorities_AppendAfterExisting) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  list.InsertByPriority(&a, ImageCachePriority::MEDIUM);
  list.InsertByPriority(&b, ImageCachePriority::MEDIUM);
  EXPECT_EQ(list.Size(), 2u);
  EXPECT_EQ(list.Front()->value, 1);
}

TEST(ImageCacheIntrList, InsertByPriority_DuplicateItem_NotDoubleInserted) {
  TestList list;
  MAKE_ITEM(item, 1, ImageCachePriority::HIGH);
  list.InsertByPriority(&item, ImageCachePriority::HIGH);
  list.InsertByPriority(&item, ImageCachePriority::HIGH);
  EXPECT_EQ(list.Size(), 1u);
  EXPECT_EQ(list.Front(), &item);
}

// ---------------------------------------------------------------------------
// IsLinked / Detach / MarkInList (node-level)
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, IsLinked_WhenNotInList_ReturnsFalse) {
  MAKE_ITEM_MED(item, 1);
  EXPECT_FALSE(item.node.IsLinked());
}

TEST(ImageCacheIntrList, IsLinked_WhenInList_ReturnsTrue) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushBack(&item);
  EXPECT_TRUE(item.node.IsLinked());
}

TEST(ImageCacheIntrList, Detach_AfterInsert_MarksNotLinked) {
  TestList list;
  MAKE_ITEM_MED(item, 1);
  list.PushBack(&item);
  item.node.Detach();
  EXPECT_FALSE(item.node.IsLinked());
  EXPECT_EQ(list.Size(), 1u);
}

// ---------------------------------------------------------------------------
// GetNextNode / GetNextFromNode (static traversal helpers)
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, GetNextNode_NullPtr_ReturnsNullptr) {
  EXPECT_EQ(TestList::GetNextNode(nullptr), nullptr);
}

TEST(ImageCacheIntrList, GetNextNode_NotLinkedNode_ReturnsNullptr) {
  MAKE_ITEM_MED(item, 1);
  EXPECT_EQ(TestList::GetNextNode(&item.node), nullptr);
}

TEST(ImageCacheIntrList, GetNextNode_LinkedNode_ReturnsNextNode) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  list.PushBack(&a);
  list.PushBack(&b);
  EXPECT_EQ(TestList::GetNextNode(&a.node), &b.node);
}

TEST(ImageCacheIntrList, GetNextFromNode_NullPtr_ReturnsNullptr) {
  EXPECT_EQ(TestList::GetNextFromNode(nullptr), nullptr);
}

TEST(ImageCacheIntrList, GetNextFromNode_LinkedNode_ReturnsContainer) {
  TestList list;
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  list.PushBack(&a);
  list.PushBack(&b);
  // GetNextFromNode returns ContainerOf(node) — i.e., the T that owns the
  // given node — NOT the next element. Passing &a.node returns &a.
  TestItem* owner = TestList::GetNextFromNode(&a.node);
  EXPECT_NE(owner, nullptr);
  EXPECT_EQ(owner, &a);
  EXPECT_EQ(owner->value, 1);
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

TEST(ImageCacheIntrList, Destructor_ClearsAllItems) {
  MAKE_ITEM_MED(a, 1);
  MAKE_ITEM_MED(b, 2);
  MAKE_ITEM_MED(c, 3);
  {
    TestList list;
    list.PushBack(&a);
    list.PushBack(&b);
    list.PushBack(&c);
    EXPECT_EQ(list.Size(), 3u);
  }
  EXPECT_FALSE(a.node.IsLinked());
  EXPECT_FALSE(b.node.IsLinked());
  EXPECT_FALSE(c.node.IsLinked());
}
