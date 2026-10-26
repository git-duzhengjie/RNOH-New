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

#ifndef IMAGE_CACHE_DATA_CACHE_INTERNAL_H
#define IMAGE_CACHE_DATA_CACHE_INTERNAL_H

#include <memory>
#include "ImageCacheData.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheType.h"

namespace ImageCachePro {

struct ImageCacheDataCacheInternal {
 public:
  size_t GetImageCacheSize() const {
    return cacheSize_;
  }

  std::shared_ptr<ImageCacheData> GetImageData() const {
    return imageData_;
  }

  std::shared_ptr<FrameDecodeContext> GetFrameDecodeContext() const {
    return frameDecodeContext_;
  }

  ImageCacheDataCacheInternal(
      size_t cacheSize,
      std::shared_ptr<ImageCacheData> data,
      std::shared_ptr<FrameDecodeContext> frameDecodeContext)
      : cacheSize_(cacheSize),
        imageData_(std::move(data)),
        frameDecodeContext_(std::move(frameDecodeContext)) {
    ImageCacheMetrics::Instance().Objects().Increment(
        "ImageCacheDataCacheInternal", this);
  }

  ~ImageCacheDataCacheInternal() {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "ImageCacheDataCacheInternal", this);
  }

 private:
  size_t cacheSize_ = 0;
  std::shared_ptr<ImageCacheData> imageData_ = nullptr;
  std::shared_ptr<FrameDecodeContext> frameDecodeContext_ = nullptr;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DATA_CACHE_INTERNAL_H
