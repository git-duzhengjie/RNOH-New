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

#ifndef IMAGE_CACHE_TASK_INTERNAL_H
#define IMAGE_CACHE_TASK_INTERNAL_H

#include <multimedia/image_framework/image/image_source_native.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include "ImageCacheConfig.h"
#include "ImageCacheIntrList.h"
#include "ImageCacheJobQueue.h"
#include "ImageCacheKeyGenerator.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheRequest.h"
#include "ImageCacheTask.h"
#include "utils/ImageCacheLog.h"

namespace ImageCachePro {

class ImageCacheLoaderInternal;

// Manages OH_ImageSourceNative lifetime.
// Non-copyable; users hold via std::shared_ptr<ImageSourceNativeWrapper>.
// Calls OH_ImageSourceNative_Release on destruction.
class ImageSourceNativeWrapper {
 public:
  // Factory method: creates wrapper only if raw pointer is valid
  static std::shared_ptr<ImageSourceNativeWrapper> Create(
      OH_ImageSourceNative* raw) {
    if (raw == nullptr) {
      DLOG(WARNING)
          << "ImageSourceNativeWrapper::Create: raw pointer is nullptr";
      return nullptr;
    }
    return std::shared_ptr<ImageSourceNativeWrapper>(
        new ImageSourceNativeWrapper(raw));
  }

  // Direct constructor - caller must ensure raw is valid
  explicit ImageSourceNativeWrapper(OH_ImageSourceNative* raw)
      : imageSourceNative_(raw) {
    ImageCacheMetrics::Instance().Objects().Increment(
        "ImageSourceNativeWrapper", this);
  }

  ~ImageSourceNativeWrapper() {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "ImageSourceNativeWrapper", this);
    if (imageSourceNative_) {
      OH_ImageSourceNative_Release(imageSourceNative_);
      imageSourceNative_ = nullptr;
    }
  }

  ImageSourceNativeWrapper(const ImageSourceNativeWrapper&) = delete;
  ImageSourceNativeWrapper& operator=(const ImageSourceNativeWrapper&) = delete;
  ImageSourceNativeWrapper(ImageSourceNativeWrapper&&) = delete;
  ImageSourceNativeWrapper& operator=(ImageSourceNativeWrapper&&) = delete;

  OH_ImageSourceNative* get() const {
    return imageSourceNative_;
  }
  bool isValid() const {
    return imageSourceNative_ != nullptr;
  }

 private:
  OH_ImageSourceNative* imageSourceNative_ = nullptr;
};

class ImageCacheDispatcher;

