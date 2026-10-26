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

#ifndef IMAGE_CACHE_ANIM_CONTROLLER_H
#define IMAGE_CACHE_ANIM_CONTROLLER_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "../ImageCacheConfig.h"
#include "../ImageCacheRequest.h"
#include "../ImageCacheTaskWorker.h"
#include "../include/ImageCacheAnimCallback.h"
#include "ImageCachePrefetchBuffer.h"

namespace rnoh {
class TaskExecutor;
}

namespace ImageCachePro {

struct FrameDecodeContext;
struct ImageCacheData;
class ImageCacheManagerInternal;
class ImageCacheDispatcher;

enum class ImageCacheAnimState : int32_t {
  INITIAL = 0,
  RUNNING = 1,
  PAUSED = 2,
  STOPPED = 3,
};

// -- AnimController = Request + animation controller --
// Inherits ImageCacheRequest; frame-decode callbacks are routed via
// InvokeCallback. id = AnimHandle (allocated by Manager), option = frame-decode
// options. Does not hold a timer task handle; all delayed callbacks are
// invalidated via versionId_.
class ImageCacheAnimController
    : public ImageCacheRequest,
      public std::enable_shared_from_this<ImageCacheAnimController> {
 public:
  ImageCacheAnimController(
      std::shared_ptr<ImageCacheManagerInternal> manager,
      ImageCacheRequestHandle handle,
      const std::shared_ptr<ImageCacheOption>& option,
      std::shared_ptr<FrameDecodeContext> frameContext,
      ImageCacheGetCacheCompleteCallback onFrameReady,
      AnimationEndCallback onAnimationEnd);

  ~ImageCacheAnimController() override;

  ImageCacheHandleType GetRequestType() const override {
    return ImageCacheHandleType::ANIMATION;
  }

  // -- ImageCacheRequest polymorphic callbacks --
  void InvokeCallback(
      std::shared_ptr<rnoh::TaskExecutor> taskExecutor,
      ImageCacheLoadData data,
      ImageCacheLoadResult result) override;

  // -- Init + Start (split in v3.5) --
  void SetAnimatorOption(std::shared_ptr<ImageCacheAnimOption> option);
  void SetFirstFrame(std::shared_ptr<ImageCacheData> firstFrame);
  void Start();

  void PauseAnimation();
  void ResumeAnimation();
  void Cancel();
  void Restart();
  void Terminate();

  void SetIterations(uint32_t iterations) {
    iterations_.store(iterations, std::memory_order_release);
  }
  void SetReverse(bool reverse) {
    reverse_.store(reverse, std::memory_order_release);
  }
  uint32_t GetIterations() const {
    return iterations_.load(std::memory_order_acquire);
  }
  bool IsReverse() const {
    return reverse_.load(std::memory_order_acquire);
  }

 private:
  ImageCacheAnimState GetState() const {
    return state_.load(std::memory_order_acquire);
  }

  // -- Frame callbacks (InvokeCallback routes here) --
  void OnFramesDecoded(
      const std::vector<std::shared_ptr<ImageCacheData>>& frameList,
      uint32_t frameIndex);
  void OnFrameDecodeFailed();

  // -- Prefetch --
  void PrefetchFrames(uint32_t from);
  void ScheduleNextDisplay();
  void DisplayCurrentFrame();
  uint32_t GetFrameDelay(uint32_t frameIndex) const;

  // -- State helpers --
  uint32_t GetNextFrame();
  bool IsRunning() const {
    return state_.load(std::memory_order_acquire) ==
        ImageCacheAnimState::RUNNING;
  }
  int GetVersionId() const {
    return versionId_.load(std::memory_order_acquire);
  }
  uint32_t GetCurrentFrame() const {
    return currentFrame_.load(std::memory_order_acquire);
  }
  void PrefetchAndScheduleNext();
  void PrefetchAndScheduleCurrentFrame(uint32_t curFrame);
  void NotifyAnimationEnd(AnimationResult result);

  void DispatchFrameReady(ImageCacheLoadData data, ImageCacheLoadResult result);

  void CancelDisplayTask();
  void CancelPendingDecode();

  FramePrefetchBuffer prefetchBuffer_{DEFAULT_PREFETCH_AHEAD};
  static constexpr uint32_t kInvalidFrameIndex = UINT32_MAX;
  std::atomic<bool> isPendingDecodeFrame_{false};
  std::atomic<bool> isPendingDisplayFrame_{false};
  std::atomic<uint32_t> pendingDecodeFrameIndex_{kInvalidFrameIndex};

  std::weak_ptr<ImageCacheManagerInternal> manager_;
  const std::shared_ptr<FrameDecodeContext> frameContext_;

  mutable std::mutex displayTaskMutex_;
  std::atomic<ImageCacheAnimState> state_{ImageCacheAnimState::INITIAL};
  std::atomic<uint32_t> iterations_{0};
  std::atomic<bool> reverse_{false};
  std::atomic<uint32_t> currentFrame_{0};
  std::atomic<uint32_t> currentIteration_{0};
  std::atomic<int> versionId_{0};

  std::shared_ptr<ImageCacheAnimOption> animatorOption_;

  std::optional<ImageCacheTaskWorker::DelayedTaskHandle> displayTaskHandle_;
  ImageCacheGetCacheCompleteCallback onFrameReady_;
  AnimationEndCallback onAnimationEnd_;
  std::shared_ptr<ImageCacheData> firstFrame_;
  std::atomic<int> consecutiveFailures_{0};
  static constexpr int kMaxConsecutiveFailures = 3;
  std::shared_ptr<ImageCacheJob> cachedDecodeJob_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_ANIM_CONTROLLER_H
