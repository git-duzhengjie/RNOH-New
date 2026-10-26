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

#ifndef IMAGE_CACHE_DOWNLOAD_INTERCEPTOR_H
#define IMAGE_CACHE_DOWNLOAD_INTERCEPTOR_H

#include <atomic>
#include <memory>
#include "ImageCacheDownloader.h"
#include "ImageCacheInterceptor.h"
#include "ImageCacheLog.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheTaskInternal.h"
#include "ImageCacheUriUtil.h"

namespace ImageCachePro {

class ImageCacheManagerInternal;

class ImageCacheDownloadInterceptor : public ImageCacheLoadInterceptor {
 public:
  explicit ImageCacheDownloadInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheLoadInterceptor(manager) {
    name = "DownloadInterceptor";
  }

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override;
  void Cancel(const std::shared_ptr<ImageCacheTask>& task) override;

 private:
  std::shared_ptr<ImageCacheDownloader> GetLoader() const {
    return GetManager().GetDownloadProvider();
  }

  static bool IsLocalUrl(const std::string& url) {
    return ImageCacheUriUtil::IsLocalUri(url);
  }
};

inline bool ImageCacheDownloadInterceptor::Resolve(
    const std::shared_ptr<ImageCacheTask>& task) {
  auto loader = GetLoader();
  IMAGE_CACHE_CHECK_RETURN(loader == nullptr, false);
  std::string url;
  if (!task->GetImageSource().GetString(url)) {
    return false;
  }

  if (url.empty()) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::EMPTY_URL);
    return false;
  }
  if (IsLocalUrl(url)) {
    return false;
  }

  auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  IMAGE_CACHE_CHECK_RETURN(taskInternal == nullptr, false);

  auto ctx = loader->AllocateCallbackContext(
      taskInternal,
      std::dynamic_pointer_cast<ImageCacheDownloadInterceptor>(
          shared_from_this()));
  if (!ctx) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::ALLOCATE_REQUEST_ID_FAILED);
    return false;
  }

  taskInternal->SetDownloadRequestId(ctx->requestId);
  auto needSuspend = std::make_shared<std::atomic<bool>>(true);
  loader->Download(
      url,
      ctx,
      [loader, needSuspend](
          uint32_t requestId, const ImageCacheDownloadResult& result) {
        needSuspend->store(false, std::memory_order_release);
        auto ctx = loader->Get(requestId, true);
        if (!ctx) {
          DLOG(ERROR) << "Can not get ctx request " << requestId;
          return;
        }
        auto& taskPtr = ctx->task;
        if (!taskPtr) {
          DLOG(ERROR) << "Can not get task request " << requestId;
          return;
        }
        auto& interceptor = ctx->interceptor;
        if (!interceptor) {
          TASK_SET_ERROR(taskPtr, ImageCacheErrorCode::DOWNLOAD_DESTROYED);
          return;
        }
        if (result.success) {
          taskPtr->product.imageBuffer = result.buffer;
          taskPtr->product.imageLength = result.length;
          interceptor->OnComplete(taskPtr, true);
        } else {
          TASK_SET_ERROR(taskPtr, ImageCacheErrorCode::DOWNLOAD_FAILED);
          interceptor->OnComplete(taskPtr, false);
        }
      });
  if (needSuspend->load(std::memory_order_acquire)) {
    taskInternal->Suspend();
  }
  return true;
}

inline void ImageCacheDownloadInterceptor::Cancel(
    const std::shared_ptr<ImageCacheTask>& task) {
  auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  IMAGE_CACHE_CHECK(taskInternal == nullptr);
  uint32_t requestId = taskInternal->GetDownloadRequestId();
  if (requestId != 0) {
    auto loader = GetLoader();
    if (loader != nullptr) {
      loader->Cancel(requestId);
    }
  }
}

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWNLOAD_INTERCEPTOR_H
