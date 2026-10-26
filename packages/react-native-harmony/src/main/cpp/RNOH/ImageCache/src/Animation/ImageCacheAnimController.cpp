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

#include "ImageCacheAnimController.h"
#include "ImageCacheDispatcher.h"
#include "ImageCacheJobQueue.h"
#include "ImageCacheLog.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheOption.h"
#include "ImageCacheSource.h"
#include "ImageCacheTaskInternal.h"
#include "ImageCacheTaskWorker.h"

namespace ImageCachePro {

namespace {
constexpr uint32_t kDefaultDelayTimeMs = 100;
constexpr uint32_t kMinimumDelayTimeMs = 40;
} // namespace

ImageCacheAnimController::ImageCacheAnimController(
    std::shared_ptr<ImageCacheManagerInternal> manager,
    ImageCacheRequestHandle handle,
    const std::shared_ptr<ImageCacheOption>& option,
    std::shared_ptr<FrameDecodeContext> frameContext,
    ImageCacheGetCacheCompleteCallback onFrameReady,
    AnimationEndCallback onAnimationEnd)
    : ImageCacheRequest(handle, option),
      manager_(std::move(manager)),
      frameContext_(std::move(frameContext)),
      onFrameReady_(std::move(onFrameReady)),
      onAnimationEnd_(std::move(onAnimationEnd)) {
  ImageCacheMetrics::Instance().Objects().Increment(
      "ImageCacheAnimController", this);
  cachedDecodeJob_ = nullptr;
  firstFrame_ = nullptr;
  cachedDecodeJob_ = nullptr;
  consecutiveFailures_.store(0, std::memory_order_release);
  currentFrame_.store(0, std::memory_order_release);
  currentIteration_.store(0, std::memory_order_release);
  option->PrepareTransformations();
}

ImageCacheAnimController::~ImageCacheAnimController() {
  ImageCacheMetrics::Instance().Objects().Decrement(
      "ImageCacheAnimController", this);
  Terminate();
}

void ImageCacheAnimController::SetFirstFrame(
    std::shared_ptr<ImageCacheData> firstFrame) {
  firstFrame_ = std::move(firstFrame);
}

void ImageCacheAnimController::DispatchFrameReady(
    ImageCacheLoadData data,
    ImageCacheLoadResult result) {
  if (!onFrameReady_) {
    return;
  }
  auto mgr = manager_.lock();
  if (!mgr) {
    return;
  }
  auto taskExecutor = mgr->GetTaskExecutor();
  auto opt = GetOption();
  if (opt && opt->IsCallbackOnMainThread() && taskExecutor != nullptr) {
    auto cb = onFrameReady_;
    ImageCacheTaskWorker::ToMainThread(
        taskExecutor,
        [cb = std::move(cb),
         data = std::move(data),
         result = std::move(result)](void*) mutable {
          cb(std::move(data), std::move(result));
        },
        nullptr);
    return;
  }
  onFrameReady_(std::move(data), std::move(result));
}

void ImageCacheAnimController::InvokeCallback(
    std::shared_ptr<rnoh::TaskExecutor> taskExecutor,
    ImageCacheLoadData data,
    ImageCacheLoadResult result) {
  if (!isPendingDecodeFrame_.load(std::memory_order_acquire)) {
    return;
  }

  auto frameIndex = pendingDecodeFrameIndex_.load(std::memory_order_acquire);
  auto isNotifyDisplay = isPendingDisplayFrame_.load(std::memory_order_acquire);
  auto isNotifyError = false;
  if (result.IsSuccess() && data.IsImageData()) {
    auto& dataList = data.GetImageDataList();
    OnFramesDecoded(dataList, frameIndex);
  } else {
    isPendingDecodeFrame_.store(false, std::memory_order_release);
    isNotifyError = true;
  }
  if (!isNotifyDisplay && !isNotifyError) {
    return;
  }
  auto weak = weak_from_this();
  ImageCacheTaskWorker::ToMainThread(
      taskExecutor,
      [weak, isNotifyError](void*) {
        auto self = weak.lock();
        if (self == nullptr) {
          return;
        }
        if (isNotifyError) {
          self->OnFrameDecodeFailed();
          return;
        }
        self->DisplayCurrentFrame();
      },
      nullptr);
}

void ImageCacheAnimController::Start() {
  state_.store(ImageCacheAnimState::RUNNING, std::memory_order_release);
  versionId_.fetch_add(1, std::memory_order_acq_rel);
  if (auto animOpt = animatorOption_) {
    animOpt->onStart();
  }

  if (firstFrame_ != nullptr) {
    DispatchFrameReady(
        ImageCacheLoadData::From({firstFrame_}), ImageCacheLoadResult::Ok());
    firstFrame_ = nullptr;
    PrefetchAndScheduleNext();
    return;
  }
  PrefetchAndScheduleCurrentFrame(GetCurrentFrame());
}

void ImageCacheAnimController::PauseAnimation() {
  auto current = state_.load(std::memory_order_acquire);
  if (current == ImageCacheAnimState::RUNNING) {
    state_.store(ImageCacheAnimState::PAUSED, std::memory_order_release);
    if (auto animOpt = animatorOption_) {
      animOpt->onPause();
    }
  }
}

void ImageCacheAnimController::ResumeAnimation() {
  auto current = state_.load(std::memory_order_acquire);
  if (current == ImageCacheAnimState::PAUSED) {
    state_.store(ImageCacheAnimState::RUNNING, std::memory_order_release);
    if (auto animOpt = animatorOption_) {
      animOpt->onResume();
    }
  } else {
    DLOG(ERROR) << "ResumeAnimation: unexpected state="
                << static_cast<int>(current);
    return;
  }
  DisplayCurrentFrame();
}

void ImageCacheAnimController::Cancel() {
  auto current = state_.load(std::memory_order_acquire);
  if (current == ImageCacheAnimState::STOPPED) {
    return;
  }
  if (auto animOpt = animatorOption_) {
    animOpt->onCancel();
  }
  NotifyAnimationEnd(AnimationResult::CANCELLED);
}

void ImageCacheAnimController::Restart() {
  Terminate();
  state_.store(ImageCacheAnimState::RUNNING, std::memory_order_release);
  versionId_.fetch_add(1, std::memory_order_acq_rel);
  auto animOpt = animatorOption_;
  if (animOpt != nullptr && animOpt->onStart != nullptr) {
    animOpt->onStart();
  }
  PrefetchAndScheduleCurrentFrame(0);
}

void ImageCacheAnimController::Terminate() {
  CancelDisplayTask();
  prefetchBuffer_.Clear();
  CancelPendingDecode();
  currentFrame_.store(0, std::memory_order_release);
  currentIteration_.store(0, std::memory_order_release);
  consecutiveFailures_.store(0, std::memory_order_release);
  state_.store(ImageCacheAnimState::STOPPED, std::memory_order_release);
}

void ImageCacheAnimController::NotifyAnimationEnd(AnimationResult result) {
  Terminate();
  cachedDecodeJob_ = nullptr;
  if (auto endCallback = onAnimationEnd_) {
    endCallback(result);
  }
  auto mgr = manager_.lock();
  if (mgr) {
    const auto handle = GetRequestHandle();
    mgr->RemoveAnimation(handle);
    mgr->RemoveRequest(handle);
  }
}

void ImageCacheAnimController::SetAnimatorOption(
    std::shared_ptr<ImageCacheAnimOption> animatorOpt) {
  auto current = state_.load(std::memory_order_acquire);
  if (current != ImageCacheAnimState::INITIAL) {
    return;
  }
  animatorOption_ = animatorOpt;
  if (animatorOpt != nullptr) {
    iterations_.store(animatorOpt->iterations, std::memory_order_release);
    reverse_.store(animatorOpt->reverse, std::memory_order_release);
  }
}

uint32_t ImageCacheAnimController::GetNextFrame() {
  if (frameContext_ == nullptr) {
    return kInvalidFrameIndex;
  }
  auto current = currentFrame_.load(std::memory_order_acquire);
  current++;
  auto iterations = iterations_.load(std::memory_order_acquire);
  if (current >= frameContext_->frameCount_) {
    if (iterations == 0) {
      current = 0;
    } else {
      auto iteration =
          currentIteration_.fetch_add(1, std::memory_order_acq_rel) + 1;
      if (iteration >= iterations) {
        return kInvalidFrameIndex;
      } else {
        current = 0;
      }
    }
  }
  currentFrame_.store(current, std::memory_order_release);
  return current;
}

uint32_t ImageCacheAnimController::GetFrameDelay(uint32_t frameIndex) const {
  if (!frameContext_) {
    return kDefaultDelayTimeMs;
  }
  if (frameIndex >= frameContext_->frameCount_) {
    return kDefaultDelayTimeMs;
  }
  int32_t* delayList = frameContext_->animationDelayTimeList_.get();
  // Ensure delayList has enough elements - check frameCount_ matches array size
  if (delayList && frameContext_->frameCount_ > frameIndex &&
      delayList[frameIndex] > 0) {
    return std::max(
        static_cast<uint32_t>(delayList[frameIndex]), kMinimumDelayTimeMs);
  }
  return kDefaultDelayTimeMs;
}

void ImageCacheAnimController::OnFramesDecoded(
    const std::vector<std::shared_ptr<ImageCacheData>>& frameList,
    uint32_t frameIdx) {
  consecutiveFailures_.store(0, std::memory_order_relaxed);
  isPendingDecodeFrame_.store(false, std::memory_order_release);
  pendingDecodeFrameIndex_.store(kInvalidFrameIndex, std::memory_order_release);
  prefetchBuffer_.Put(frameIdx, frameContext_->frameCount_, frameList);
}

void ImageCacheAnimController::OnFrameDecodeFailed() {
  int failures =
      consecutiveFailures_.fetch_add(1, std::memory_order_relaxed) + 1;
  isPendingDecodeFrame_.store(false, std::memory_order_release);
  if (failures >= kMaxConsecutiveFailures) {
    NotifyAnimationEnd(AnimationResult::ERROR);
  }
}

void ImageCacheAnimController::PrefetchFrames(uint32_t from) {
  if (!frameContext_ || !IsRunning()) {
    return;
  }

  auto mgr = manager_.lock();
  if (!mgr) {
    return;
  }
  auto dispatcher = mgr->GetDispatcherShared();
  if (!dispatcher || dispatcher->IsDestroyed()) {
    return;
  }

  uint32_t count = prefetchBuffer_.GetPrefetchAhead();
  uint32_t totalFrames = frameContext_->frameCount_;
  uint32_t actualCount = std::min(count, totalFrames);
  if (actualCount == 0) {
    return;
  }

  if (isPendingDecodeFrame_.load(std::memory_order_acquire)) {
    return;
  }

  isPendingDecodeFrame_.store(true, std::memory_order_relaxed);
  pendingDecodeFrameIndex_.store(from, std::memory_order_release);

  auto self = shared_from_this();
  mgr->AddRequest(this->GetRequestHandle(), self);
  dispatcher->Enqueue(
      self,
      [dispatcher](std::shared_ptr<ImageCacheRequest> r, const std::string& key)
          -> std::shared_ptr<ImageCacheJob> { return nullptr; },
      [self, from, actualCount, dispatcher](
          std::shared_ptr<ImageCacheRequest> req)
          -> std::shared_ptr<ImageCacheJob> {
        auto ctrl = std::static_pointer_cast<ImageCacheAnimController>(req);
        if (!ctrl || !ctrl->IsRunning()) {
          return nullptr;
        }
        FrameDecodeRequest decodeReq;
        decodeReq.startFrame = from;
        decodeReq.fetchCount = actualCount;
        decodeReq.frameContext = ctrl->frameContext_;
        if (ctrl->cachedDecodeJob_) {
          ctrl->cachedDecodeJob_->UpdateDecodeRequest(decodeReq);
          return ctrl->cachedDecodeJob_;
        }
        auto job = ImageCacheJob::CreateJob(
            "prefetch#batch#" + std::to_string(ctrl->GetRequestHandle()) + "_" +
                std::to_string(ctrl->GetVersionId()),
            ctrl->option,
            dispatcher,
            ctrl,
            decodeReq);
        ctrl->cachedDecodeJob_ = job;
        return job;
      });
}

void ImageCacheAnimController::ScheduleNextDisplay() {
  if (!frameContext_ || !IsRunning()) {
    return;
  }
  uint32_t curFrame = GetCurrentFrame();
  uint32_t delay = GetFrameDelay(curFrame);
  int vid = GetVersionId();
  auto weak = weak_from_this();
  auto func = [weak, vid]() {
    auto self = weak.lock();
    if (!self) {
      return;
    }
    {
      std::lock_guard<std::mutex> lock(self->displayTaskMutex_);
      self->displayTaskHandle_ = std::nullopt;
    }
    if (vid != self->GetVersionId() || !self->IsRunning()) {
      return;
    }
    self->DisplayCurrentFrame();
  };

  auto mgr = manager_.lock();
  if (!mgr) {
    return;
  }
  CancelDisplayTask();
  auto taskHandle = ImageCacheTaskWorker::RunWithDelay(
      mgr->GetTaskExecutor(), rnoh::TaskThread::MAIN, std::move(func), delay);
  {
    std::lock_guard<std::mutex> lock(displayTaskMutex_);
    displayTaskHandle_ = std::move(taskHandle);
  }
}

void ImageCacheAnimController::DisplayCurrentFrame() {
  if (!IsRunning()) {
    return;
  }
  uint32_t curFrame = GetCurrentFrame();

  auto frameData = prefetchBuffer_.TryGet(curFrame);
  if (frameData) {
    DispatchFrameReady(
        ImageCacheLoadData::From({frameData}),
        ImageCacheLoadResult::Ok(curFrame));
    PrefetchAndScheduleNext();
  } else {
    // Current frame not in cache: prefetch first. Since it is already timed
    // out, mark it as pending display so it is shown directly once available.
    isPendingDisplayFrame_.store(true, std::memory_order_relaxed);
    PrefetchAndScheduleCurrentFrame(curFrame);
  }
}

void ImageCacheAnimController::PrefetchAndScheduleCurrentFrame(
    uint32_t curFrame) {
  if (curFrame == kInvalidFrameIndex) {
    return;
  }
  PrefetchFrames(curFrame);
  ScheduleNextDisplay();
}

void ImageCacheAnimController::PrefetchAndScheduleNext() {
  uint32_t nextFrame = kInvalidFrameIndex;
  std::function<void(uint32_t)> onRepeatCallback = nullptr;
  std::function<void()> onFinishCallback = nullptr;
  {
    nextFrame = GetNextFrame();
    auto animOpt = animatorOption_;
    if (nextFrame == kInvalidFrameIndex) {
      onRepeatCallback = animOpt ? animOpt->onRepeat : nullptr;
      onFinishCallback = animOpt ? animOpt->onFinish : nullptr;
    } else if (nextFrame == 0) {
      onRepeatCallback = animOpt ? animOpt->onRepeat : nullptr;
    }
  }
  auto currentIteration = currentIteration_.load(std::memory_order_acquire);
  if (onRepeatCallback) {
    onRepeatCallback(currentIteration);
  }
  if (nextFrame == kInvalidFrameIndex) {
    if (onFinishCallback != nullptr) {
      onFinishCallback();
    }
    NotifyAnimationEnd(AnimationResult::COMPLETED);
    return;
  }

  if (!prefetchBuffer_.HasFrame(nextFrame)) {
    PrefetchFrames(nextFrame);
  } else if (
      ((nextFrame + 1) < frameContext_->frameCount_) &&
      (currentIteration == 0) && !prefetchBuffer_.HasFrame(nextFrame + 1)) {
    PrefetchFrames(nextFrame + 1);
  }
  ScheduleNextDisplay();
}

void ImageCacheAnimController::CancelDisplayTask() {
  isPendingDisplayFrame_.store(false, std::memory_order_release);
  std::optional<ImageCacheTaskWorker::DelayedTaskHandle> taskHandle;
  {
    std::lock_guard<std::mutex> lock(displayTaskMutex_);
    taskHandle = std::move(displayTaskHandle_);
    displayTaskHandle_ = std::nullopt;
  }
  if (!taskHandle.has_value()) {
    return;
  }
  auto mgr = manager_.lock();
  if (mgr) {
    ImageCacheTaskWorker::CancelDelayedTask(
        mgr->GetTaskExecutor(), std::move(*taskHandle));
  }
}

void ImageCacheAnimController::CancelPendingDecode() {
  isPendingDecodeFrame_.store(false, std::memory_order_release);
  pendingDecodeFrameIndex_.store(kInvalidFrameIndex, std::memory_order_release);
  if (!cachedDecodeJob_) {
    return;
  }
  auto jobKey = cachedDecodeJob_->GetRequestKey();
  auto mgr = manager_.lock();
  if (mgr) {
    auto dispatcher = mgr->GetDispatcherShared();
    if (dispatcher && !dispatcher->IsDestroyed()) {
      dispatcher->CancelJobByKey(jobKey, ImageCacheErrorCode::LOADING_CANCEL);
    }
  }
  cachedDecodeJob_ = nullptr;
}
} // namespace ImageCachePro
