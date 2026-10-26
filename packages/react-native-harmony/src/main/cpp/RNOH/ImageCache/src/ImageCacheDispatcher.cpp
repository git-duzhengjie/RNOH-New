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

#include "ImageCacheDispatcher.h"
#include "ImageCacheDataCacheInternal.h"
#include "ImageCacheDecodeUtil.h"
#include "ImageCacheDownSampler.h"
#include "ImageCacheLoaderInternal.h"
#include "ImageCacheLog.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheMemoryCache.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheOption.h"
#include "ImageCachePixelmapUtils.h"
#include "ImageCacheTaskWorker.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"

#include <algorithm>
#include <vector>
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
#include "ImageCacheEGL.h"
#include "ImageCacheTransformation.h"
#endif

namespace ImageCachePro {

struct PendingCallback {
  std::shared_ptr<ImageCacheRequest> req;
  ImageCacheLoadData data;
  ImageCacheLoadResult result;
};
std::shared_ptr<rnoh::TaskExecutor> ImageCacheDispatcher::GetTaskExecutor()
    const {
  auto m = managerWeak_.lock();
  if (m == nullptr)
    return nullptr;
  return m->GetTaskExecutor();
}

std::shared_ptr<ImageCacheRequest> ImageCacheDispatcher::GetAndRemoveRequest(
    ImageCacheRequestHandle handle) const {
  auto m = managerWeak_.lock();
  if (m == nullptr)
    return nullptr;
  auto req = m->GetRequest(handle);
  if (req == nullptr) {
    return nullptr;
  }
  m->RemoveRequest(handle);
  return req;
}

std::string ImageCacheDispatcher::GenerateRequestKey(
    const ImageCacheOption& option) const {
  auto m = managerWeak_.lock();
  if (m == nullptr)
    return {};
  auto* keyGen = m->GetCacheKeyGenerator();
  if (keyGen == nullptr)
    return {};
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &option.loadSrc, ImageCacheRequestType::MAIN_SRC, option, false);
  params.transformationInfo = option.ResolveTransformationInfo();
  if (params.downSamplingInfo.empty()) {
    params.downSamplingInfo = ImageCacheDownSampler::GetDownSamplingInfo(
        option.requestImageSize, option.downSampling, option.resizeMultiplier);
  }
  return keyGen->GenerateRequestKey(params);
}

std::string ImageCacheDispatcher::GenerateMemoryKey(
    const ImageCacheOption& option,
    bool isAnimated) const {
  auto m = managerWeak_.lock();
  if (m == nullptr)
    return {};
  auto* keyGen = m->GetCacheKeyGenerator();
  if (keyGen == nullptr) {
    return {};
  }
  ImageCacheKeyGenerator::MemoryKeyParams params(
      &option.loadSrc, ImageCacheRequestType::MAIN_SRC, option, isAnimated);
  params.transformationInfo = option.ResolveTransformationInfo();
  if (params.downSamplingInfo.empty()) {
    params.downSamplingInfo = ImageCacheDownSampler::GetDownSamplingInfo(
        option.requestImageSize, option.downSampling, option.resizeMultiplier);
  }
  return keyGen->GenerateMemoryKey(params);
}

static std::shared_ptr<ImageCacheData> GetMemoryCacheData(
    const std::string& memoryKey) {
  if (memoryKey.empty()) {
    return nullptr;
  }
  auto dataCache = ImageCacheMemoryCache::GetInstance()->Get(memoryKey);
  if (dataCache == nullptr) {
    return nullptr;
  }
  auto internal =
      std::dynamic_pointer_cast<ImageCacheDataCacheInternal>(dataCache);
  if (internal == nullptr || internal->GetFrameDecodeContext() != nullptr) {
    return nullptr;
  }
  return internal->GetImageData();
}

std::shared_ptr<ImageCacheData> ImageCacheDispatcher::GetCachedImageSync(
    const std::shared_ptr<ImageCacheOption>& option) {
  IMAGE_CACHE_CHECK_RETURN(option == nullptr, nullptr);
  option->PrepareTransformations();
  auto requestKey = GenerateRequestKey(*option);
  auto data = GetMemoryCacheData(requestKey);
  if (data != nullptr) {
    return data;
  }
  return GetMemoryCacheData(GenerateMemoryKey(*option, true));
}

