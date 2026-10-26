/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "SchedulerDelegate.h"
#include <cxxreact/TraceSection.h>
#include "MountingManager.h"
#include "ParallelCheck.h"
#include "RNOH/FFRTConfig.h"
#include "RNOH/ParallelCheck.h"
#include "RNOH/ParallelComponent.h"
#include "RNOH/Performance/RNOHMarker.h"
#include "ffrt/cpp/pattern/job_partner.h"
#include "ffrt/cpp/pattern/job_ring.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <glog/logging.h>
#include <native_vsync/native_vsync.h>
#include <sstream>
#include <string>
#include <string_view>

namespace {

class NextFrameDispatcher {
 public:
  static NextFrameDispatcher& Get() {
    static NextFrameDispatcher instance;
    return instance;
  }

  void post(std::function<void()> task) {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_pending.emplace_back(std::move(task));
    }
    requestIfNeeded();
  }

 private:
  NextFrameDispatcher() {
    const char* name = "RNOH::SchedulerDelegate";
    m_vsync = OH_NativeVSync_Create(
        name, static_cast<unsigned int>(std::strlen(name)));
  }

  ~NextFrameDispatcher() {
    // Mark shutdown before tearing down so a VSync callback that is already in
    // flight (or races with destruction) bails out instead of touching freed
    // state. The VSync system holds a raw C pointer to us and is not aware of
    // this object's lifetime.
    m_shuttingDown.store(true, std::memory_order_release);
    if (m_vsync != nullptr) {
      OH_NativeVSync_Destroy(m_vsync);
      m_vsync = nullptr;
    }
  }

  static void onVsync(long long /*timestamp*/, void* data) {
    auto* self = static_cast<NextFrameDispatcher*>(data);
    if (self == nullptr || self->m_shuttingDown.load(std::memory_order_acquire)) {
      return;
    }
    self->m_requestInFlight.store(false, std::memory_order_release);
    self->flush();
  }

  void requestIfNeeded() {
    bool expected = false;
    if (!m_requestInFlight.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel,
            std::memory_order_relaxed)) {
      return;
    }

    if (m_vsync != nullptr) {
      (void)OH_NativeVSync_RequestFrame(
          m_vsync, &NextFrameDispatcher::onVsync, this);
      return;
    }

    m_requestInFlight.store(false, std::memory_order_release);
    flush();
  }

  void flush() {
    std::vector<std::function<void()>> tasks;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      tasks.swap(m_pending);
    }
    for (auto& t : tasks) {
      t();
    }
    bool hasMore = false;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      hasMore = !m_pending.empty();
    }
    if (hasMore) {
      requestIfNeeded();
    }
  }

 private:
  std::mutex m_mutex;
  std::vector<std::function<void()>> m_pending;
  std::atomic<bool> m_requestInFlight{false};
  std::atomic<bool> m_shuttingDown{false};
  OH_NativeVSync* m_vsync{nullptr};
};

struct TwoBatchSplit {
  bool enabled{false};
  facebook::react::ShadowViewMutationList batchA;
  facebook::react::ShadowViewMutationList batchB;
};

using SteadyClock = std::chrono::steady_clock;

// ---- SchedulerDelegate tuning constants (avoid magic numbers) ----
constexpr size_t SURFACE_LOAD_MAX_ENTRIES = 128;
constexpr int SURFACE_LOAD_TTL_MINUTES = 2;

constexpr size_t NONCREATE_SPLIT_MIN_COUNT = 60;
constexpr int NONCREATE_SPLIT_MIN_COST = 120;

constexpr size_t GROUPS_RESERVE_HINT = 16;
constexpr size_t FENCE_RESERVE_HINT = 64;

constexpr size_t TWO_BATCHES = 2;

// Non-create mutation cost weights (rough heuristic).
constexpr int COST_DEFAULT = 2; // Update / miscellaneous
constexpr int COST_INSERT = 3;
constexpr int COST_REMOVE = 3;
constexpr int COST_DELETE = 4;

constexpr int64_t CONFIG_CHANGE_SKIP_WINDOW_MS = 3000; // 3s

constexpr size_t SURFACE_SERIAL_FORCE_MAX_ENTRIES = 128;
constexpr int SURFACE_SERIAL_FORCE_TTL_MINUTES = 10;
// ---------------------------------------------------------------

struct SurfaceLoadState {
  SteadyClock::time_point firstSeen{};
  SteadyClock::time_point lastSeen{};
  uint32_t txCount{0};
};

static std::mutex g_surfaceLoadMutex;
static std::unordered_map<facebook::react::SurfaceId, SurfaceLoadState>
    g_surfaceLoad;

static std::atomic<int64_t> g_lastConfigChangeMs{0};
static std::atomic<int32_t> g_activeModalCount{0};

static std::mutex g_surfaceSerialForceMutex;
static std::unordered_map<facebook::react::SurfaceId, SteadyClock::time_point>
    g_surfaceSerialForce;

static inline int64_t steadyClockMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             SteadyClock::now().time_since_epoch())
      .count();
}

static inline bool isInConfigChangeWindow() {
  const int64_t last = g_lastConfigChangeMs.load(std::memory_order_acquire);
  if (last == 0) {
    return false;
  }
  return (steadyClockMs() - last) <= CONFIG_CHANGE_SKIP_WINDOW_MS;
}

