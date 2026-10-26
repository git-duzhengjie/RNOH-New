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

#ifndef IMAGE_CACHE_REQUEST_H
#define IMAGE_CACHE_REQUEST_H

#include <atomic>
#include <functional>
#include <memory>
#include "ImageCacheMetrics.h"
#include "ImageCacheOption.h"
#include "ImageCacheTaskWorker.h"
#include "ImageCacheType.h"
#include "utils/ImageCacheLog.h"

namespace rnoh {
class TaskExecutor;
}

namespace ImageCachePro {
struct ImageCacheJob;

enum class ImageCacheHandleType : uint8_t {
  REQUEST,
  ANIMATION,
};

namespace {
constexpr ImageCacheRequestHandle kMaxHandle = INT64_MAX - 1000;
}

class ImageCacheRequest {
 public:
  ImageCacheRequest(
      ImageCacheRequestHandle handle,
      const std::shared_ptr<ImageCacheOption>& option)
      : requestHandle(handle), option(std::move(option)) {}
  virtual ~ImageCacheRequest() = default;

  virtual void InvokeCallback(
      std::shared_ptr<rnoh::TaskExecutor> taskExecutor,
      ImageCacheLoadData data,
      ImageCacheLoadResult result) = 0;

  virtual ImageCacheHandleType GetRequestType() const = 0;

  static ImageCacheRequestHandle AllocateHandle() {
    static std::atomic<ImageCacheRequestHandle> sNextHandle{1};
    auto h = sNextHandle.fetch_add(1, std::memory_order_relaxed);
    if (h >= kMaxHandle) {
      LOG(WARNING) << "ImageCache handle overflow detected, resetting counter";
      sNextHandle.store(1, std::memory_order_relaxed);
      return sNextHandle.fetch_add(1, std::memory_order_relaxed);
    }
    return h;
  }
  ImageCacheRequestHandle GetRequestHandle() const {
    return requestHandle;
  }
  std::shared_ptr<ImageCacheOption> GetOption() const {
    return option;
  }

 protected:
  ImageCacheRequestHandle requestHandle;
  const std::shared_ptr<ImageCacheOption> option;
};

class ImageCacheNormalRequest : public ImageCacheRequest {
 public:
  ImageCacheNormalRequest(
      ImageCacheRequestHandle handle,
      const std::shared_ptr<ImageCacheOption>& option,
      ImageCacheGetCacheCompleteCallback onComplete)
      : ImageCacheRequest(handle, option), onComplete(std::move(onComplete)) {
    ImageCacheMetrics::Instance().Objects().Increment(
        "ImageCacheNormalRequest", this);
  }

  ~ImageCacheNormalRequest() override {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "ImageCacheNormalRequest", this);
  }

  ImageCacheHandleType GetRequestType() const override {
    return ImageCacheHandleType::REQUEST;
  }
  void InvokeCallback(
      std::shared_ptr<rnoh::TaskExecutor> taskExecutor,
      ImageCacheLoadData data,
      ImageCacheLoadResult result) override {
    bool expected = false;
    if (!callbackInvoked_.compare_exchange_strong(expected, true)) {
      return;
    }
    if (!onComplete) {
      return;
    }
    if (option && option->IsCallbackOnMainThread()) {
      auto cb = onComplete;
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
    onComplete(std::move(data), std::move(result));
  }

 private:
  ImageCacheGetCacheCompleteCallback onComplete;
  std::atomic<bool> callbackInvoked_{false};
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_REQUEST_H
