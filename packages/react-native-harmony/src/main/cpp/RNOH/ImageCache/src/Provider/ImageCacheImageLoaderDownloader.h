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

#ifndef IMAGE_CACHE_IMAGE_LOADER_DOWNLOADER_H
#define IMAGE_CACHE_IMAGE_LOADER_DOWNLOADER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "ImageCacheDownloader.h"
#include "ImageCacheMetrics.h"

namespace rnoh {
class ArkTSTurboModule;
}

namespace ImageCachePro {

/**
 * @brief Holds ImageLoader (ArkTSTurboModule), calls prefetchImage to download
 * HTTP(S) images.
 */
class ImageCacheImageLoaderDownloader
    : public ImageCacheDownloader,
      public std::enable_shared_from_this<ImageCacheImageLoaderDownloader> {
 public:
  explicit ImageCacheImageLoaderDownloader(
      const std::shared_ptr<rnoh::ArkTSTurboModule>& imageLoader);

  ~ImageCacheImageLoaderDownloader() override;

  std::shared_ptr<DownloadCallbackContext> AllocateCallbackContext(
      std::shared_ptr<ImageCacheTaskInternal> task = {},
      std::shared_ptr<ImageCacheDownloadInterceptor> interceptor = {}) override;
  std::shared_ptr<DownloadCallbackContext> Get(
      uint32_t requestId,
      bool remove = false) override;
  void Download(
      const std::string& url,
      const std::shared_ptr<DownloadCallbackContext>& ctx,
      DownloadCallback onComplete) override;
  void Cancel(uint32_t requestId) override;

 private:
  void HandleDownloadResult(
      ImageCacheDownloadResult result,
      const std::string& url,
      uint32_t requestId);
  std::shared_ptr<rnoh::ArkTSTurboModule> imageLoader_;
  std::atomic<uint32_t> nextRequestId_{1};
  std::mutex contextsMutex_;
  std::unordered_map<uint32_t, std::shared_ptr<DownloadCallbackContext>>
      contexts_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_IMAGE_LOADER_DOWNLOADER_H