static inline bool shouldDisableSchedulerParallelForModalFold() {
  return g_activeModalCount.load(std::memory_order_acquire) > 0;
}

static void doPruneSurfaceLoad(
    SteadyClock::time_point now,
    std::chrono::minutes ttl) {
  for (auto it = g_surfaceLoad.begin(); it != g_surfaceLoad.end();) {
    if (now - it->second.lastSeen > ttl) {
      it = g_surfaceLoad.erase(it);
    } else {
      ++it;
    }
  }
}

static inline void pruneSurfaceLoadLocked(SteadyClock::time_point now) {
  if (g_surfaceLoad.size() <= SURFACE_LOAD_MAX_ENTRIES) {
    return;
  }

  const auto ttl = std::chrono::minutes(SURFACE_LOAD_TTL_MINUTES);
  doPruneSurfaceLoad(now, ttl);
}

static bool isInInitialLoadWindowImpl(facebook::react::SurfaceId surfaceId) {
  if (surfaceId == 0) {
    return false;
  }

  constexpr int64_t LOAD_WINDOW_MS = 3000; // 3s
  constexpr uint32_t LOAD_WINDOW_MAX_TX = 3; // first 3 transactions

  const auto now = SteadyClock::now();
  std::lock_guard<std::mutex> lock(g_surfaceLoadMutex);
  pruneSurfaceLoadLocked(now);

  auto& st = g_surfaceLoad[surfaceId];
  if (st.firstSeen == SteadyClock::time_point{}) {
    st.firstSeen = now;
    st.txCount = 0;
  }
  st.lastSeen = now;
  ++st.txCount;

  const auto elapsedMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - st.firstSeen)
          .count();
  return elapsedMs < LOAD_WINDOW_MS || st.txCount <= LOAD_WINDOW_MAX_TX;
}

static inline bool isInInitialLoadWindow(facebook::react::SurfaceId surfaceId) {
  return isInInitialLoadWindowImpl(surfaceId);
}

static inline bool isFlashListSerialOnlyComponent(const char* componentName) {
  if (componentName == nullptr) {
    return false;
  }
  const std::string_view name{componentName};
  return name == "AutoLayoutView" || name == "CellContainer";
}

static inline void pruneSurfaceSerialForceLocked(SteadyClock::time_point now) {
  if (g_surfaceSerialForce.size() <= SURFACE_SERIAL_FORCE_MAX_ENTRIES) {
    return;
  }
  const auto ttl = std::chrono::minutes(SURFACE_SERIAL_FORCE_TTL_MINUTES);
  for (auto it = g_surfaceSerialForce.begin();
       it != g_surfaceSerialForce.end();) {
    if (now - it->second > ttl) {
      it = g_surfaceSerialForce.erase(it);
    } else {
      ++it;
    }
  }
}

static int estimateNonCreateCostImpl(
    const facebook::react::ShadowViewMutation& m) {
  using M = facebook::react::ShadowViewMutation;
  switch (m.type) {
    case M::Create:
      return 0;
    case M::Insert:
      return COST_INSERT;
    case M::Remove:
      return COST_REMOVE;
    case M::Delete:
      return COST_DELETE;
    case M::Update:
    default:
      return COST_DEFAULT;
  }
}

static inline int estimateNonCreateCost(
    const facebook::react::ShadowViewMutation& m) {
  return estimateNonCreateCostImpl(m);
}

static inline bool isStructuralWithParent(
    const facebook::react::ShadowViewMutation& m) {
  using M = facebook::react::ShadowViewMutation;
  return m.type == M::Insert || m.type == M::Remove;
}

static const facebook::react::ShadowView& getMutationTargetShadowView(
    const facebook::react::ShadowViewMutation& mutation) {
  using M = facebook::react::ShadowViewMutation;
  switch (mutation.type) {
    case M::Remove:
    case M::Delete:
      return mutation.oldChildShadowView;
    case M::Create:
    case M::Insert:
    case M::Update:
    default:
      return mutation.newChildShadowView;
  }
}

static bool shouldForceSerialForSurface(
    facebook::react::SurfaceId surfaceId,
    const facebook::react::ShadowViewMutationList& mutations) {
  if (surfaceId == 0) {
    return false;
  }

  bool containsFlashListComponent = false;
  for (const auto& mutation : mutations) {
    const auto& targetShadowView = getMutationTargetShadowView(mutation);
    if (isFlashListSerialOnlyComponent(targetShadowView.componentName)) {
      containsFlashListComponent = true;
      break;
    }
  }

  const auto now = SteadyClock::now();
  std::lock_guard<std::mutex> lock(g_surfaceSerialForceMutex);
  pruneSurfaceSerialForceLocked(now);
  if (containsFlashListComponent) {
    g_surfaceSerialForce[surfaceId] = now;
    return true;
  }
  return g_surfaceSerialForce.find(surfaceId) != g_surfaceSerialForce.end();
}

