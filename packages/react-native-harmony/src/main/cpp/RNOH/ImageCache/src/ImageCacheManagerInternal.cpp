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

#include "ImageCacheManagerInternal.h"
#include "ImageCacheAnimController.h"
#include "ImageCacheDownloadInterceptor.h"
#include "ImageCacheKeyGeneratorDefault.h"
#include "ImageCacheLoaderInternal.h"
#include "ImageCacheLog.h"
#include "ImageCacheMemoryCache.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheOption.h"
#include "ImageCacheRequest.h"
#include "ImageCacheUriUtil.h"

namespace ImageCachePro {

ImageCacheManager::Shared ImageCacheManager::Create(ImageCacheContext context) {
  return ImageCacheManagerInternal::Create(std::move(context));
}

ImageCacheManager::Shared ImageCacheManagerInternal::Create(
    ImageCacheContext context) {
  if (context.taskExecutor == nullptr) {
    LOG(ERROR) << "ImageCacheManager::Create failed: TaskExecutor is required";
    return nullptr;
  }
  return std::shared_ptr<ImageCacheManager>(
      new ImageCacheManagerInternal(std::move(context)));
}

ImageCacheManagerInternal::ImageCacheManagerInternal(ImageCacheContext context)
    : nativeResourceManager_(context.resourceManager),
      taskExecutor_(std::move(context.taskExecutor)) {
  ImageCacheMetrics::Instance().Objects().Increment(
      "ImageCacheManagerInternal", this);
}

ImageCacheManagerInternal::~ImageCacheManagerInternal() {
  // Ensure proper cleanup order
  Shutdown();
  defaultLoader_.reset();
  downloadProvider_.reset();
  dispatcher_.reset();
  ImageCacheMetrics::Instance().Objects().Decrement(
      "ImageCacheManagerInternal", this);
}

void ImageCacheManagerInternal::Shutdown() {
  if (!initialized_.exchange(false)) {
    return;
  }
  CloseAllRequests();
  if (dispatcher_) {
    dispatcher_->Shutdown();
    dispatcher_.reset();
  }
}

void ImageCacheManagerInternal::Init() {
  if (initialized_.load(std::memory_order_acquire)) {
    return;
  }
  std::lock_guard<std::mutex> lock(initMutex_);
  if (initialized_.load(std::memory_order_relaxed)) {
    return;
  }
  if (taskExecutor_ == nullptr) {
    LOG(ERROR) << "ImageCache Init failed: TaskExecutor is required";
    return;
  }
  dispatcher_ = std::make_shared<ImageCacheDispatcher>(shared_from_this());
  keyGenerator_ = std::make_unique<ImageCacheKeyGeneratorDefault>();
  defaultLoader_ = ImageCacheLoader::CreateDefaultLoader(*this);
  ImageCacheMemoryCache::GetInstance();
  initialized_.store(true, std::memory_order_release);
}

void ImageCacheManagerInternal::SetMaxRequests(int concurrency) {
  auto dispatcher = dispatcher_;
  if (dispatcher) {
    dispatcher->SetMaxRequests(concurrency);
  }
}

void ImageCacheManagerInternal::SetCacheKeyGenerator(
    std::unique_ptr<ImageCacheKeyGenerator> keyGenerator) {
  std::lock_guard<std::mutex> lock(configMutex_);
  keyGenerator_ = std::move(keyGenerator);
}

void ImageCacheManagerInternal::SetDownloadProvider(
    std::shared_ptr<ImageCacheDownloader> loader) {
  if (loader == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(configMutex_);
  downloadProvider_ = std::move(loader);
}

bool ImageCacheManagerInternal::HasDownloadProvider() const {
  std::lock_guard<std::mutex> lock(configMutex_);
  return downloadProvider_ != nullptr;
}

std::shared_ptr<ImageCacheDownloader>
ImageCacheManagerInternal::GetDownloadProvider() const {
  std::lock_guard<std::mutex> lock(configMutex_);
  return downloadProvider_;
}

bool ImageCacheManagerInternal::IsRemoteUri(const std::string& uri) const {
  return ImageCacheUriUtil::IsRemoteUri(uri);
}

const NativeResourceManager*
ImageCacheManagerInternal::GetNativeResourceManager() const {
  return nativeResourceManager_;
}

std::shared_ptr<rnoh::TaskExecutor> ImageCacheManagerInternal::GetTaskExecutor()
    const {
  return taskExecutor_;
}

ImageCacheDispatcher* ImageCacheManagerInternal::GetDispatcher() {
  if (dispatcher_ == nullptr) {
    return nullptr;
  }
  return dispatcher_.get();
}

std::shared_ptr<ImageCacheDispatcher>
ImageCacheManagerInternal::GetDispatcherShared() {
  return dispatcher_;
}

void ImageCacheManagerInternal::SetCacheLimit(size_t maxSize, size_t maxUsage) {
  ImageCacheMemoryCache::GetInstance()->SetCacheLimit(maxSize, maxUsage);
}

int64_t ImageCacheManagerInternal::GetCacheLimitNum() {
  return static_cast<int64_t>(
      ImageCacheMemoryCache::GetInstance()->GetAllowMaxSizeLimit());
}

int64_t ImageCacheManagerInternal::GetCacheLimitSize() {
  return static_cast<int64_t>(
      ImageCacheMemoryCache::GetInstance()->GetAllowMaxMemory());
}

int64_t ImageCacheManagerInternal::GetCurrentCacheNum() {
  return static_cast<int64_t>(ImageCacheMemoryCache::GetInstance()->Size());
}

int64_t ImageCacheManagerInternal::GetCurrentCacheSize() {
  return static_cast<int64_t>(
      ImageCacheMemoryCache::GetInstance()->GetCurrentMemory());
}

std::shared_ptr<ImageCacheLoader> ImageCacheManagerInternal::GetDefaultLoader()
    const {
  return defaultLoader_;
}

ImageCacheKeyGenerator* ImageCacheManagerInternal::GetCacheKeyGenerator()
    const {
  return keyGenerator_.get();
}

std::shared_ptr<ImageCacheData> ImageCacheManagerInternal::GetCachedImageSync(
    const std::shared_ptr<ImageCacheOption>& option) {
  IMAGE_CACHE_CHECK_RETURN(option == nullptr, nullptr);
  if (!IsInitialized()) {
    return nullptr;
  }
  if (dispatcher_ == nullptr) {
    return nullptr;
  }
  auto data = dispatcher_->GetCachedImageSync(option);
  if (data != nullptr) {
    ImageCacheMetrics::Instance().Increment(MetricsCounter::CacheHits);
  } else {
    ImageCacheMetrics::Instance().Increment(MetricsCounter::CacheMisses);
  }
  return data;
}

std::shared_ptr<ImageCacheRequest> ImageCacheManagerInternal::GetRequest(
    ImageCacheRequestHandle handle) {
  std::lock_guard<std::mutex> lock(requestMutex_);
  auto it = requestMap_.find(handle);
  if (it == requestMap_.end()) {
    return nullptr;
  }
  return it->second;
}

ImageCacheRequestHandle ImageCacheManagerInternal::GetCacheImage(
    const std::shared_ptr<ImageCacheOption>& option,
    ImageCacheGetCacheCompleteCallback onComplete) {
  auto dispatcher = dispatcher_;
  if (!IsInitialized() || option == nullptr || dispatcher == nullptr ||
      !option->IsValid()) {
    LOG(ERROR) << "ImageCache GetCacheImage failed: invalid parameter";
    return INVALID_REQUEST_HANDLE;
  }

  option->PrepareTransformations();
  auto requestKey = dispatcher->GenerateRequestKey(*option);

  // 1. Memory cache hit callback synchronously, no cancellable request
  auto data = dispatcher->GetCachedImageSync(option);
  if (data != nullptr) {
    if (onComplete) {
      onComplete(ImageCacheLoadData::From({data}), ImageCacheLoadResult::Ok());
    }
    return INVALID_REQUEST_HANDLE;
  }

  auto handle = ImageCacheRequest::AllocateHandle();

  // 2. Cache miss register request, submit to Dispatcher
  auto req =
      std::make_shared<ImageCacheNormalRequest>(handle, option, onComplete);
  if (req == nullptr) {
    LOG(ERROR) << "ImageCache GetCacheImage failed: create request fail";
    return INVALID_REQUEST_HANDLE;
  }
  {
    std::lock_guard<std::mutex> lock(requestMutex_);
    requestMap_[handle] = req;
  }
  // 3. Submit request to Dispatcher
  dispatcher->Enqueue(
      req,
      // mergeFunc: look up jobMap and return an existing Job to merge into
      [dispatcher](
          std::shared_ptr<ImageCacheRequest> r,
          const std::string& requestKey) -> std::shared_ptr<ImageCacheJob> {
        return dispatcher->GetJob(requestKey);
      },
      // createJobFunc: create a regular Job
      [dispatcher](std::shared_ptr<ImageCacheRequest> r)
          -> std::shared_ptr<ImageCacheJob> {
        auto requestKey = dispatcher->GenerateRequestKey(*r->GetOption());
        return ImageCacheJob::CreateJob(
            requestKey, r->GetOption(), dispatcher, r);
      });

  return handle;
}

void ImageCacheManagerInternal::CancelRequest(ImageCacheRequestHandle handle) {
  std::shared_ptr<ImageCacheRequest> req;
  std::shared_ptr<ImageCacheDispatcher> dispatcher;
  {
    std::lock_guard<std::mutex> lock(requestMutex_);
    auto it = requestMap_.find(handle);
    if (it == requestMap_.end()) {
      return;
    }
    req = it->second;
    requestMap_.erase(it);
    dispatcher = dispatcher_;
  }

  if (req == nullptr) {
    return;
  }

  if (dispatcher == nullptr) {
    return;
  }
  dispatcher->Dequeue(req);
}

ImageCacheRequestHandle ImageCacheManagerInternal::Prefetch(
    const std::shared_ptr<ImageCacheOption>& option) {
  return GetCacheImage(option, nullptr);
}

std::shared_ptr<ImageCacheData> ImageCacheManagerInternal::LoadImageSync(
    const std::shared_ptr<ImageCacheOption>& option) {
  if (option == nullptr) {
    return nullptr;
  }
  option->PrepareTransformations();
  auto dispatcher = dispatcher_;
  if (dispatcher == nullptr) {
    return nullptr;
  }
  return dispatcher->LoadImageSync(option);
}

void ImageCacheManagerInternal::RemoveMemoryCache(
    const std::shared_ptr<ImageCacheOption>& option) {
  if (option == nullptr) {
    return;
  }
  if (!IsInitialized()) {
    return;
  }
  option->PrepareTransformations();
  auto dispatcher = dispatcher_;
  if (dispatcher == nullptr) {
    return;
  }
  std::string requestKey = dispatcher->GenerateRequestKey(*option);
  std::string animatedKey = dispatcher->GenerateMemoryKey(*option, true);
  ImageCacheMemoryCache::GetInstance()->Remove(requestKey);
  ImageCacheMemoryCache::GetInstance()->Remove(animatedKey);
}

ImageCacheAnimHandle ImageCacheManagerInternal::PlayAnimation(
    const ImageCacheLoadData& loadData,
    const std::shared_ptr<ImageCacheOption>& option,
    std::shared_ptr<ImageCacheAnimOption> animOption,
    ImageCacheGetCacheCompleteCallback onFrameReady,
    AnimationEndCallback onAnimationEnd) {
  if (loadData.frameContext == nullptr || option == nullptr ||
      !option->IsValid() || !IsInitialized() || onFrameReady == nullptr) {
    return INVALID_ANIM_HANDLE;
  }
  option->PrepareTransformations();

  auto handle = ImageCacheRequest::AllocateHandle();

  auto controller = std::make_shared<ImageCacheAnimController>(
      shared_from_this(),
      handle,
      option,
      loadData.frameContext,
      onFrameReady,
      onAnimationEnd);
  if (controller == nullptr) {
    return INVALID_ANIM_HANDLE;
  }

  {
    std::lock_guard<std::mutex> lock(requestMutex_);
    requestMap_[handle] = controller;
    animationMap_[handle] = controller;
  }
  if (animOption) {
    controller->SetAnimatorOption(animOption);
  }
  if (!loadData.imageDataList.empty()) {
    controller->SetFirstFrame(loadData.imageDataList.front());
  }
  controller->Start();

  return handle;
}

void ImageCacheManagerInternal::ControlAnimation(
    ImageCacheAnimHandle handle,
    AnimationAction action) {
  std::shared_ptr<ImageCacheAnimController> controller;
  {
    std::lock_guard<std::mutex> lock(requestMutex_);
    auto it = animationMap_.find(handle);
    if (it == animationMap_.end()) {
      return;
    }
    // Only ANIMATION type handles are valid for ControlAnimation
    if (it->second->GetRequestType() != ImageCacheHandleType::ANIMATION) {
      return;
    }
    controller = it->second;
  }
  if (controller == nullptr) {
    return;
  }
  switch (action) {
    case AnimationAction::Start:
      controller->Start();
      break;
    case AnimationAction::Pause:
      controller->PauseAnimation();
      break;
    case AnimationAction::Resume:
      controller->ResumeAnimation();
      break;
    case AnimationAction::Cancel:
      controller->Cancel();
      break;
    case AnimationAction::Restart:
      controller->Restart();
      break;
  }
}

void ImageCacheManagerInternal::CloseAllRequests() {
  std::vector<std::shared_ptr<ImageCacheAnimController>> controllers;
  std::vector<std::shared_ptr<ImageCacheRequest>> requests;
  std::shared_ptr<ImageCacheDispatcher> dispatcher;
  {
    std::lock_guard<std::mutex> lock(requestMutex_);
    dispatcher = dispatcher_;
    for (auto it = requestMap_.begin(); it != requestMap_.end();) {
      requests.push_back(it->second);
      it = requestMap_.erase(it);
    }

    for (auto it = animationMap_.begin(); it != animationMap_.end();) {
      controllers.push_back(it->second);
      it = animationMap_.erase(it);
    }
  }
  for (auto& ctrl : controllers) {
    if (ctrl != nullptr) {
      ctrl->Cancel();
    }
  }
  if (dispatcher == nullptr) {
    return;
  }
  for (auto& req : requests) {
    if (req != nullptr) {
      dispatcher->Dequeue(req);
    }
  }
}
} // namespace ImageCachePro
