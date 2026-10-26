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

#include "ImageCacheImageLoaderDownloader.h"

#include <glog/logging.h>
#include <cstring>
#include <fstream>
#include "RNOH/ArkTSTurboModule.h"

namespace ImageCachePro {

ImageCacheImageLoaderDownloader::ImageCacheImageLoaderDownloader(
    const std::shared_ptr<rnoh::ArkTSTurboModule>& imageLoader)
    : imageLoader_(imageLoader) {
  ImageCacheMetrics::Instance().Objects().Increment(
      "ImageCacheImageLoaderDownloader", this);
}

ImageCacheImageLoaderDownloader::~ImageCacheImageLoaderDownloader() {
  ImageCacheMetrics::Instance().Objects().Decrement(
      "ImageCacheImageLoaderDownloader", this);
  std::vector<int32_t> requestIds;
  {
    std::lock_guard<std::mutex> lock(contextsMutex_);
    requestIds.reserve(contexts_.size());
    for (const auto& pair : contexts_) {
      requestIds.push_back(pair.first);
    }
    contexts_.clear();
  }
  if (imageLoader_) {
    for (const auto& requestId : requestIds) {
      imageLoader_->callSync("abortRequest", {requestId});
    }
  }
  imageLoader_.reset();
}

namespace {

constexpr const char* FILE_PREFIX = "file://";

bool TryLoadFromPrefetchCache(
    const std::shared_ptr<rnoh::ArkTSTurboModule>& imageLoader,
    const std::string& url,
    ImageCacheDownloadResult& result) {
  if (imageLoader == nullptr) {
    return false;
  }

  auto prefetchResult = imageLoader->callSync("getPrefetchResult", {url});
  if (prefetchResult.isNull() || !prefetchResult.isString()) {
    return false;
  }

  const std::string fileUri = prefetchResult.asString();
  if (fileUri.empty() || fileUri == "pending") {
    return false;
  }

  std::string filePath = fileUri;
  if (filePath.rfind(FILE_PREFIX, 0) == 0) {
    filePath = filePath.substr(std::strlen(FILE_PREFIX));
  }

  if (filePath.empty()) {
    return false;
  }

  std::ifstream file(filePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return false;
  }

  auto size = file.tellg();
  if (size <= 0 || size > UINT32_MAX) {
    return false;
  }

  const auto fileSize = static_cast<size_t>(size);
  auto buffer = std::shared_ptr<uint8_t[]>(new uint8_t[fileSize]);
  file.seekg(0, std::ios::beg);
  file.read(
      reinterpret_cast<char*>(buffer.get()),
      static_cast<std::streamsize>(fileSize));
  if (!file || static_cast<size_t>(file.gcount()) != fileSize) {
    return false;
  }

  result.buffer = std::move(buffer);
  result.length = static_cast<uint32_t>(fileSize);
  result.success = true;
  return true;
}

} // namespace

std::shared_ptr<DownloadCallbackContext>
ImageCacheImageLoaderDownloader::AllocateCallbackContext(
    std::shared_ptr<ImageCacheTaskInternal> task,
    std::shared_ptr<ImageCacheDownloadInterceptor> interceptor) {
  auto imageLoader = imageLoader_;
  if (imageLoader == nullptr) {
    return nullptr;
  }
  auto ctx = std::make_shared<DownloadCallbackContext>();
  if (ctx == nullptr) {
    return nullptr;
  }
  ctx->requestId = nextRequestId_.fetch_add(1);
  ctx->task = std::move(task);
  ctx->interceptor = std::move(interceptor);
  std::lock_guard<std::mutex> lock(contextsMutex_);
  contexts_[ctx->requestId] = ctx;
  return ctx;
}

std::shared_ptr<DownloadCallbackContext> ImageCacheImageLoaderDownloader::Get(
    uint32_t requestId,
    bool remove) {
  std::lock_guard<std::mutex> lock(contextsMutex_);
  auto it = contexts_.find(requestId);
  if (it != contexts_.end()) {
    auto ctx = it->second;
    if (remove) {
      contexts_.erase(it);
    }
    return ctx;
  }
  return nullptr;
}

void ImageCacheImageLoaderDownloader::HandleDownloadResult(
    ImageCacheDownloadResult result,
    const std::string& url,
    uint32_t requestId) {
  auto ctx = Get(requestId);
  if (!ctx) {
    return;
  }

  if (result.success) {
    // Check if loader is still valid (may be reset in destructor)
    auto imageLoader = imageLoader_;
    if (imageLoader == nullptr) {
      result.success = false;
      result.errorMessage = std::string("ImageLoader unavailable");
    } else if (!TryLoadFromPrefetchCache(imageLoader, url, result)) {
      result.success = false;
      result.errorMessage = std::string("Prefetch cache file unavailable");
    }
  }
  if (result.success) {
    ImageCacheMetrics::Instance().Increment(MetricsCounter::DownloadsSucceeded);
  } else {
    ImageCacheMetrics::Instance().Increment(MetricsCounter::DownloadsFailed);
  }
  ctx->onComplete(requestId, result);
}
void ImageCacheImageLoaderDownloader::Download(
    const std::string& url,
    const std::shared_ptr<DownloadCallbackContext>& ctx,
    DownloadCallback onComplete) {
  auto imageLoader = imageLoader_;
  ImageCacheDownloadResult result;
  uint32_t requestId = ctx->requestId;
  if (imageLoader == nullptr) {
    result.errorMessage = "ImageLoader unavailable";
    onComplete(requestId, result);
    return;
  }

  if (TryLoadFromPrefetchCache(imageLoader, url, result)) {
    LOG(INFO) << "Load image from file cache: " << url;
    ImageCacheMetrics::Instance().Increment(MetricsCounter::LocalFileHits);
    onComplete(requestId, result);
    return;
  }

  ImageCacheMetrics::Instance().Increment(MetricsCounter::DownloadsInitiated);
  ctx->onComplete = std::move(onComplete);
  auto weakSelf = weak_from_this();
  imageLoader->callAsyncNative(
      "prefetchImage",
      {url, requestId},
      [weakSelf, url, requestId](folly::dynamic prefetchResult) {
        auto self = weakSelf.lock();
        if (!self) {
          return;
        }
        ImageCacheDownloadResult result;
        result.success = prefetchResult.isBool() && prefetchResult.asBool();
        self->HandleDownloadResult(result, url, requestId);
      },
      [weakSelf, url, requestId](std::string error) {
        auto self = weakSelf.lock();
        if (!self) {
          return;
        }
        ImageCacheDownloadResult result;
        result.success = false;
        result.errorMessage = std::move(error);
        self->HandleDownloadResult(result, url, requestId);
      });
}

void ImageCacheImageLoaderDownloader::Cancel(uint32_t requestId) {
  auto ctx = Get(requestId, true);
  auto imageLoader = imageLoader_;
  if (ctx && imageLoader) {
    imageLoader->callSync("abortRequest", {requestId});
  }
}

std::shared_ptr<ImageCacheDownloader> CreateImageLoaderDownloader(
    const std::shared_ptr<rnoh::ArkTSTurboModule>& imageLoader) {
  if (imageLoader == nullptr) {
    return nullptr;
  }
  return std::make_shared<ImageCacheImageLoaderDownloader>(imageLoader);
}

} // namespace ImageCachePro
