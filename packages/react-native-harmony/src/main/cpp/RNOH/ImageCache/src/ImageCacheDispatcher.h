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

#ifndef IMAGE_CACHE_DISPATCHER_H
#define IMAGE_CACHE_DISPATCHER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include "ImageCacheConfig.h"
#include "ImageCacheData.h"
#include "ImageCacheJobQueue.h"
#include "ImageCacheKeyGenerator.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheOption.h"
#include "ImageCacheSource.h"
#include "ImageCacheTaskInternal.h"
#include "ImageCacheTaskWorker.h"

namespace ImageCachePro {

class ImageCacheManagerInternal;
class ImageCacheLoaderInternal;
class ImageCacheDataCacheInternal;

class ImageCacheDispatcher
    : public std::enable_shared_from_this<ImageCacheDispatcher> {
 public:
  explicit ImageCacheDispatcher(
      std::weak_ptr<ImageCacheManagerInternal> manager);
  ~ImageCacheDispatcher();
  void Shutdown();

  // -- Job entry (called by Manager) --
  using MergeFunc = std::function<std::shared_ptr<ImageCacheJob>(
      std::shared_ptr<ImageCacheRequest>,
      const std::string& requestKey)>;
  using CreateJobFunc = std::function<std::shared_ptr<ImageCacheJob>(
      std::shared_ptr<ImageCacheRequest>)>;
  void Enqueue(
      std::shared_ptr<ImageCacheRequest> request,
      MergeFunc&& mergeFunc,
      CreateJobFunc&& createJobFunc);
  void Dequeue(const std::shared_ptr<ImageCacheRequest>& request);
  void CancelJobByKey(
      const std::string& jobKey,
      ImageCacheErrorCode reason = ImageCacheErrorCode::LOADING_CANCEL);
  std::shared_ptr<ImageCacheJob> GetJob(const std::string& requestKey);

  // -- Task lifecycle (called by TaskInternal) --
  void SuspendTask(std::shared_ptr<ImageCacheTaskInternal> task);
  bool ResumeTask(std::shared_ptr<ImageCacheTaskInternal> task);
  void ResumeTask(std::shared_ptr<ImageCacheTaskInternal> task, bool result);
  void NotifyCompletedRequests(
      std::shared_ptr<ImageCacheTaskInternal> task,
      bool success,
      std::shared_ptr<rnoh::TaskExecutor> taskExecutor);

  // -- Key generation & cache lookup (called by Manager) --
  std::string GenerateRequestKey(const ImageCacheOption& option) const;
  std::string GenerateMemoryKey(
      const ImageCacheOption& option,
      bool isAnimated = false) const;
  std::shared_ptr<ImageCacheData> GetCachedImageSync(
      const std::shared_ptr<ImageCacheOption>& option);
  std::shared_ptr<ImageCacheData> LoadImageSync(
      const std::shared_ptr<ImageCacheOption>& option);

  void SetMaxRequests(int concurrency);
  bool IsDestroyed() const {
    return destroyed_.load(std::memory_order_acquire);
  }

 private:
  // -- Worker dispatch --
  void ExecuteTask(std::shared_ptr<ImageCacheTaskInternal> task);
  using TrackedWorkerTask =
      std::function<void(std::shared_ptr<ImageCacheTaskInternal> task)>;
  void PushTrackedWorkerTask(
      TrackedWorkerTask execute,
      std::shared_ptr<ImageCacheTaskInternal> task);
  void OnTaskDone(std::shared_ptr<ImageCacheTaskInternal> task, bool success);

  // -- Manager access --
  std::weak_ptr<ImageCacheManagerInternal> managerWeak_;
  std::shared_ptr<rnoh::TaskExecutor> GetTaskExecutor() const;
  std::shared_ptr<ImageCacheRequest> GetAndRemoveRequest(
      ImageCacheRequestHandle handle) const;

  // -- Loader helpers --
  std::shared_ptr<ImageCacheLoaderInternal> GetDefaultLoaderInternal() const;
  bool ChooseLoader(std::shared_ptr<ImageCacheRequest> request);

  // -- Task execution pipeline (internal) --
  void DispatchNextJob();
  void CheckSuspendTimeout();
  void AbortJobs(
      const std::vector<std::string>& jobKeys,
      ImageCacheErrorCode reason);

  // -- State --
  std::atomic<bool> destroyed_{false};
  int maxRequests_{8};
  std::mutex jobMapMutex_;
  std::unordered_map<std::string, std::shared_ptr<ImageCacheJob>> jobMap_;

  // -- Queues --
  using TaskList = ImageCacheIntrList<
      ImageCacheTaskInternal,
      &ImageCacheTaskInternal::listNode>;
  std::mutex queueMutex_;
  TaskList readyQueue_;
  TaskList runningQueue_;
  TaskList suspendQueue_;
  std::atomic<int> runningCount_{0};

  // -- Schedule helpers --
  bool ScheduleNext(std::shared_ptr<ImageCacheTaskInternal>& outTask);
  void CompleteTask(std::shared_ptr<ImageCacheTaskInternal> task);
  void TrySchedule();
  void TransitionToClosedAtomic(ImageCacheTaskInternal* task);

  // -- Suspend timeout timer --
  std::optional<ImageCacheTaskWorker::DelayedTaskHandle> suspendCheckTimer_;
  void StartSuspendCheckTimer();
  void StopSuspendCheckTimer();
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DISPATCHER_H
