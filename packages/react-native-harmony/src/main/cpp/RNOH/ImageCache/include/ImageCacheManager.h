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

#ifndef IMAGE_CACHE_MANAGER_H
#define IMAGE_CACHE_MANAGER_H

#include <memory>
#include <string>
#include "ImageCacheAnimCallback.h"
#include "ImageCacheData.h"
#include "ImageCacheDownloader.h"
#include "ImageCacheOption.h"
#include "ImageCacheType.h"

namespace rnoh {
class TaskExecutor;
}

namespace ImageCachePro {

class ImageCacheDataCacheInternal;
struct FrameDecodeContext;

/** Animation lifecycle actions for ControlAnimation. */
enum class AnimationAction {
  Start,   ///< Start or restart a paused animation.
  Pause,   ///< Pause a running animation (e.g. node scrolled off-screen).
  Resume,  ///< Resume a previously paused animation.
  Cancel,  ///< Cancel and release animation resources. No-op for invalid handles.
  Restart, ///< Reset to first frame and start playing from the beginning.
};

/**
 * @brief RNOH runtime context required for ImageCache creation.
 * taskExecutor is mandatory; ImageCacheManager::Create returns nullptr when absent.
 */
struct ImageCacheContext {
  const NativeResourceManager* resourceManager = nullptr;
  std::shared_ptr<rnoh::TaskExecutor> taskExecutor;

  ImageCacheContext(
      const NativeResourceManager* resourceManager,
      std::shared_ptr<rnoh::TaskExecutor> taskExecutor)
      : resourceManager(resourceManager),
        taskExecutor(std::move(taskExecutor)) {}
};

/**
 * @brief Each RNInstance has its own ImageCacheManager; MemoryCache is a global singleton shared across instances.
 */
class ImageCacheManager {
 public:
  using Shared = std::shared_ptr<ImageCacheManager>;

  static Shared Create(ImageCacheContext context);

  virtual ~ImageCacheManager() = default;

  virtual void Init() = 0;

  virtual void Shutdown() = 0;

  virtual void SetMaxRequests(int concurrency) = 0;

  virtual void SetDownloadProvider(std::shared_ptr<ImageCacheDownloader> loader) = 0;

  virtual bool HasDownloadProvider() const = 0;

  virtual void SetCacheLimit(size_t maxSize, size_t maxUsage) = 0;

  virtual int64_t GetCacheLimitNum() = 0;
  virtual int64_t GetCacheLimitSize() = 0;
  virtual int64_t GetCurrentCacheNum() = 0;
  virtual int64_t GetCurrentCacheSize() = 0;

  /**
   * @brief Submit an asynchronous image load request.
   * Returns an opaque handle that can be passed to CancelRequest to abort.
   * When option->callbackOnMainThread is true, the onComplete callback is
   * guaranteed to run on the MAIN thread.
   */
  virtual ImageCacheRequestHandle GetCacheImage(
      const std::shared_ptr<ImageCacheOption>& option,
      ImageCacheGetCacheCompleteCallback onComplete) = 0;

  /**
   * @brief Prefetch: pre-load image into cache. Equivalent to
   * GetCacheImage(option, nullptr).
   */
  virtual ImageCacheRequestHandle Prefetch(
      const std::shared_ptr<ImageCacheOption>& option) = 0;

  /**
   * @brief Cancel an in-flight request by its handle.
   * Safe to call with INVALID_REQUEST_HANDLE (no-op).
   */
  /**
   * @brief Cancel an in-flight request by its handle.
   * Safe to call with INVALID_REQUEST_HANDLE (no-op).
   */
  virtual void CancelRequest(ImageCacheRequestHandle handle) = 0;

  /**
   * @brief Synchronously look up an already-cached image.
   * Returns nullptr when the image is not in the memory cache.
   */
  virtual std::shared_ptr<ImageCacheData> GetCachedImageSync(
      const std::shared_ptr<ImageCacheOption>& option) = 0;

  virtual void RemoveMemoryCache(const std::shared_ptr<ImageCacheOption>& option) = 0;

  /** Determines whether the URI is a remote HTTP(S) address. */
  virtual bool IsRemoteUri(const std::string& uri) const = 0;

  /**
   * @brief Start a frame-by-frame animation.
   * @param dataCache  Decoded frame data source.
   * @param option     Image load options (size, priority, etc.).
   * @param onFrameReady  Called on each decoded frame with pixelmap data.
   * @param onAnimationEnd  Called when animation finishes or errors.
   * @return Animation handle for ControlAnimation, or INVALID_REQUEST_HANDLE on failure.
   */
  virtual ImageCacheAnimHandle PlayAnimation(
      const ImageCacheLoadData& loadData,
      const std::shared_ptr<ImageCacheOption>& option,
      std::shared_ptr<ImageCacheAnimOption> animOption,
      ImageCacheGetCacheCompleteCallback onFrameReady,
      AnimationEndCallback onAnimationEnd = nullptr) = 0;

  /**
   * @brief Control an animation's lifecycle.
   * Safe to call with INVALID_ANIM_HANDLE (no-op for all actions).
   * @param handle  Animation handle returned by PlayAnimation.
   * @param action  Lifecycle action (Start / Pause / Resume / Stop).
   */
  virtual void ControlAnimation(
      ImageCacheAnimHandle handle,
      AnimationAction action) = 0;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_MANAGER_H
