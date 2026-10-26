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

#include "ImageCacheData.h"
#include <algorithm>
#include <climits>
#include "ImageCacheLog.h"
#include "ImageCacheMetrics.h"
#include "ImageCachePixelmapUtils.h"

namespace ImageCachePro {

ImageCacheData::ImageCacheData(OH_PixelmapNative* pixelmap)
    : frameCount_(pixelmap != nullptr ? 1 : 0) {
  ImageCacheMetrics::Instance().Objects().Increment("ImageCacheData", this);
  if (pixelmap != nullptr) {
    pixelmapList_ = std::make_unique<OH_PixelmapNative*[]>(1);
    pixelmapList_[0] = pixelmap;
  }
}

ImageCacheData::ImageCacheData(
    std::unique_ptr<OH_PixelmapNative*[]> pixelmapList,
    std::unique_ptr<int32_t[]> delayTimeList,
    size_t frameCount)
    : pixelmapList_(std::move(pixelmapList)),
      delayTimeList_(std::move(delayTimeList)),
      frameCount_(
          pixelmapList_ != nullptr
              ? static_cast<uint32_t>(
                    std::min(frameCount, static_cast<size_t>(UINT32_MAX)))
              : 0) {
  ImageCacheMetrics::Instance().Objects().Increment("ImageCacheData", this);
}

OH_PixelmapNative* ImageCacheData::GetPixelmap() const {
  if (pixelmapList_ == nullptr || frameCount_ == 0) {
    return nullptr;
  }
  return pixelmapList_[0];
}

OH_PixelmapNative* ImageCacheData::GetPixelmapAt(size_t index) const {
  if (pixelmapList_ == nullptr || index >= frameCount_) {
    return nullptr;
  }
  return pixelmapList_[index];
}

OH_PixelmapNative** ImageCacheData::GetPixelmapList() const {
  return pixelmapList_.get();
}

int32_t* ImageCacheData::GetDelayTimeList() const {
  return delayTimeList_.get();
}

uint32_t ImageCacheData::GetFrameCount() const {
  return frameCount_;
}

Image_Size ImageCacheData::GetImageSize() const {
  return ImageCachePixelmapUtils::GetImageSize(GetPixelmap(), nullptr);
}

ImageCacheData::~ImageCacheData() {
  ImageCacheMetrics::Instance().Objects().Decrement("ImageCacheData", this);
  if (pixelmapList_ != nullptr && frameCount_ > 0) {
    for (uint32_t i = 0; i < frameCount_; i++) {
      OH_PixelmapNative* pixelmap = pixelmapList_[i];
      if (pixelmap != nullptr) {
        OH_PixelmapNative_Release(pixelmap);
        pixelmapList_[i] = nullptr;
      }
    }
  }
  delayTimeList_.reset();
}

} // namespace ImageCachePro