ImageCacheDispatcher::ImageCacheDispatcher(
    std::weak_ptr<ImageCacheManagerInternal> manager)
    : managerWeak_(std::move(manager)) {
  ImageCacheMetrics::Instance().Objects().Increment(
      "ImageCacheDispatcher", this);
}

ImageCacheDispatcher::~ImageCacheDispatcher() {
  ImageCacheMetrics::Instance().Objects().Decrement(
      "ImageCacheDispatcher", this);
  StopSuspendCheckTimer();
  if (!destroyed_.load(std::memory_order_acquire)) {
    Shutdown();
  }
}

void ImageCacheDispatcher::SetMaxRequests(int concurrency) {
  std::lock_guard<std::mutex> lock(queueMutex_);
  maxRequests_ = concurrency;
}

bool ImageCacheDispatcher::ScheduleNext(
    std::shared_ptr<ImageCacheTaskInternal>& outTask) {
  std::lock_guard<std::mutex> lock(queueMutex_);
  if (destroyed_.load(std::memory_order_acquire)) {
    return false;
  }
  if (runningCount_.load() >= maxRequests_ || readyQueue_.Empty()) {
    return false;
  }
  auto* taskItem = readyQueue_.Front();

  // Capture shared_ptr BEFORE Remove to guarantee task lifetime.
  auto jobPtr = taskItem->GetJob();
  outTask = (jobPtr != nullptr) ? jobPtr->task : nullptr;
  readyQueue_.Remove(taskItem);

  if (outTask == nullptr || outTask->ShouldAbort()) {
    if (outTask != nullptr) {
      TransitionToClosedAtomic(outTask.get());
    }
    return false;
  }
  outTask->TransitionTo(
      ImageCacheTaskState::READY, ImageCacheTaskState::RUNNING);
  runningQueue_.PushBack(outTask.get());
  runningCount_.fetch_add(1, std::memory_order_relaxed);
  return true;
}

void ImageCacheDispatcher::CompleteTask(
    std::shared_ptr<ImageCacheTaskInternal> task) {
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (task->state.load(std::memory_order_relaxed) ==
        ImageCacheTaskState::RUNNING) {
      if (task->listNode.IsLinked()) {
        runningQueue_.Remove(task.get());
        runningCount_.fetch_sub(1, std::memory_order_relaxed);
      }
    } else if (
        task->state.load(std::memory_order_relaxed) ==
        ImageCacheTaskState::SUSPEND) {
      if (task->listNode.IsLinked()) {
        suspendQueue_.Remove(task.get());
      }
    }
  }
  task->ForceTransitionTo(ImageCacheTaskState::DONE);
}

void ImageCacheDispatcher::SuspendTask(
    std::shared_ptr<ImageCacheTaskInternal> task) {
  std::lock_guard<std::mutex> lock(queueMutex_);
  if (!task->TransitionTo(
          ImageCacheTaskState::RUNNING, ImageCacheTaskState::SUSPEND)) {
    return;
  }
  runningQueue_.Remove(task.get());
  suspendQueue_.PushBack(task.get());
  runningCount_.fetch_sub(1, std::memory_order_relaxed);
  StartSuspendCheckTimer();
}

