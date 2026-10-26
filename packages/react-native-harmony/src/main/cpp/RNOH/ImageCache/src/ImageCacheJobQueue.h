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

#ifndef IMAGE_CACHE_JOB_QUEUE_H
#define IMAGE_CACHE_JOB_QUEUE_H

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>
#include "ImageCacheMetrics.h"
#include "ImageCacheRequest.h"
#include "ImageCacheSource.h"
#include "ImageCacheType.h"

namespace ImageCachePro {

class ImageCacheDataCacheInternal;
class ImageCacheDispatcher;
class ImageCacheManagerInternal;

class ImageCacheJob : public std::enable_shared_from_this<ImageCacheJob> {
 public:
  friend class ImageCacheTaskInternal;
  friend class ImageCacheDispatcher;

  ImageCacheJob() {
    ImageCacheMetrics::Instance().Objects().Increment("ImageCacheJob", this);
  }
  ~ImageCacheJob() {
    ImageCacheMetrics::Instance().Objects().Decrement("ImageCacheJob", this);
  }
  static std::shared_ptr<ImageCacheJob> CreateJob(
      const std::string& requestKey,
      std::shared_ptr<ImageCacheOption> option,
      std::shared_ptr<ImageCacheDispatcher> dispatcher,
      std::shared_ptr<ImageCacheRequest> request = nullptr,
      FrameDecodeRequest decodeRequest = {0, 1, nullptr});

  void UpdateDecodeRequest(const FrameDecodeRequest& decodeRequest);

  // -- RequestID management (thread-safe) --
  void AddRequest(std::shared_ptr<ImageCacheRequest> request) {
    std::lock_guard<std::mutex> lock(requestIdsMutex_);
    requestIds.push_back(request->GetRequestHandle());
  }

  bool RemoveRequest(ImageCacheRequestHandle id) {
    std::lock_guard<std::mutex> lock(requestIdsMutex_);
    requestIds.erase(
        std::remove(requestIds.begin(), requestIds.end(), id),
        requestIds.end());
    return requestIds.empty();
  }

  void NotifyRequests(std::function<void(ImageCacheRequestHandle)> callback) {
    std::vector<ImageCacheRequestHandle> ids;
    {
      std::lock_guard<std::mutex> lock(requestIdsMutex_);
      ids = std::move(requestIds);
    }
    for (auto rid : ids) {
      callback(rid);
    }
  }

  // -- Public accessors --
  ImageCacheRequestType GetType() const {
    return type;
  }

  const ImageCacheSource& GetImageSource() const {
    return imageSrc;
  }

  const std::shared_ptr<ImageCacheOption>& GetOption() const {
    return option;
  }

  const std::string& GetRequestKey() const {
    return requestKey;
  }

  // -- Frame decode accessors --
  int GetFrameIndex() const {
    return decodeRequest_.startFrame;
  }

  const FrameDecodeRequest& GetDecodeRequest() const {
    return decodeRequest_;
  }

  bool IsFrameDecode() const {
    return decodeRequest_.frameContext != nullptr;
  }

 private:
  std::string requestKey;
  std::shared_ptr<ImageCacheOption> option;
  ImageCacheSource imageSrc;
  ImageCacheRequestType type = ImageCacheRequestType::MAIN_SRC;
  std::shared_ptr<class ImageCacheTaskInternal> task;
  std::vector<ImageCacheRequestHandle> requestIds;
  FrameDecodeRequest decodeRequest_;
  ImageCachePriority priority{ImageCachePriority::MEDIUM};
  mutable std::mutex requestIdsMutex_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_JOB_QUEUE_H
