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

#ifndef IMAGE_CACHE_INTERCEPTOR_H
#define IMAGE_CACHE_INTERCEPTOR_H

#include <functional>
#include <memory>
#include <string>
#include "ImageCacheTask.h"

namespace ImageCachePro {

class ImageCacheManagerInternal;
class ImageCacheTaskInternal;

class ImageCacheInterceptor
    : public std::enable_shared_from_this<ImageCacheInterceptor> {
 public:
  explicit ImageCacheInterceptor(ImageCacheManagerInternal& manager)
      : manager_(manager) {}
  std::string name;
  virtual bool Resolve(const std::shared_ptr<ImageCacheTask>& task) = 0;
  virtual void Cancel(const std::shared_ptr<ImageCacheTask>& task);
  virtual bool Process(
      const std::shared_ptr<ImageCacheTask>& task,
      std::function<bool(const std::shared_ptr<ImageCacheTask>&)> resolveCallback =
          nullptr);
  virtual ~ImageCacheInterceptor() = default;
  ImageCacheManagerInternal& GetManager() const { return manager_; }

 protected:
  // Thread-safety invariant: next_ is set once during chain construction
  // (via SetNext) before any Process/Cancel calls. Once the chain is built,
  // next_ is only read. Do NOT call SetNext after the chain starts processing.
  std::shared_ptr<ImageCacheInterceptor> next_ = nullptr;
  ImageCacheManagerInternal& manager_;
};

class ImageCacheMemoryInterceptor : public ImageCacheInterceptor {
 public:
  explicit ImageCacheMemoryInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheInterceptor(manager) {}
  bool Process(
      const std::shared_ptr<ImageCacheTask>& task,
      std::function<bool(const std::shared_ptr<ImageCacheTask>&)> resolveCallback =
          nullptr) final;
  void SetNext(const std::shared_ptr<ImageCacheMemoryInterceptor>& interceptor);
};

class ImageCacheLoadInterceptor : public ImageCacheInterceptor {
 public:
  explicit ImageCacheLoadInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheInterceptor(manager) {}
  void OnComplete(const std::shared_ptr<ImageCacheTask>& task, bool result);
  void Cancel(const std::shared_ptr<ImageCacheTask>& task) override;
  bool Process(
      const std::shared_ptr<ImageCacheTask>& task,
      std::function<bool(const std::shared_ptr<ImageCacheTask>&)> resolveCallback =
          nullptr) final;
  void SetNext(const std::shared_ptr<ImageCacheLoadInterceptor>& interceptor);

 protected:
  bool isLoadFromRemote = true;
};

class ImageCacheDecodeInterceptor : public ImageCacheInterceptor {
 public:
  explicit ImageCacheDecodeInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheInterceptor(manager) {}
  bool Process(
      const std::shared_ptr<ImageCacheTask>& task,
      std::function<bool(const std::shared_ptr<ImageCacheTask>&)> resolveCallback =
          nullptr) final;
  void SetNext(const std::shared_ptr<ImageCacheDecodeInterceptor>& interceptor);
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_INTERCEPTOR_H
