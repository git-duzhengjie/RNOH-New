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

#ifndef IMAGE_CACHE_MANAGER_INTERNAL_H
#define IMAGE_CACHE_MANAGER_INTERNAL_H

#include <resourcemanager/ohresmgr.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <variant>
#include "Animation/ImageCacheAnimController.h"
#include "ImageCacheDispatcher.h"
#include "ImageCacheKeyGenerator.h"
#include "ImageCacheLoader.h"
#include "ImageCacheManager.h"
#include "ImageCacheRequest.h"

namespace rnoh {
class TaskExecutor;
}

namespace ImageCachePro {

class ImageCacheAnimController;

class ImageCacheManagerInternal
    : public ImageCacheManager,
      public std::enable_shared_from_this<ImageCacheManagerInternal> {
 public:
  ~ImageCacheManagerInternal() override;

  static ImageCacheManager::Shared Create(ImageCacheContext context);

  void Init() override;
  void Shutdown() override;
  void SetMaxRequests(int concurrency) override;
  void SetCacheKeyGenerator(
      std::unique_ptr<ImageCacheKeyGenerator> keyGenerator);
  void SetDownloadProvider(
      std::shared_ptr<ImageCacheDownloader> loader) override;
  bool HasDownloadProvider() const override;
  bool IsRemoteUri(const std::string& uri) const override;
  void SetCacheLimit(size_t maxSize, size_t maxUsage) override;
  int64_t GetCacheLimitNum() override;
  int64_t GetCacheLimitSize() override;
  int64_t GetCurrentCacheNum() override;
  int64_t GetCurrentCacheSize() override;

  ImageCacheRequestHandle GetCacheImage(
      const std::shared_ptr<ImageCacheOption>& option,
      ImageCacheGetCacheCompleteCallback onComplete) override;
  ImageCacheRequestHandle Prefetch(
      const std::shared_ptr<ImageCacheOption>& option) override;
  void CancelRequest(ImageCacheRequestHandle handle) override;
  std::shared_ptr<ImageCacheData> GetCachedImageSync(
      const std::shared_ptr<ImageCacheOption>& option) override;
  std::shared_ptr<ImageCacheData> LoadImageSync(
      const std::shared_ptr<ImageCacheOption>& option);

  void RemoveMemoryCache(const std::string& url);
  void RemoveMemoryCache(const std::shared_ptr<ImageCacheOption>& option);

  ImageCacheKeyGenerator* GetCacheKeyGenerator() const;
  std::shared_ptr<ImageCacheLoader> GetDefaultLoader() const;
  std::shared_ptr<ImageCacheDownloader> GetDownloadProvider() const;
  ImageCacheDispatcher* GetDispatcher();
  std::shared_ptr<ImageCacheDispatcher> GetDispatcherShared();

  const NativeResourceManager* GetNativeResourceManager() const;
  std::shared_ptr<rnoh::TaskExecutor> GetTaskExecutor() const;

  // -- Handle lookup (unified entry) --
  std::shared_ptr<ImageCacheRequest> GetRequest(ImageCacheRequestHandle handle);
  void AddRequest(
      ImageCacheRequestHandle handle,
      std::shared_ptr<ImageCacheRequest> request) {
    std::lock_guard<std::mutex> lock(requestMutex_);
    requestMap_[handle] = request;
  }
  bool RemoveRequest(ImageCacheRequestHandle handle) {
    std::lock_guard<std::mutex> lock(requestMutex_);
    return requestMap_.erase(handle) > 0;
  }

  // -- Animation control --
  ImageCacheAnimHandle PlayAnimation(
      const ImageCacheLoadData& loadData,
      const std::shared_ptr<ImageCacheOption>& option,
      std::shared_ptr<ImageCacheAnimOption> animOption,
      ImageCacheGetCacheCompleteCallback onFrameReady,
      AnimationEndCallback onAnimationEnd = nullptr) override;
  void ControlAnimation(ImageCacheAnimHandle handle, AnimationAction action)
      override;
  void RemoveAnimation(ImageCacheAnimHandle handle) {
    std::lock_guard<std::mutex> lock(requestMutex_);
    animationMap_.erase(handle);
  }

 private:
  friend class ImageCacheDispatcher;

  void CloseAllRequests();
  explicit ImageCacheManagerInternal(ImageCacheContext context);
  bool IsInitialized() const {
    return initialized_.load(std::memory_order_acquire);
  }

  std::shared_ptr<ImageCacheDispatcher> dispatcher_;
  std::shared_ptr<ImageCacheLoader> defaultLoader_ = nullptr;
  std::unique_ptr<ImageCacheKeyGenerator> keyGenerator_;
  std::shared_ptr<ImageCacheDownloader> downloadProvider_ = nullptr;
  const NativeResourceManager* nativeResourceManager_ = nullptr;
  std::shared_ptr<rnoh::TaskExecutor> taskExecutor_;
  std::mutex initMutex_;
  mutable std::mutex configMutex_;
  std::atomic<bool> initialized_{false};

  std::mutex requestMutex_;
  std::
      unordered_map<ImageCacheRequestHandle, std::shared_ptr<ImageCacheRequest>>
          requestMap_;
  std::unordered_map<
      ImageCacheAnimHandle,
      std::shared_ptr<ImageCacheAnimController>>
      animationMap_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_MANAGER_INTERNAL_H