static facebook::react::Tag getTargetTagImpl(
    const facebook::react::ShadowViewMutation& m) {
  using M = facebook::react::ShadowViewMutation;
  switch (m.type) {
    case M::Create:
    case M::Insert:
    case M::Update:
      return m.newChildShadowView.tag;
    case M::Remove:
    case M::Delete:
      return m.oldChildShadowView.tag;
    default:
      return (m.oldChildShadowView.tag != 0) ? m.oldChildShadowView.tag
                                             : m.newChildShadowView.tag;
  }
}

static inline facebook::react::Tag getTargetTag(
    const facebook::react::ShadowViewMutation& m) {
  return getTargetTagImpl(m);
}

static bool containsCreateMutation(
    const facebook::react::ShadowViewMutationList& mutations) {
  using M = facebook::react::ShadowViewMutation;
  for (const auto& m : mutations) {
    if (m.type == M::Create) {
      return true;
    }
  }
  return false;
}

static bool isTextHeavyInsertTail(
    const facebook::react::ShadowViewMutationList& mutations) {
  using M = facebook::react::ShadowViewMutation;
  size_t paragraphInserts = 0;
  for (const auto& m : mutations) {
    if (m.type != M::Insert) {
      continue;
    }
    const char* name = m.newChildShadowView.componentName;
    if (name != nullptr && std::string_view(name) == "Paragraph") {
      ++paragraphInserts;
      if (paragraphInserts >= 200) {
        return true;
      }
    }
  }
  return false;
}

static TwoBatchSplit splitNonCreateMutationsIntoTwoBatches(
    const facebook::react::ShadowViewMutationList& nonCreate) {
  facebook::react::TraceSection s(
      "#RNOH::SchedulerDelegate::splitNonCreateMutationsIntoTwoBatches");

  TwoBatchSplit out;
  const size_t n = nonCreate.size();

  // Pre-scan: used to (1) distinguish real "move" (Remove+Insert) from "delete"
  // (Remove+Delete), and (2) protect Insert+Update chains for the same tag from
  // being split across frames.
  std::unordered_set<facebook::react::Tag> willInsertTags;
  willInsertTags.reserve(n);
  std::unordered_map<facebook::react::Tag, size_t> lastUpdateIndex;
  lastUpdateIndex.reserve(n);

  int quickTotalCost = 0;
  for (size_t i = 0; i < n; ++i) {
    const auto& m = nonCreate[i];
    quickTotalCost += estimateNonCreateCost(m);

    using M = facebook::react::ShadowViewMutation;
    const auto tag = getTargetTag(m);
    if (m.type == M::Insert) {
      willInsertTags.insert(tag);
    } else if (m.type == M::Update) {
      lastUpdateIndex[tag] = i; // keep last
    }
  }

  // Small & cheap batches don't benefit from extra frame; keep single batch.
  if (n < NONCREATE_SPLIT_MIN_COUNT &&
      quickTotalCost < NONCREATE_SPLIT_MIN_COST) {
    out.enabled = false;
    out.batchA = nonCreate;
    return out;
  }

  struct Group {
    size_t start{0};
    size_t end{0};
    int cost{0};
  };

  std::vector<Group> groups;
  groups.reserve(GROUPS_RESERVE_HINT);

  size_t groupStart = 0;
  int groupCost = 0;

  // openMovedTags: tags that have been Removed and are expected to be Inserted
  // later (i.e. Move).
  std::unordered_set<facebook::react::Tag> openMovedTags;
  openMovedTags.reserve(FENCE_RESERVE_HINT);

  // pendingInitTags: tags that have been Inserted but still have Updates later
  // in this same transaction. We store their last update index and prevent
  // boundary split until those updates are consumed.
  std::unordered_map<facebook::react::Tag, size_t> pendingInitTags;
  pendingInitTags.reserve(FENCE_RESERVE_HINT);

  constexpr int SOFT_SAMEPARENT_BREAK_COST =
      90; // allow breaking long Insert/Remove runs under same parent

  auto pushGroup = [&groups, &groupStart, &groupCost](size_t endExclusive) {
    if (endExclusive <= groupStart) {
      return;
    }
    groups.push_back(Group{groupStart, endExclusive, groupCost});
    groupStart = endExclusive;
    groupCost = 0;
  };

  for (size_t i = 0; i < n; ++i) {
    const auto& m = nonCreate[i];
    groupCost += estimateNonCreateCost(m);

    {
      using M = facebook::react::ShadowViewMutation;
      const auto tag = getTargetTag(m);

      // Distinguish "move" vs "delete":
      // - Only treat Remove as "move-open" if we know there will be a matching
      // Insert later.
      // - If it's a delete chain (Remove+Delete), do not block all boundaries.
      if (m.type == M::Remove) {
        if (willInsertTags.find(tag) != willInsertTags.end()) {
          openMovedTags.insert(tag);
        }
        // For safety: if an inserted tag gets removed, it no longer needs init
        // protection.
        pendingInitTags.erase(tag);
      } else if (m.type == M::Insert) {
        // Close move-open when we see the matching Insert.
        auto it = openMovedTags.find(tag);
        if (it != openMovedTags.end())
          openMovedTags.erase(it);

        // Protect Insert+Update chain: if this tag has Updates later, keep it
        // within the same group until last Update.
        auto uIt = lastUpdateIndex.find(tag);
        if (uIt != lastUpdateIndex.end() && uIt->second > i) {
          pendingInitTags[tag] = uIt->second;
        }
      } else if (m.type == M::Update) {
        // If this Update is the last Update for a tag we inserted, the init
        // fence is closed here.
        auto pIt = pendingInitTags.find(tag);
        if (pIt != pendingInitTags.end() && pIt->second == i) {
          pendingInitTags.erase(pIt);
        }
      } else if (m.type == M::Delete) {
        // Remove+Delete is NOT a move; make sure it doesn't keep openMovedTags
        // stuck.
        openMovedTags.erase(tag);
        pendingInitTags.erase(tag);
      }
    }

    if (i + 1 < n) {
      const auto& next = nonCreate[i + 1];

      bool sameParentStructuralBlock = false;
      if (isStructuralWithParent(m) && isStructuralWithParent(next)) {
        if (m.parentTag == next.parentTag) {
          sameParentStructuralBlock = true;
        }
      }

      // boundaryAllowed must satisfy:
      // 1) No open move chain (Remove ... Insert of same tag)
      // 2) No pending Insert+Update init chains
      // 3) Avoid cutting tiny same-parent structural runs, but allow a soft
      // break once current group is big enough
      const bool boundaryAllowed = openMovedTags.empty() &&
          pendingInitTags.empty() &&
          (!sameParentStructuralBlock ||
           groupCost >= SOFT_SAMEPARENT_BREAK_COST);
      if (boundaryAllowed) {
        pushGroup(i + 1);
      }
    }
  }
  pushGroup(n);

  if (groups.size() <= 1) {
    out.enabled = false;
    out.batchA = nonCreate;
    return out;
  }

  int totalCost = 0;
  for (const auto& g : groups)
    totalCost += g.cost;

  if (totalCost < NONCREATE_SPLIT_MIN_COST) {
    out.enabled = false;
    out.batchA = nonCreate;
    return out;
  }

  const int target = totalCost / static_cast<int>(TWO_BATCHES);

  size_t bestGroupBoundaryIndex = 0;
  int bestDiff = std::numeric_limits<int>::max();

  int prefixCost = 0;
  for (size_t k = 1; k < groups.size(); ++k) {
    prefixCost += groups[k - 1].cost;
    const int diff = std::abs(prefixCost - target);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestGroupBoundaryIndex = k;
    }
  }

  size_t splitIndex = groups[bestGroupBoundaryIndex - 1].end;
  if (splitIndex == 0 || splitIndex >= n) {
    if (n >= TWO_BATCHES) {
      splitIndex = n / TWO_BATCHES;
      if (splitIndex == 0) {
        splitIndex = 1;
      }
      if (splitIndex >= n) {
        splitIndex = n - 1;
      }
    } else {
      out.enabled = false;
      out.batchA = nonCreate;
      return out;
    }
  }

  out.enabled = true;
  out.batchA = facebook::react::ShadowViewMutationList(
      nonCreate.begin(), nonCreate.begin() + splitIndex);
  out.batchB = facebook::react::ShadowViewMutationList(
      nonCreate.begin() + splitIndex, nonCreate.end());
  return out;
}

} // namespace

