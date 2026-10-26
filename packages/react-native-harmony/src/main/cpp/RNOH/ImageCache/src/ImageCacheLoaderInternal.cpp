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

#include "ImageCacheLoaderInternal.h"
#include "ImageCacheLog.h"

namespace ImageCachePro {

void ImageCacheLoaderInternal::AddMemoryCacheInterceptor(
    std::shared_ptr<ImageCacheMemoryInterceptor> interceptor,
    ImageCacheLoader::Position position) {
  if (interceptor == nullptr) {
    return;
  }
  if (memoryHead_ == nullptr) {
    memoryHead_ = memoryTail_ = interceptor;
    return;
  }
  if (position == Position::START) {
    interceptor->SetNext(memoryHead_);
    memoryHead_ = interceptor;
  } else {
    memoryTail_->SetNext(interceptor);
    memoryTail_ = interceptor;
  }
}

void ImageCacheLoaderInternal::AddLoadInterceptor(
    std::shared_ptr<ImageCacheLoadInterceptor> interceptor,
    ImageCacheLoader::Position position) {
  if (interceptor == nullptr) {
    return;
  }
  if (loadHead_ == nullptr) {
    loadHead_ = loadTail_ = interceptor;
    return;
  }
  if (position == Position::START) {
    interceptor->SetNext(loadHead_);
    loadHead_ = interceptor;
  } else {
    loadTail_->SetNext(interceptor);
    loadTail_ = interceptor;
  }
}

void ImageCacheLoaderInternal::AddDecodeInterceptor(
    std::shared_ptr<ImageCacheDecodeInterceptor> interceptor,
    ImageCacheLoader::Position position) {
  if (interceptor == nullptr) {
    return;
  }
  if (decodeHead_ == nullptr) {
    decodeHead_ = decodeTail_ = interceptor;
    return;
  }
  if (position == Position::START) {
    interceptor->SetNext(decodeHead_);
    decodeHead_ = interceptor;
  } else {
    decodeTail_->SetNext(interceptor);
    decodeTail_ = interceptor;
  }
}

bool ImageCacheLoaderInternal::LoadFromMemory(
    const std::shared_ptr<ImageCacheTaskInternal>& task) {
  if (task == nullptr) {
    return false;
  }
  if (memoryHead_ == nullptr) {
    return false;
  }
  task->cacheType = ImageCacheTaskType::READ;
  return memoryHead_->Process(task);
}

bool ImageCacheLoaderInternal::WriteCacheToMemory(
    const std::shared_ptr<ImageCacheTaskInternal>& task) {
  if (task == nullptr) {
    return false;
  }
  if (memoryHead_ == nullptr) {
    return false;
  }
  task->cacheType = ImageCacheTaskType::WRITE;
  return memoryHead_->Process(task);
}

bool ImageCacheLoaderInternal::DownloadImage(
    const std::shared_ptr<ImageCacheTaskInternal>& task) {
  if (task == nullptr) {
    return false;
  }
  if (loadHead_ == nullptr) {
    return false;
  }
  task->cacheType = ImageCacheTaskType::NONE;
  return loadHead_->Process(task);
}

bool ImageCacheLoaderInternal::DecodeImage(
    const std::shared_ptr<ImageCacheTaskInternal>& task) {
  if (task == nullptr) {
    return false;
  }
  if (decodeHead_ == nullptr) {
    return false;
  }
  task->cacheType = ImageCacheTaskType::NONE;
  return decodeHead_->Process(task);
}

void ImageCacheLoaderInternal::CancelLoad(
    const std::shared_ptr<ImageCacheTaskInternal>& task) {
  if (task == nullptr || loadHead_ == nullptr) {
    return;
  }
  loadHead_->Cancel(task);
}

} // namespace ImageCachePro