bool ImageCacheDispatcher::ResumeTask(
    std::shared_ptr<ImageCacheTaskInternal> task) {
  if (task->ShouldAbort()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(queueMutex_);
  if (!task->TransitionTo(
          ImageCacheTaskState::SUSPEND, ImageCacheTaskState::RUNNING)) {
    return false;
  }
  suspendQueue_.Remove(task.get());
  runningQueue_.PushBack(task.get());
  runningCount_.fetch_add(1, std::memory_order_relaxed);
  if (suspendQueue_.Empty()) {
    StopSuspendCheckTimer();
  }
  return true;
}

void ImageCacheDispatcher::ResumeTask(
    std::shared_ptr<ImageCacheTaskInternal> task,
    bool result) {
  if (task == nullptr) {
    return;
  }
  if (IsDestroyed()) {
    return;
  }

  if (!result || task->ShouldAbort() || !task->IsSuccess()) {
    if (!ResumeTask(task)) {
      return;
    }
    OnTaskDone(task, false);
    return;
  }

  if (!ResumeTask(task)) {
    return;
  }
  PushTrackedWorkerTask(
      [self = shared_from_this()](std::shared_ptr<ImageCacheTaskInternal> t) {
        self->ExecuteTask(t);
      },
      task);
}

void ImageCacheDispatcher::TrySchedule() {
  if (destroyed_.load()) {
    return;
  }
  auto self = shared_from_this();
  std::shared_ptr<ImageCacheTaskInternal> task;
  while (ScheduleNext(task)) {
    PushTrackedWorkerTask(
        [self](std::shared_ptr<ImageCacheTaskInternal> t) {
          self->ExecuteTask(t);
        },
        task);
    task.reset();
  }
}

void ImageCacheDispatcher::TransitionToClosedAtomic(
    ImageCacheTaskInternal* task) {
  task->ForceTransitionTo(ImageCacheTaskState::CLOSED);
}

void ImageCacheDispatcher::PushTrackedWorkerTask(
    TrackedWorkerTask execute,
    std::shared_ptr<ImageCacheTaskInternal> task) {
  if (!execute || task == nullptr) {
    return;
  }
  if (destroyed_.load(std::memory_order_acquire)) {
    task->SetError(ImageCacheErrorCode::LOADING_CANCEL);
    CompleteTask(task);
    return;
  }
  auto taskExecutor = GetTaskExecutor();
  if (taskExecutor == nullptr) {
    task->SetError(ImageCacheErrorCode::UNKNOWN_ERROR);
    CompleteTask(task);
    return;
  }
  auto self = shared_from_this();
  ImageCacheTaskWorker::PushTask(
      taskExecutor,
      [self, execute = std::move(execute)](
          std::shared_ptr<ImageCacheTaskInternal> task) mutable {
        if (self->destroyed_.load(std::memory_order_acquire)) {
          task->SetError(ImageCacheErrorCode::LOADING_CANCEL);
          self->CompleteTask(task);
          return;
        }
        execute(task);
      },
      nullptr,
      task);
}

std::shared_ptr<ImageCacheLoaderInternal>
ImageCacheDispatcher::GetDefaultLoaderInternal() const {
  if (destroyed_.load()) {
    return nullptr;
  }
  auto m = managerWeak_.lock();
  if (m == nullptr)
    return nullptr;
  return std::dynamic_pointer_cast<ImageCacheLoaderInternal>(
      m->GetDefaultLoader());
}

bool ImageCacheDispatcher::ChooseLoader(
    std::shared_ptr<ImageCacheRequest> request) {
  (void)request;
  return GetDefaultLoaderInternal() != nullptr;
}

void ImageCacheDispatcher::StartSuspendCheckTimer() {
  StopSuspendCheckTimer();
  auto taskExecutor = GetTaskExecutor();
  if (taskExecutor == nullptr) {
    return;
  }
  auto weakSelf = weak_from_this();
  auto handle = ImageCacheTaskWorker::RunWithDelay(
      taskExecutor,
      rnoh::TaskThread::WORKER,
      [weakSelf]() {
        auto self = weakSelf.lock();
        if (!self || self->IsDestroyed()) {
          return;
        }
        self->CheckSuspendTimeout();
      },
      5000,
      5000);
  if (handle.has_value()) {
    suspendCheckTimer_ = std::move(*handle);
  }
}

void ImageCacheDispatcher::StopSuspendCheckTimer() {
  if (!suspendCheckTimer_.has_value()) {
    return;
  }
  auto taskExecutor = GetTaskExecutor();
  if (taskExecutor != nullptr) {
    ImageCacheTaskWorker::CancelDelayedTask(
        taskExecutor, std::move(*suspendCheckTimer_));
  }
  suspendCheckTimer_.reset();
}

std::shared_ptr<ImageCacheJob> ImageCacheDispatcher::GetJob(
    const std::string& requestKey) {
  std::lock_guard<std::mutex> lock(jobMapMutex_);
  auto it = jobMap_.find(requestKey);
  if (it != jobMap_.end() && it->second != nullptr) {
    return it->second;
  }
  return nullptr;
}

void ImageCacheDispatcher::Enqueue(
    std::shared_ptr<ImageCacheRequest> request,
    MergeFunc&& mergeFunc,
    CreateJobFunc&& createJobFunc) {
  if (request == nullptr || !createJobFunc) {
    DLOG(ERROR) << "Invalid arg";
    return;
  }
  if (destroyed_.load()) {
    request->InvokeCallback(
        nullptr,
        ImageCacheLoadData::None(),
        ImageCacheLoadResult::Failure(ImageCacheErrorCode::UNKNOWN_ERROR));
    return;
  }
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalRequest);
  auto requestKey = GenerateRequestKey(*request->GetOption());
  auto existingJob = mergeFunc ? mergeFunc(request, requestKey) : nullptr;
  if (existingJob != nullptr) {
    existingJob->AddRequest(request);
    ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalMerged);
    return;
  }

  auto job = std::move(createJobFunc)(request);
  if (job == nullptr) {
    request->InvokeCallback(
        GetTaskExecutor(),
        ImageCacheLoadData::None(),
        ImageCacheLoadResult::Failure(ImageCacheErrorCode::UNKNOWN_ERROR));
    return;
  }
  {
    std::lock_guard<std::mutex> lock(jobMapMutex_);
    jobMap_[job->GetRequestKey()] = job;
  }
  job->AddRequest(request);
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    job->task->SetPriority(job->priority);
    readyQueue_.InsertByPriority(job->task.get(), job->priority);
  }
  ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalSubmitted);
  TrySchedule();
}