namespace rnoh {

struct CreateBuckets {
  facebook::react::ShadowViewMutationList movableCreates;
  facebook::react::ShadowViewMutationList deferredTail;
  size_t deferredCreateCount{0};
};

static CreateBuckets bucketizeCreateMutations(
    const facebook::react::ShadowViewMutationList& all) {
  using M = facebook::react::ShadowViewMutation;

  CreateBuckets out;
  out.movableCreates.reserve(all.size());
  out.deferredTail.reserve(all.size());

  // If a non-create mutation for a tag appears before its Create in the same
  // transaction, that Create must not be moved ahead.
  std::unordered_set<facebook::react::Tag> seenNonCreateTags;
  seenNonCreateTags.reserve(all.size());

  for (const auto& m : all) {
    const auto tag = getTargetTag(m);
    if (m.type == M::Create) {
      if (seenNonCreateTags.find(tag) != seenNonCreateTags.end()) {
        out.deferredTail.push_back(m);
        ++out.deferredCreateCount;
      } else {
        out.movableCreates.push_back(m);
      }
      continue;
    }

    out.deferredTail.push_back(m);
    seenNonCreateTags.insert(tag);
  }

  return out;
}

SchedulerDelegate::~SchedulerDelegate() {
  DLOG(INFO) << "~SchedulerDelegate";
}

void SchedulerDelegate::markConfigurationChange() {
  g_lastConfigChangeMs.store(steadyClockMs(), std::memory_order_release);
}

void SchedulerDelegate::notifyModalVisibilityChanged(bool visible) {
  if (visible) {
    auto current =
        g_activeModalCount.fetch_add(1, std::memory_order_acq_rel) + 1;
    return;
  }

  int32_t current = g_activeModalCount.load(std::memory_order_acquire);
  while (current > 0 &&
         !g_activeModalCount.compare_exchange_weak(
             current,
             current - 1,
             std::memory_order_acq_rel,
             std::memory_order_acquire)) {
    current = g_activeModalCount.load(std::memory_order_acquire);
  }
  if (current <= 0) {
    return;
  }
}

void SchedulerDelegate::schedulerDidFinishTransaction(
    const std::shared_ptr<const MountingCoordinator>& /*mountingCoordinator*/) {
  // no-op, we will flush the transaction from schedulerShouldRenderTransactions
}

void SchedulerDelegate::schedulerDidUpdateShadowTree(
    const std::unordered_map<facebook::react::Tag, folly::dynamic>&
        tagToProps) {
  // no-op
  // We will investigate if this method should be implemented
}

void logTransactionTelemetryMarkers(MountingTransaction const& transaction) {
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_FINISH_TRANSACTION_END);
  const auto& telemetry = transaction.getTelemetry();
  auto commitStartTime =
      telemetryTimePointToMilliseconds(telemetry.getCommitStartTime());
  auto commitEndTime =
      telemetryTimePointToMilliseconds(telemetry.getCommitEndTime());
  auto diffStartTime =
      telemetryTimePointToMilliseconds(telemetry.getDiffStartTime());
  auto diffEndTime =
      telemetryTimePointToMilliseconds(telemetry.getDiffEndTime());
  auto layoutStartTime =
      telemetryTimePointToMilliseconds(telemetry.getLayoutStartTime());
  auto layoutEndTime =
      telemetryTimePointToMilliseconds(telemetry.getLayoutEndTime());

  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_COMMIT_START, "", commitStartTime);
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_COMMIT_END, "", commitEndTime);
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_DIFF_START, "", diffStartTime);
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_DIFF_END, "", diffEndTime);
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_LAYOUT_START, "", layoutStartTime);
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_LAYOUT_END, "", layoutEndTime);
}

