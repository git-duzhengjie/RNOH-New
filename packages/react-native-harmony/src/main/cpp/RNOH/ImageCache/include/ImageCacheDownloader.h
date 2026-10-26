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

#ifndef IMAGE_CACHE_DOWNLOADER_H
#define IMAGE_CACHE_DOWNLOADER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "ImageCacheMetrics.h"

namespace rnoh {
class ArkTSTurboModule;
}

namespace ImageCachePro {

class ImageCacheTaskInternal;
class ImageCacheDownloadInterceptor;

struct ImageCacheDownloadResult {
  std::shared_ptr<uint8_t[]> buffer;
  uint32_t length = 0;
  bool success = false;
  std::string errorMessage;
};

using DownloadCallback =
    std::function<void(uint32_t requestId, const ImageCacheDownloadResult&)>;

/** Shared cancel context between DownloadInterceptor and ImageCacheDownloader.
 */
struct DownloadCallbackContext {
  uint32_t requestId = 0;
  std::shared_ptr<ImageCacheTaskInternal> task;
  std::shared_ptr<ImageCacheDownloadInterceptor> interceptor;
  DownloadCallback onComplete;

  DownloadCallbackContext() {
    ImageCacheMetrics::Instance().Objects().Increment(
        "DownloadCallbackContext", this);
  }
  ~DownloadCallbackContext() {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "DownloadCallbackContext", this);
  }
  DownloadCallbackContext(const DownloadCallbackContext&) = delete;
  DownloadCallbackContext& operator=(const DownloadCallbackContext&) = delete;
};

class ImageCacheDownloader {
 public:
  virtual ~ImageCacheDownloader() = default;

  virtual std::shared_ptr<DownloadCallbackContext> AllocateCallbackContext(
      std::shared_ptr<ImageCacheTaskInternal> task = {},
      std::shared_ptr<ImageCacheDownloadInterceptor> interceptor = {}) {
    return nullptr;
  }

  virtual std::shared_ptr<DownloadCallbackContext> Get(
      uint32_t requestId,
      bool remove = false) {
    return nullptr;
  }

  virtual void Download(
      const std::string& url,
      const std::shared_ptr<DownloadCallbackContext>& ctx,
      ::ImageCachePro::DownloadCallback onComplete) = 0;
  virtual void Cancel(uint32_t requestId) = 0;
};

std::shared_ptr<ImageCacheDownloader> CreateImageLoaderDownloader(
    const std::shared_ptr<rnoh::ArkTSTurboModule>& imageLoader);

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWNLOADER_H