void ImageCacheDispatcher::Dequeue(
    const std::shared_ptr<ImageCacheRequest>& request) {
  auto requestKey = GenerateRequestKey(*request->GetOption());
  std::shared_ptr<ImageCacheJob> job;
  {
    std::lock_guard<std::mutex> lock(jobMapMutex_);
    auto it = jobMap_.find(requestKey);
    if (it == jobMap_.end() || it->second == nullptr) {
      return;
    }
    job = it->second;
  }

  bool shouldCancelJob = job->RemoveRequest(request->GetRequestHandle());
  if (!shouldCancelJob) {
    return;
  }

  // Cancel download directly without re-looking up the job
  AbortJobs({requestKey}, ImageCacheErrorCode::LOADING_CANCEL);
}

void ImageCacheDispatcher::CancelJobByKey(
    const std::string& jobKey,
    ImageCacheErrorCode reason) {
  if (jobKey.empty() || destroyed_.load(std::memory_order_acquire)) {
    return;
  }
  AbortJobs({jobKey}, reason);
}

void ImageCacheDispatcher::ExecuteTask(
    std::shared_ptr<ImageCacheTaskInternal> task) {
  if (task->ShouldAbort()) {
    OnTaskDone(task, false);
    return;
  }

  auto loader = GetDefaultLoaderInternal();
  bool success = task->Execute(loader);
  if (task->IsSuspended()) {
    return;
  }
  OnTaskDone(task, success);
}

void ImageCacheDispatcher::OnTaskDone(
    std::shared_ptr<ImageCacheTaskInternal> task,
    bool success) {
  if (task == nullptr) {
    DLOG(WARNING) << "OnTaskDone: task is nullptr";
    return;
  }

  if (task->IsSuspended()) {
    return;
  }

  if (!task->TryMarkCompletionHandled()) {
    return;
  }

  const bool isDestroyed = destroyed_.load();
  auto taskExecutor = GetTaskExecutor();
  if (taskExecutor == nullptr && !isDestroyed) {
    CompleteTask(task);
    return;
  }

  auto job = task->GetJob();
  if (job == nullptr) {
    CompleteTask(task);
    return;
  }

  if (!job->requestKey.empty()) {
    std::lock_guard<std::mutex> lock(jobMapMutex_);
    jobMap_.erase(job->requestKey);
  }

  if (!isDestroyed) {
    NotifyCompletedRequests(task, success, taskExecutor);
  }
  CompleteTask(task);
  if (!isDestroyed) {
    TrySchedule();
  }
}

