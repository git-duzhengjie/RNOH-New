/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/**
 * Used only in C-API based Architecture.
 */
#pragma once
#include <memory>
#include <mutex>

#include <cxxreact/TraceSection.h>
#include <react/renderer/core/ReactPrimitives.h>
#include "RNOH/Assert.h"
#include "RNOH/ComponentInstance.h"

namespace rnoh {
/**
 * @Thread: MAIN
 *
 * ComponentInstanceRegistry stores the ComponentInstance objects and allows
 * retrieving them by tag or id.
 */
class ComponentInstanceRegistry : public ComponentInstance::Registry {
 public:
  using Shared = std::shared_ptr<ComponentInstanceRegistry>;

  ~ComponentInstanceRegistry() {
    DLOG(INFO) << "~ComponentInstanceRegistry";
  }

  ComponentInstance::Shared findByTag(facebook::react::Tag tag) const {
    // A single mutex protects both maps, so the lookup is a consistent
    // snapshot even when other threads mutate the registry concurrently.
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_componentInstanceByTag.find(tag);
    if (it != m_componentInstanceByTag.end()) {
      return it->second;
    }
    return nullptr;
  }

  std::optional<facebook::react::Tag> findTagById(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_tagById.find(id);
    if (it != m_tagById.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  ComponentInstance::Shared findById(const std::string& id) const override {
    // Hold the lock across both lookups so no other thread can erase the tag
    // (and invalidate the mapping) between resolving the id and fetching the
    // instance — the TOCTOU window that existed with two separate mutexes.
    std::lock_guard<std::mutex> lock(m_mtx);
    auto tagIt = m_tagById.find(id);
    if (tagIt == m_tagById.end()) {
      return nullptr;
    }
    auto instIt = m_componentInstanceByTag.find(tagIt->second);
    if (instIt == m_componentInstanceByTag.end()) {
      return nullptr;
    }
    return instIt->second;
  }

  void insert(ComponentInstance::Shared componentInstance) {
    auto tag = componentInstance->getTag();
    std::lock_guard<std::mutex> lock(m_mtx);
    m_componentInstanceByTag.emplace(tag, std::move(componentInstance));
  }

  void updateTagById(
      facebook::react::Tag tag,
      const std::string& id,
      const std::string prevId) {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (!prevId.empty() && m_tagById.find(prevId) != m_tagById.end()) {
      m_tagById.erase(prevId);
    }
    if (!id.empty()) {
      m_tagById.emplace(id, tag);
    }
  }

  void deleteByTag(facebook::react::Tag tag) {
    // The whole operation runs under one lock: previously the iterator was
    // read under one mutex and then dereferenced after releasing it, which
    // could dereference an iterator invalidated by a concurrent insert/erase.
    std::optional<std::string> componentInstanceId;
    {
      std::lock_guard<std::mutex> lock(m_mtx);
      auto it = m_componentInstanceByTag.find(tag);
      if (it == m_componentInstanceByTag.end()) {
        return;
      }
      // Copy the id while still holding the lock, then drop the entry. We must
      // not hold an iterator into the map after the map is mutated.
      auto componentInstance = it->second;
      auto idStr = componentInstance->getId();
      if (!idStr.empty()) {
        componentInstanceId = std::move(idStr);
      }
      m_componentInstanceByTag.erase(it);
      if (componentInstanceId.has_value()) {
        m_tagById.erase(*componentInstanceId);
      }
    }
  }

  size_t getComponentsCount() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_componentInstanceByTag.size();
  }

 private:
  std::unordered_map<facebook::react::Tag, ComponentInstance::Shared>
      m_componentInstanceByTag = {};
  std::unordered_map<std::string, facebook::react::Tag> m_tagById = {};
  // Single mutex guards both maps so compound operations (findById,
  // deleteByTag) are atomic and no stale iterator/map state can be observed
  // between two separately-locked critical sections.
  mutable std::mutex m_mtx;
};
} // namespace rnoh
