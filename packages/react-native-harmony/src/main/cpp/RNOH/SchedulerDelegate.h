/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/scheduler/SchedulerDelegate.h>
#include "RNOH/ComponentInstancePreallocationRequestQueue.h"
#include "RNOH/MountingManager.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "TextStorageProvider.h"

namespace rnoh {

/**
 * @ThreadSafe
 *
 * Implementation of the react::SchedulerDelegate interface.
 * Schedules operations on the main thread
 * and delegates them to the MountingManager.
 */
class SchedulerDelegate final : public facebook::react::SchedulerDelegate {
  using MountingCoordinator = facebook::react::MountingCoordinator;
  using ShadowNode = facebook::react::ShadowNode;
  using ShadowView = facebook::react::ShadowView;
  using SurfaceId = facebook::react::SurfaceId;

 public:
  SchedulerDelegate(
      MountingManager::Shared mountingManager,
      TaskExecutor::Shared taskExecutor,
      ComponentInstancePreallocationRequestQueue::Weak
          weakPreallocationRequestQueue,
      TextStorageProvider::Weak weakTextStorageProvider)
      : m_mountingManager(mountingManager),
        m_taskExecutor(taskExecutor),
        m_weakPreallocationRequestQueue(
            std::move(weakPreallocationRequestQueue)),
        m_weakTextStorageProvider(std::move(weakTextStorageProvider)){};

  ~SchedulerDelegate() override;

  void schedulerShouldRenderTransactions(
      const std::shared_ptr<const MountingCoordinator>& mountingCoordinator)
      override;

  void schedulerDidFinishTransaction(
      const std::shared_ptr<const MountingCoordinator>& mountingCoordinator)
      override;

  static void markConfigurationChange();
  static void notifyModalVisibilityChanged(bool visible);

  void schedulerDidRequestPreliminaryViewAllocation(
      const ShadowNode& shadowView) override;

  void schedulerDidDispatchCommand(
      const ShadowView& shadowView,
      std::string const& commandName,
      folly::dynamic const& args) override;

  void schedulerDidSendAccessibilityEvent(
      const ShadowView& shadowView,
      std::string const& eventType) override;

  void schedulerDidSetIsJSResponder(
      ShadowView const& shadowView,
      bool isJSResponder,
      bool blockNativeResponder) override;

  void schedulerShouldSynchronouslyUpdateViewOnUIThread(
      facebook::react::Tag tag,
      const folly::dynamic& props) override;

  void schedulerDidUpdateShadowTree(
      const std::unordered_map<facebook::react::Tag, folly::dynamic>&
          tagToProps) override;

  // RN 0.88 新增的纯虚接口：视图快照（view snapshotting）与 revision 合并。
  // 鸿蒙暂不实现快照能力，做 no-op 处理。
  void schedulerShouldMergeReactRevision(facebook::react::SurfaceId surfaceId)
      override;

  void schedulerDidCaptureViewSnapshot(
      facebook::react::Tag tag,
      facebook::react::SurfaceId surfaceId) override;

  void schedulerDidSetViewSnapshot(
      facebook::react::Tag sourceTag,
      facebook::react::Tag targetTag,
      facebook::react::SurfaceId surfaceId) override;

  void schedulerDidClearPendingSnapshots() override;

  void setScheduler(
      std::shared_ptr<facebook::react::Scheduler> const& scheduler);

  void performTransaction(
      const std::shared_ptr<const MountingCoordinator>& mountingCoordinator);

 private:
  void reportMount(const facebook::react::ShadowViewMutationList& mutations);
  template <typename Operation>
  void performOnMainThread(Operation operation) {
    m_taskExecutor->runTask(
        TaskThread::MAIN,
        [weakMountingManager = m_mountingManager,
         operation = std::move(operation)] {
          if (auto mountingManager = weakMountingManager.lock()) {
            operation(mountingManager);
          }
        });
  }

  struct TransactionState final {
    bool transactionInFlight = false;
    bool followUpTransactionRequired = false;
  };

  MountingManager::Weak m_mountingManager;
  TaskExecutor::Shared m_taskExecutor;
  ComponentInstancePreallocationRequestQueue::Weak
      m_weakPreallocationRequestQueue;
  TextStorageProvider::Weak m_weakTextStorageProvider;
  std::shared_ptr<TransactionState> m_transactionState =
      std::make_shared<TransactionState>();
  std::weak_ptr<facebook::react::Scheduler> m_scheduler{};
};

}; // namespace rnoh