void ImageCacheDispatcher::NotifyCompletedRequests(
    std::shared_ptr<ImageCacheTaskInternal> task,
    bool success,
    std::shared_ptr<rnoh::TaskExecutor> taskExecutor) {
  auto job = task->GetJob();
  if (job == nullptr) {
    return;
  }

  std::vector<PendingCallback> mainThreadCallbacks;
  auto successLoadData = ImageCacheLoadData::None();
  auto errorCode = task->GetError();
  // Statistics: completed / cancelled / failed are mutually exclusive.
  if (success) {
    successLoadData = ImageCacheLoadData::From(
        task->product.imageDataList, task->product.frameDecodeContext);
    ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalCompleted);
  } else {
    if (errorCode == ImageCacheErrorCode::LOADING_CANCEL ||
        errorCode == ImageCacheErrorCode::SUSPEND_TIMEOUT) {
      ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalCancelled);
    } else {
      ImageCacheMetrics::Instance().Increment(MetricsCounter::TotalFailed);
    }
  }
  auto failedCode = (errorCode == ImageCacheErrorCode::OK)
      ? ImageCacheErrorCode::LOADING_CANCEL
      : errorCode;

  job->NotifyRequests([&](ImageCacheRequestHandle rid) {
    auto req = GetAndRemoveRequest(rid);
    if (req == nullptr) {
      DLOG(WARNING)
          << "ImageCache NotifyCompletedRequests: request not found for rid="
          << rid << " requestKey=" << job->requestKey;
      return;
    }
    ImageCacheLoadData loadData;
    ImageCacheLoadResult loadResult;
    if (success) {
      loadData = successLoadData;
      loadResult = ImageCacheLoadResult::Ok();
    } else {
      loadData = ImageCacheLoadData::None();
      loadResult = ImageCacheLoadResult::Failure(failedCode);
    }

    auto option = req->GetOption();
    if (option && option->IsCallbackOnMainThread()) {
      mainThreadCallbacks.push_back(
          {std::move(req), std::move(loadData), std::move(loadResult)});
    } else {
      req->InvokeCallback(
          taskExecutor, std::move(loadData), std::move(loadResult));
    }
  });

  task->product.imageDataList.clear();

  DLOG(INFO) << "ImageCache NotifyCompletedRequests: task="
             << task->GetTaskInfo() << " success=" << success
             << " callbacks=" << mainThreadCallbacks.size();
  if (mainThreadCallbacks.empty()) {
    return;
  }
  auto executorCopy = taskExecutor;
  ImageCacheTaskWorker::ToMainThread(
      taskExecutor,
      [callbacks = std::move(mainThreadCallbacks),
       executor = std::move(executorCopy)](void*) mutable {
        for (auto& cb : callbacks) {
          cb.req->InvokeCallback(
              executor, std::move(cb.data), std::move(cb.result));
        }
      },
      nullptr);
}

void ImageCacheDispatcher::DispatchNextJob() {
  TrySchedule();
}

void ImageCacheDispatcher::CheckSuspendTimeout() {
  // Collect timed-out tasks (can't remove from IntrList during traversal)
  std::vector<std::string> timedJobOut;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    auto* item = suspendQueue_.Front();
    while (item != nullptr) {
      auto nextNode = TaskList::GetNextNode(&item->listNode);
      if (item->IsSuspendTimedOut()) {
        timedJobOut.push_back(item->GetJobKey());
      }
      // Advance to next using safe method from IntrList
      item = nextNode ? TaskList::GetNextFromNode(nextNode) : nullptr;
    }
  }

  AbortJobs(timedJobOut, ImageCacheErrorCode::SUSPEND_TIMEOUT);
}