void SchedulerDelegate::reportMount(
    const facebook::react::ShadowViewMutationList& mutations) {
  m_taskExecutor->runTask(
      TaskThread::MAIN, [scheduler = m_scheduler, mutations] {
        const auto& schedulerShared = scheduler.lock();
        if (!schedulerShared) {
          return;
        }
        std::unordered_set<SurfaceId> mountedSurfaceIds;
        for (const auto& mutation : mutations) {
          mountedSurfaceIds.insert(
              getMutationTargetShadowView(mutation).surfaceId);
        }
        if (!mountedSurfaceIds.empty()) {
          for (SurfaceId surfaceId : mountedSurfaceIds) {
            schedulerShared->reportMount(surfaceId);
          }
        }
      });
}

void SchedulerDelegate::schedulerDidRequestPreliminaryViewAllocation(
    const ShadowNode& shadowNode) {
  facebook::react::TraceSection s(
      "#RNOH::SchedulerDelegate::schedulerDidRequestPreliminaryViewAllocation");
  auto preallocationRequestQueue = m_weakPreallocationRequestQueue.lock();
  if (preallocationRequestQueue == nullptr) {
    return;
  }
  preallocationRequestQueue->push({
      shadowNode.getTag(),
      shadowNode.getComponentHandle(),
      shadowNode.getComponentName(),
  });
}

void SchedulerDelegate::schedulerDidDispatchCommand(
    const ShadowView& shadowView,
    std::string const& commandName,
    folly::dynamic const& args) {
  facebook::react::TraceSection s(
      "#RNOH::SchedulerDelegate::schedulerDidDispatchCommand");
  performOnMainThread([shadowView, commandName, args](
                          MountingManager::Shared const& mountingManager) {
    mountingManager->dispatchCommand(shadowView, commandName, args);
  });
}

void SchedulerDelegate::schedulerDidSendAccessibilityEvent(
    const ShadowView& shadowView,
    std::string const& eventType) {
  performOnMainThread([shadowView, eventType](
                          MountingManager::Shared const& mountingManager) {
    mountingManager->schedulerDidSendAccessibilityEvent(shadowView, eventType);
  });
}

void SchedulerDelegate::schedulerDidSetIsJSResponder(
    ShadowView const& shadowView,
    bool isJSResponder,
    bool blockNativeResponder) {
  facebook::react::TraceSection s(
      "#RNOH::SchedulerDelegate::schedulerDidSetIsJSResponder");
  performOnMainThread([shadowView, isJSResponder, blockNativeResponder](
                          MountingManager::Shared const& mountingManager) {
    mountingManager->setIsJsResponder(
        shadowView, isJSResponder, blockNativeResponder);
  });
}

