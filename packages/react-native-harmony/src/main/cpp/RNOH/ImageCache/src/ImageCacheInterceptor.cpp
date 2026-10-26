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

#include "ImageCacheInterceptor.h"
#include "ImageCacheDispatcher.h"
#include "ImageCacheLoaderInternal.h"
#include "ImageCacheLog.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

static bool IsLoadInterceptor(ImageCacheInterceptor* interceptor) {
  return dynamic_cast<ImageCacheLoadInterceptor*>(interceptor) != nullptr;
}

bool ImageCacheInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& task,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)>
        resolveCallback) {
  auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  IMAGE_CACHE_CHECK_RETURN(taskInternal == nullptr, false);
  if (taskInternal->GetError() != ImageCacheErrorCode::OK) {
    return false;
  }

  auto nameCopy = name;
  auto func =
      [self = shared_from_this()](const std::shared_ptr<ImageCacheTask>& t) {
        return self->Resolve(t);
      };
  auto resolveFunction = resolveCallback == nullptr ? func : resolveCallback;
  bool result = resolveFunction(task);
  if (taskInternal->IsSuspended() && IsLoadInterceptor(this)) {
    return true;
  }
  if (taskInternal->ShouldAbort()) {
    return false;
  }
  if (result) {
    return true;
  }
  if (next_ != nullptr) {
    return next_->Process(task);
  }
  return false;
}

void ImageCacheInterceptor::Cancel(
    const std::shared_ptr<ImageCacheTask>& task) {}

void ImageCacheMemoryInterceptor::SetNext(
    const std::shared_ptr<ImageCacheMemoryInterceptor>& interceptor) {
  next_ = interceptor;
}

bool ImageCacheMemoryInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& task,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)>
        resolveCallback) {
  return ImageCacheInterceptor::Process(task, resolveCallback);
}

void ImageCacheLoadInterceptor::OnComplete(
    const std::shared_ptr<ImageCacheTask>& task,
    bool result) {
  auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  IMAGE_CACHE_CHECK(taskInternal == nullptr);
  if (!taskInternal->IsSuccess()) {
    if (taskInternal->IsSuspended()) {
      taskInternal->Resume(false);
    }
    return;
  }

  // If current interceptor failed, try next interceptor in chain
  if (!result && next_ != nullptr) {
    result = next_->Process(taskInternal);
    if (taskInternal->IsSuspended()) { // next interceptor is async
      return;
    }
  }

  if (taskInternal->IsSuspended()) { // next interceptor is complete
    taskInternal->Resume(result);
  }
}

void ImageCacheLoadInterceptor::Cancel(
    const std::shared_ptr<ImageCacheTask>& task) {
  if (next_ != nullptr) {
    next_->Cancel(task);
  }
}

bool ImageCacheLoadInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& task,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)>
        resolveCallback) {
  return ImageCacheInterceptor::Process(task, resolveCallback);
}

void ImageCacheLoadInterceptor::SetNext(
    const std::shared_ptr<ImageCacheLoadInterceptor>& interceptor) {
  next_ = interceptor;
}

void ImageCacheDecodeInterceptor::SetNext(
    const std::shared_ptr<ImageCacheDecodeInterceptor>& interceptor) {
  next_ = interceptor;
}

bool ImageCacheDecodeInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& task,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)>
        resolveCallback) {
  return ImageCacheInterceptor::Process(task, resolveCallback);
}

} // namespace ImageCachePro