void ImageCacheDispatcher::AbortJobs(
    const std::vector<std::string>& jobKeys,
    ImageCacheErrorCode reason) {
  bool shouldStopSuspendTimer = false;
  for (const auto& jobKey : jobKeys) {
    auto job = GetJob(jobKey);
    if (job == nullptr) {
      continue;
    }

    auto loader = GetDefaultLoaderInternal();
    if (loader != nullptr && job->task != nullptr) {
      loader->CancelLoad(job->task);
    }

    auto task = job->task;
    if (task == nullptr) {
      continue;
    }

    const auto state = task->state.load(std::memory_order_acquire);
    if (state == ImageCacheTaskState::RUNNING) {
      continue;
    }

    if (state == ImageCacheTaskState::SUSPEND) {
      ResumeTask(task);
    } else if (state == ImageCacheTaskState::READY) {
      std::lock_guard<std::mutex> lock(queueMutex_);
      if (task->listNode.IsLinked()) {
        readyQueue_.Remove(task.get());
      }
    }

    TASK_SET_ERROR(task, reason);
    OnTaskDone(task, false);
  }

  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (suspendQueue_.Empty()) {
      shouldStopSuspendTimer = true;
    }
  }
  if (shouldStopSuspendTimer) {
    StopSuspendCheckTimer();
  }
}

void ImageCacheDispatcher::Shutdown() {
  destroyed_.store(true, std::memory_order_release);
  bool shouldStopSuspendTimer = false;

  // 1. Collect job keys from the suspend queue
  std::vector<std::string> suspendedJobs;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    for (auto* item = suspendQueue_.Front(); item != nullptr;) {
      auto* next = TaskList::GetNextNode(&item->listNode);
      suspendedJobs.push_back(item->GetJobKey());
      item = next ? TaskList::GetNextFromNode(next) : nullptr;
    }
  }
  if (!suspendedJobs.empty()) {
    AbortJobs(suspendedJobs, ImageCacheErrorCode::LOADING_CANCEL);
  }

  std::vector<std::string> readyJobs;
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    for (auto* item = readyQueue_.Front(); item != nullptr;) {
      auto* next = TaskList::GetNextNode(&item->listNode);
      readyJobs.push_back(item->GetJobKey());
      item = next ? TaskList::GetNextFromNode(next) : nullptr;
    }
  }
  if (!readyJobs.empty()) {
    AbortJobs(readyJobs, ImageCacheErrorCode::LOADING_CANCEL);
  }
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!runningQueue_.Empty()) {
      runningQueue_.Remove(runningQueue_.Front());
    }
    while (!suspendQueue_.Empty()) {
      suspendQueue_.Remove(suspendQueue_.Front());
    }
    while (!readyQueue_.Empty()) {
      readyQueue_.Remove(readyQueue_.Front());
    }
    runningCount_.store(0, std::memory_order_relaxed);
    shouldStopSuspendTimer = suspendQueue_.Empty();
  }

  if (shouldStopSuspendTimer) {
    StopSuspendCheckTimer();
  }

  {
    std::lock_guard<std::mutex> lock(jobMapMutex_);
    jobMap_.clear();
  }
}

std::shared_ptr<ImageCacheData> ImageCacheDispatcher::LoadImageSync(
    const std::shared_ptr<ImageCacheOption>& option) {
  if (option == nullptr) {
    DLOG(WARNING) << "LoadImageSync: option is nullptr";
    return nullptr;
  }

  option->PrepareTransformations();
  auto taskExecutor = GetTaskExecutor();
  if (taskExecutor == nullptr) {
    DLOG(WARNING) << "LoadImageSync: manager is gone";
    return nullptr;
  }

  auto data = GetCachedImageSync(option);
  if (data != nullptr) {
    ImageCacheMetrics::Instance().Increment(MetricsCounter::SyncLoadHits);
    return data;
  }

  auto requestKey = GenerateRequestKey(*option);
  auto job = ImageCacheJob::CreateJob(requestKey, option, shared_from_this());
  if (job == nullptr || job->task == nullptr) {
    return nullptr;
  }

  auto loader = GetDefaultLoaderInternal();
  if (!job->task->Execute(loader)) {
    return nullptr;
  }

  data = GetCachedImageSync(option);
  if (data != nullptr) {
    ImageCacheMetrics::Instance().Increment(MetricsCounter::SyncLoadHits);
    return data;
  }
  ImageCacheMetrics::Instance().Increment(MetricsCounter::SyncLoadMisses);
  return nullptr;
}
} // namespace ImageCachePro