void SchedulerDelegate::performTransaction(
    const std::shared_ptr<const MountingCoordinator>& mountingCoordinator) {
  facebook::react::TraceSection s(
      "#RNOH::SchedulerDelegate::performTransaction");
  RNOHMarker::logMarker(
      RNOHMarker::RNOHMarkerId::FABRIC_FINISH_TRANSACTION_START);
  mountingCoordinator->getTelemetryController().pullTransaction(
      [](auto transaction, auto const& /*surfaceTelemetry*/) {},
      [this](auto transaction, auto const& surfaceTelemetry) {
        performOnMainThread(
            [transaction](MountingManager::Shared const& mountingManager) {
              mountingManager->doMount(transaction->getMutations());
            });
      },
      [this](auto transaction, auto const& surfaceTelemetry) {
        int taskId = random();
        std::string taskTrace =
            "#RNOH::TaskExecutor::runningTask t" + std::to_string(taskId);
        const auto txId = transaction->getNumber();
        const auto surfaceId = transaction->getSurfaceId();
        auto performOnMainThreadNextFrame =
            [this](std::function<void(MountingManager::Shared const&)> task) {
              NextFrameDispatcher::Get().post(
                  [this, t = std::move(task)]() mutable {
                    facebook::react::TraceSection s(
                        "#RNOH::SchedulerDelegate::onNextFrame");
                    this->performOnMainThread(std::move(t));
                  });
            };

        const bool forceSerialForSurface =
            shouldForceSerialForSurface(surfaceId, transaction->getMutations());
        if (IsParallelizationEnabled() && !forceSerialForSurface) {
          facebook::react::TraceSection s(
              "#RNOH::SchedulerDelegate::didMount");
          auto job_partner =
              ffrt::job_partner<ScenarioID::MUTATION_PARALLELIZATION>::
                  get_main_partner(
                      ffrt::job_partner_attr()
                          .max_parallelism(MAX_THREAD_NUM_MUTATION_CREATE)
                          .qos(THREAD_PRIORITY_LEVEL_5));
          auto createBuckets =
              bucketizeCreateMutations(transaction->getMutations());
          ShadowViewMutationList paraCreateMutationList;
          ShadowViewMutationList otherCreateMutationList;
          auto otherMutationList = std::move(createBuckets.deferredTail);

          for (uint64_t i = 0; i < createBuckets.movableCreates.size(); ++i) {
            auto const& mutation = createBuckets.movableCreates[i];
            if (mutation.type == facebook::react::ShadowViewMutation::Create) {
              if (parallelComponentHandles.find(
                      mutation.newChildShadowView.componentHandle) !=
                      parallelComponentHandles.end() ||
                  ComponentNameManager::getInstance().hasComponentName(
                      mutation.newChildShadowView.componentName)) {
                // Resolve the weak_ptr to a shared_ptr up front and null-check
                // it before enqueueing. Capturing the shared_ptr by value
                // guarantees the MountingManager stays alive for the whole FFRT
                // task; calling m_mountingManager.lock() inline would return a
                // temporary dropped at the end of the statement, so a concurrent
                // destroy (e.g. during RNInstance teardown) could turn a later
                // ->handleMutation into a null dereference / use-after-free.
                // We check before pushing to paraCreateMutationList so that a
                // skipped mutation is also excluded from finalizeMutationUpdates.
                auto mountingManagerShared = this->m_mountingManager.lock();
                if (mountingManagerShared == nullptr) {
                  continue;
                }
                paraCreateMutationList.push_back(mutation);
                job_partner->submit([mountingManagerShared, mutation] {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::handleMutation_parallel");
                  try {
                    mountingManagerShared->handleMutation(mutation);
                  } catch (std::exception const& e) {
                    LOG(ERROR) << "Mutation " << mutation.type
                                   << " failed: " << e.what();
                  }
                });
              } else {
                otherCreateMutationList.push_back(mutation);
              }
            }
          }
          auto taskExecutor = this->m_taskExecutor;
          auto weakMountingManager = this->m_mountingManager;
          auto weakScheduler = this->m_scheduler;
          auto transactionCopy = transaction;
          auto paraCreateCopy = std::move(paraCreateMutationList);
          auto otherCreateCopy = std::move(otherCreateMutationList);
          auto otherMutationCopy = std::move(otherMutationList);
          auto taskTraceCopy = taskTrace;
          bool splitMutation = false;
#ifdef SPLIT_MUTATION_ON
          splitMutation = true;
#endif
          const bool inConfigChangeWindow =
              splitMutation && isInConfigChangeWindow();
          const bool inLoadWindow = (deviceType() != "phone") ||
              (!splitMutation) || inConfigChangeWindow ||
              isInInitialLoadWindow(surfaceId);

          static auto contJobRing =
              ffrt::job_ring<true>::make(ffrt::job_ring_attr().qos(5));
          contJobRing->submit([
              job_partner,
              taskExecutor,
              weakMountingManager,
              weakScheduler,
              transactionCopy,
              paraCreateCopy = std::move(paraCreateCopy),
              otherCreateCopy = std::move(otherCreateCopy),
              otherMutationCopy = std::move(otherMutationCopy),
              taskTraceCopy = std::move(taskTraceCopy),
              inLoadWindow]() {
            facebook::react::TraceSection waitS(
                "#RNOH::SchedulerDelegate::parallelCreateWait");
            job_partner->wait();

            auto postToMain =
                [taskExecutor,
                 weakMountingManager](std::function<void(
                     MountingManager::Shared const&)> op) {
              taskExecutor->runTask(
                  TaskThread::MAIN,
                  [weakMountingManager, op = std::move(op)] {
                    if (auto mm = weakMountingManager.lock()) {
                      op(mm);
                    }
                  });
            };

            auto reportMountFor =
                [weakScheduler](
                    MountingManager::Shared const& /*mm*/,
                    const facebook::react::ShadowViewMutationList& muts) {
                  if (auto schedulerShared = weakScheduler.lock()) {
                    std::unordered_set<SurfaceId> mountedSurfaceIds;
                    for (const auto& mutation : muts) {
                      mountedSurfaceIds.insert(
                          getMutationTargetShadowView(mutation).surfaceId);
                    }
                    if (!mountedSurfaceIds.empty()) {
                      for (SurfaceId sid : mountedSurfaceIds) {
                        schedulerShared->reportMount(sid);
                      }
                    }
                  }
                };

            postToMain([paraCreateCopy](
                           MountingManager::Shared const& mountingManager) {
              facebook::react::TraceSection s(
                  "#RNOH::TaskExecutor::runningTask");
              mountingManager->finalizeMutationUpdates(paraCreateCopy);
            });
            postToMain([otherCreateCopy, reportMountFor](
                           MountingManager::Shared const& mountingManager) {
              facebook::react::TraceSection s(
                  "#RNOH::SchedulerDelegate::otherCreate");
              mountingManager->didMount(otherCreateCopy);
              reportMountFor(mountingManager, otherCreateCopy);
            });

            if (inLoadWindow) {
              const size_t otherSize = otherMutationCopy.size();
              const bool sliceInserts =
                  otherSize > 0 && isTextHeavyInsertTail(otherMutationCopy);
              if (sliceInserts) {
                constexpr size_t kInsertSliceSize = 70;
                for (size_t i = 0; i < otherSize; i += kInsertSliceSize) {
                  facebook::react::ShadowViewMutationList slice(
                      otherMutationCopy.begin() +
                          static_cast<std::ptrdiff_t>(i),
                      otherMutationCopy.begin() +
                          static_cast<std::ptrdiff_t>(std::min(
                              otherSize, i + kInsertSliceSize)));
                  postToMain(
                      [slice = std::move(slice),
                       taskTraceCopy,
                       reportMountFor,
                       isLast = (i + kInsertSliceSize >= otherSize)](
                          MountingManager::Shared const& mountingManager) {
                        facebook::react::TraceSection s(taskTraceCopy.c_str());
                        mountingManager->didMount(slice);
                        reportMountFor(mountingManager, slice);
                        if (isLast) {
                          mountingManager->clearPreallocatedViews();
                        }
                      });
                }
              } else {
                if (otherSize > 0) {
                  postToMain(
                      [otherMutationCopy, taskTraceCopy, reportMountFor](
                          MountingManager::Shared const& mountingManager) {
                        facebook::react::TraceSection s(taskTraceCopy.c_str());
                        mountingManager->didMount(otherMutationCopy);
                        reportMountFor(mountingManager, otherMutationCopy);
                      });
                }
                postToMain([](MountingManager::Shared const& mountingManager) {
                  mountingManager->clearPreallocatedViews();
                });
              }
            } else {
              const bool hasCreateInDeferredTail =
                  containsCreateMutation(otherMutationCopy);
              auto split = hasCreateInDeferredTail
                  ? TwoBatchSplit{false, otherMutationCopy, {}}
                  : splitNonCreateMutationsIntoTwoBatches(otherMutationCopy);
              if (split.enabled) {
                auto batchA = std::move(split.batchA);
                auto batchB = std::move(split.batchB);

                postToMain([batchA = std::move(batchA), reportMountFor](
                               MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::other-nonCreate-A size:",
                      batchA.size());
                  mountingManager->didMount(batchA);
                  reportMountFor(mountingManager, batchA);
                });

                NextFrameDispatcher::Get().post(
                    [taskExecutor,
                     weakMountingManager,
                     batchB = std::move(batchB),
                     reportMountFor]() mutable {
                      taskExecutor->runTask(
                          TaskThread::MAIN,
                          [weakMountingManager,
                           batchB = std::move(batchB),
                           reportMountFor] {
                            if (auto mm = weakMountingManager.lock()) {
                              facebook::react::TraceSection s(
                                  "#RNOH::SchedulerDelegate::other-nonCreate-B size:",
                                  batchB.size());
                              mm->didMount(batchB);
                              reportMountFor(mm, batchB);
                              mm->clearPreallocatedViews();
                            }
                          });
                    });
              } else {
                postToMain([otherMutationCopy, taskTraceCopy, reportMountFor](
                               MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::other-and-finalize");
                  mountingManager->didMount(otherMutationCopy);
                  reportMountFor(mountingManager, otherMutationCopy);
                });
                postToMain(
                    [](MountingManager::Shared const& mountingManager) {
                      facebook::react::TraceSection s(
                          "#RNOH::SchedulerDelegate::clearPreallocatedViews");
                      mountingManager->clearPreallocatedViews();
                    });
              }
            }
            postToMain([paraCreateCopy, reportMountFor](
                           MountingManager::Shared const& mountingManager) {
              reportMountFor(mountingManager, paraCreateCopy);
            });
            logTransactionTelemetryMarkers(*transactionCopy);
          });
          facebook::react::TraceSection ss(
              ("#RNOH::TaskExecutor::runTask t" + std::to_string(taskId))
                  .c_str());
          return;
        }
        auto createBuckets =
            bucketizeCreateMutations(transaction->getMutations());
        auto& sliceableMutations = createBuckets.movableCreates;
        auto& otherMutations = createBuckets.deferredTail;
        // The reason for using 70 here is that the average time for each
        // mutation creation is about 60 microseconds,and the time for 70 is 4.2
        // milliseconds which corresponds to the length of half a frame
        size_t maxSliceSize = 70;
        std::vector<facebook::react::ShadowViewMutationList> slicesOfMutations;
        size_t sliceableMutationsSize = sliceableMutations.size();
        if (sliceableMutationsSize > 0) {
          slicesOfMutations.reserve(
              (sliceableMutationsSize + maxSliceSize - 1) / maxSliceSize);
          for (size_t i = 0; i < sliceableMutationsSize; i += maxSliceSize) {
            slicesOfMutations.emplace_back(
                sliceableMutations.begin() + i,
                sliceableMutations.begin() +
                    std::min(sliceableMutationsSize, i + maxSliceSize));
          }
          for (const auto& mutations : slicesOfMutations) {
            performOnMainThread(
                [mutations, this, taskTrace](
                    MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(taskTrace.c_str());
                  mountingManager->didMount(mutations);
                  this->reportMount(mutations);
                });
          }
        }

        bool splitMutation = false;
#ifdef SPLIT_MUTATION_ON
        splitMutation = true;
#endif

        const bool inConfigChangeWindow =
            splitMutation && isInConfigChangeWindow();
        const bool inLoadWindow = (deviceType() != "phone") ||
            (!splitMutation) || inConfigChangeWindow ||
            isInInitialLoadWindow(surfaceId);
        if (inLoadWindow) {
          auto allOther = std::move(otherMutations);
          performOnMainThread(
              [transaction,
               otherMutationList = std::move(allOther),
               taskTrace,
               this](MountingManager::Shared const& mountingManager) {
                facebook::react::TraceSection s(taskTrace.c_str());
                mountingManager->didMount(otherMutationList);
                this->reportMount(otherMutationList);
              });
          performOnMainThread(
              [this](MountingManager::Shared const& mountingManager) {
                mountingManager->clearPreallocatedViews();
              });
        } else {
          // The deferred tail may contain Create mutations (a Create moved to
          // the tail because a non-create for the same tag appeared first).
          // splitNonCreateMutationsIntoTwoBatches has no Create-protection
          // fence, so splitting such a list across two VSync frames could
          // separate a Create from its dependent Insert/Update. Disable the
          // split and emit a single batch instead.
          const bool hasCreateInDeferredTail =
              containsCreateMutation(otherMutations);
          auto split = hasCreateInDeferredTail
              ? TwoBatchSplit{false, otherMutations, {}}
              : splitNonCreateMutationsIntoTwoBatches(otherMutations);
          if (split.enabled) {
            auto batchA = std::move(split.batchA);
            auto batchB = std::move(split.batchB);

            performOnMainThread(
                [batchA, this](
                    MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::other-nonCreate-A size:",
                      batchA.size());
                  mountingManager->didMount(batchA);
                  this->reportMount(batchA);
                });
            performOnMainThreadNextFrame(
                [transaction, batchB, this](
                    MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::other-nonCreate-B size:",
                      batchB.size());
                  mountingManager->didMount(batchB);
                  this->reportMount(batchB);
                  mountingManager->clearPreallocatedViews();
                });
          } else {
            performOnMainThread(
                [transaction, otherMutationList = std::move(otherMutations),
                 taskTrace, this](
                    MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::other-and-finalize");
                  mountingManager->didMount(otherMutationList);
                  this->reportMount(otherMutationList);
                });
            performOnMainThread(
                [this](MountingManager::Shared const& mountingManager) {
                  facebook::react::TraceSection s(
                      "#RNOH::SchedulerDelegate::clearPreallocatedViews");
                  mountingManager->clearPreallocatedViews();
                });
          }
        }
        logTransactionTelemetryMarkers(*transaction);
        facebook::react::TraceSection s(
            ("#RNOH::TaskExecutor::runTask t" + std::to_string(taskId))
                .c_str());
      });
}