class ImageCacheTaskInternal
    : public ImageCacheTask,
      public std::enable_shared_from_this<ImageCacheTaskInternal> {
  friend class ImageCacheDispatcher;
  friend class ImageCacheLoaderInternal;

 public:
  ImageCacheTaskInternal(
      std::weak_ptr<ImageCacheJob> job,
      std::weak_ptr<ImageCacheDispatcher> dispatcher = {})
      : job_(std::move(job)), dispatcher_(std::move(dispatcher)) {
    ImageCacheMetrics::Instance().Objects().Increment(
        "ImageCacheTaskInternal", this);
  }
  ~ImageCacheTaskInternal() override {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "ImageCacheTaskInternal", this);
  }

  enum class DecodeMode : uint8_t {
    NORMAL = 0,
    BATCH_TO_FRAME,
    LAZY_DECODE,
    FRAME,
  };

  // -- Job access --
  std::shared_ptr<ImageCacheJob> GetJob() const {
    return job_.lock();
  }
  std::string GetJobKey() const {
    auto j = job_.lock();
    return j ? j->requestKey : "unknown";
  }

  bool TryMarkCompletionHandled() {
    bool expected = false;
    return completionHandled_.compare_exchange_strong(
        expected, true, std::memory_order_acq_rel);
  }

  void ResetCompletionHandled() {
    completionHandled_.store(false, std::memory_order_release);
  }

  // Reset terminal state so a completed task can be re-enqueued (animation
  // prefetch).
  void ResetForReuse() {
    completionHandled_.store(false, std::memory_order_release);
    errorCode_.store(ImageCacheErrorCode::OK, std::memory_order_release);
    state.store(ImageCacheTaskState::READY, std::memory_order_release);
  }

  // -- Image source native --
  OH_ImageSourceNative* GetImageSourceNative() const {
    return imageSourceNative_ ? imageSourceNative_->get() : nullptr;
  }
  const std::shared_ptr<ImageSourceNativeWrapper>& GetImageSourceNativeWrapper()
      const {
    return imageSourceNative_;
  }
  void SetImageSourceNative(OH_ImageSourceNative* raw) {
    imageSourceNative_ = ImageSourceNativeWrapper::Create(raw);
  }
  void SetImageSourceNativeWrapper(
      std::shared_ptr<ImageSourceNativeWrapper> wrapper) {
    imageSourceNative_ = std::move(wrapper);
  }

  bool ShouldAbort();
  bool TransitionTo(ImageCacheTaskState expected, ImageCacheTaskState desired);
  void ForceTransitionTo(ImageCacheTaskState desired);

  std::shared_ptr<ImageCacheDispatcher> GetDispatcher() const {
    return dispatcher_.lock();
  }
  bool IsAnimation() const {
    return isAnimation;
  }
  void SetDecodeMode(DecodeMode mode) {
    decodeMode_ = mode;
  }
  void SetDesiredImageSize(Image_Size size) {
    desiredImageSize_ = size;
  }
  DecodeMode GetDecodeMode() const {
    return decodeMode_;
  }
  Image_Size GetDesiredImageSize() const {
    return desiredImageSize_;
  }
  bool IsFrameDecodeMode() const {
    return decodeMode_ == DecodeMode::FRAME ||
        decodeMode_ == DecodeMode::BATCH_TO_FRAME;
  }
  ImageCachePriority GetPriority() const {
    return priority;
  }
  void SetPriority(ImageCachePriority p) {
    priority = p;
  }
  uint32_t GetDownloadRequestId() const {
    return downloadRequestId.load(std::memory_order_acquire);
  }
  void SetDownloadRequestId(uint32_t id) {
    downloadRequestId.store(id, std::memory_order_release);
  }

  // -- Task lifecycle --
  void Suspend();
  void Resume(bool result);
  bool IsSuspended() const;
  bool IsSuccess() const;
  bool IsSuspendTimedOut() const;
  void SetError(ImageCacheErrorCode code) override;
  ImageCacheErrorCode GetError() const override;

  const ImageCacheSource& GetImageSource() const override;
  std::shared_ptr<ImageCacheOption> GetOption() const;
  const ImageCacheFileTypeInfo& GetFileTypeInfo() const override {
    return fileTypeInfo_;
  }
  void SetFileTypeInfo(const ImageCacheFileTypeInfo& info) {
    fileTypeInfo_ = info;
  }
  std::string GetTaskInfo() const;

  bool Execute(std::shared_ptr<ImageCacheLoaderInternal> loader);

 private:
  void MarkSuccess();
  void QueryImageBufferFormat();
  void CheckDecodeMode();
  void CalculateDesiredImageSize();
  void UpdateFileTypeInfo();
  void FinishDecode();
  // -- Load / decode / transform (called by Dispatcher & Interceptor) --
  bool TryReadFromMemoryCache(std::shared_ptr<ImageCacheLoaderInternal> loader);
  void TryWriteToMemoryCache(std::shared_ptr<ImageCacheLoaderInternal> loader);
  void LoadImage(std::shared_ptr<ImageCacheLoaderInternal> loader);
  void DecodeFrame(std::shared_ptr<ImageCacheLoaderInternal> loader);
  void DecodeImage(std::shared_ptr<ImageCacheLoaderInternal> loader);
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  bool TransformImage();
#endif

  std::weak_ptr<ImageCacheJob> job_;
  std::weak_ptr<ImageCacheDispatcher> dispatcher_;
  std::atomic<ImageCacheTaskState> state{ImageCacheTaskState::READY};
  ImageCacheIntrListNode listNode;

  ImageCachePriority priority{ImageCachePriority::MEDIUM};
  DecodeMode decodeMode_{DecodeMode::NORMAL};
  Image_Size desiredImageSize_{0, 0};
  bool writeMemoryCache{true};
  bool isAnimation{false};

  std::atomic<uint32_t> downloadRequestId{0};
  std::shared_ptr<ImageSourceNativeWrapper> imageSourceNative_;

  std::chrono::steady_clock::time_point suspendStartTime;
  std::atomic<ImageCacheErrorCode> errorCode_{ImageCacheErrorCode::OK};
  std::atomic<bool> completionHandled_{false};
  ImageCacheFileTypeInfo fileTypeInfo_{};
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_TASK_INTERNAL_H
