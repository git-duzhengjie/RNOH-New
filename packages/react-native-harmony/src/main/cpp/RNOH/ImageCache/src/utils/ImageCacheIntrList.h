/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IMAGE_CACHE_INTR_LIST_H
#define IMAGE_CACHE_INTR_LIST_H

#include <atomic>
#include <cstddef>
#include "ImageCacheLog.h"
#include "ImageCacheType.h"

namespace ImageCachePro {

struct ImageCacheIntrListNode {
  ImageCacheIntrListNode* prev{nullptr};
  ImageCacheIntrListNode* next{nullptr};
  std::atomic<bool> inList_{false}; // Track if node is currently in a list

  void Detach() {
    if (!IsLinked()) {
      return;
    }
    if (prev) {
      prev->next = next;
    }
    if (next) {
      next->prev = prev;
    }
    prev = next = nullptr;
    inList_.store(false, std::memory_order_release);
  }

  bool IsLinked() const {
    return inList_.load(std::memory_order_acquire);
  }

  void MarkInList() {
    inList_.store(true, std::memory_order_release);
  }
};

template <typename T, ImageCacheIntrListNode T::* NodeMember>
class ImageCacheIntrList {
 public:
  ImageCacheIntrList() {
    head_.prev = &head_;
    head_.next = &head_;
  }

  ~ImageCacheIntrList() {
    while (auto* item = Front()) {
      Remove(item);
    }
  }

  void PushFront(T* item) {
    auto* node = &(item->*NodeMember);
    if (node->IsLinked()) {
      return;
    }
    node->prev = &head_;
    node->next = head_.next;
    head_.next->prev = node;
    head_.next = node;
    node->MarkInList();
    size_++;
  }

  void PushBack(T* item) {
    auto* node = &(item->*NodeMember);
    if (node->IsLinked()) {
      return;
    }
    node->prev = head_.prev;
    node->next = &head_;
    head_.prev->next = node;
    head_.prev = node;
    node->MarkInList();
    size_++;
  }

  void Remove(T* item) {
    auto* node = &(item->*NodeMember);
    if (!node->IsLinked()) {
      return; // Already detached; prevent double-remove and size_ underflow
    }
    node->Detach();
    size_--;
  }

  T* Front() {
    if (head_.next == &head_) {
      return nullptr;
    }
    return ContainerOf(head_.next);
  }

  const T* Front() const {
    if (head_.next == &head_) {
      return nullptr;
    }
    return ContainerOf(head_.next);
  }

  bool Empty() const {
    return head_.next == &head_;
  }

  size_t Size() const {
    return size_;
  }

  void InsertByPriority(T* item, ImageCachePriority priority) {
    auto* node = &(item->*NodeMember);
    if (node->IsLinked()) {
      return;
    }
    ImageCacheIntrListNode* pos = head_.next;
    while (pos != &head_) {
      T* existing = ContainerOf(pos);
      if (existing->GetPriority() < priority) {
        break;
      }
      pos = pos->next;
    }
    node->prev = pos->prev;
    node->next = pos;
    pos->prev->next = node;
    pos->prev = node;
    node->MarkInList();
    size_++;
  }

  // Public method to get next element from a node pointer (for safe traversal)
  static T* GetNextFromNode(ImageCacheIntrListNode* node) {
    if (node == nullptr || !node->IsLinked()) {
      return nullptr;
    }
    return ContainerOf(node);
  }

  static ImageCacheIntrListNode* GetNextNode(ImageCacheIntrListNode* node) {
    if (node == nullptr || !node->IsLinked()) {
      return nullptr;
    }
    return node->next;
  }

 private:
  static constexpr size_t MemberOffset() {
    return reinterpret_cast<size_t>(
               &(reinterpret_cast<T*>(alignof(T))->*NodeMember)) -
        alignof(T);
  }

  static T* ContainerOf(ImageCacheIntrListNode* node) {
    if (node == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<T*>(reinterpret_cast<char*>(node) - MemberOffset());
  }

  static const T* ContainerOf(const ImageCacheIntrListNode* node) {
    if (node == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<const T*>(
        reinterpret_cast<const char*>(node) - MemberOffset());
  }

  ImageCacheIntrListNode head_;
  // NOT thread-safe ? all operations must be protected by an external mutex
  // (e.g., queueMutex_ in ImageCacheDispatcher). The pointer manipulations
  // in PushFront/PushBack/Remove/InsertByPriority are not atomic.
  size_t size_{0};
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_INTR_LIST_H