void SchedulerDelegate::schedulerShouldRenderTransactions(
    const std::shared_ptr<const MountingCoordinator>& mountingCoordinator) {
  facebook::react::TraceSection s(
      "#RNOH::SchedulerDelegate::schedulerShouldRenderTransactions");
  if (m_transactionState->transactionInFlight) {
    m_transactionState->followUpTransactionRequired = true;
    if (auto textStorageProvider = m_weakTextStorageProvider.lock()) {
      textStorageProvider->clear();
    }
    return;
  }

  do {
    m_transactionState->followUpTransactionRequired = false;
    m_transactionState->transactionInFlight = true;
    performTransaction(mountingCoordinator);
    m_transactionState->transactionInFlight = false;
  } while (m_transactionState->followUpTransactionRequired);
  if (auto textStorageProvider = m_weakTextStorageProvider.lock()) {
    textStorageProvider->clear();
  }
}

void SchedulerDelegate::setScheduler(
    std::shared_ptr<facebook::react::Scheduler> const& scheduler) {
  m_scheduler = scheduler;
}

void SchedulerDelegate::schedulerShouldSynchronouslyUpdateViewOnUIThread(
    facebook::react::Tag tag,
    const folly::dynamic& props) {
  // We will investigate if this method should be implemented
  // Now, it is similar to iOS implementation in RCTScheduler.mm:71-75
}

} // namespace rnoh
