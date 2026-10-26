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

#ifndef IMAGE_CACHE_DOWN_SAMPLING_CENTER_INSIDE_H
#define IMAGE_CACHE_DOWN_SAMPLING_CENTER_INSIDE_H

#include "ImageCacheDownSamplingBase.h"

namespace ImageCachePro {

class ImageCacheDownSamplingCenterInside : public ImageCacheDownSamplingBase {
 public:
  explicit ImageCacheDownSamplingCenterInside(bool memory) : memory_(memory) {}

  float GetScaleFactor(
      const Image_Size& sourceSize,
      const Image_Size& requestSize) override {
    const float factor = GetFactor(sourceSize, requestSize);
    uint32_t width = std::round(factor * sourceSize.width);
    uint32_t height = std::round(factor * sourceSize.height);
    IMAGE_CACHE_CHECK_RETURN(width == 0, 1);
    IMAGE_CACHE_CHECK_RETURN(height == 0, 1);

    uint32_t scaleFactor;
    if (factor == 1) {
      scaleFactor = GetFactorAsBit(
          std::max(sourceSize.width / width, sourceSize.height / height));
    } else {
      scaleFactor = GetFactorAsBit(
          std::min(sourceSize.width / width, sourceSize.height / height));
    }
    IMAGE_CACHE_CHECK_RETURN(factor == 0, 1);
    if (memory_ && scaleFactor < (1.0f / factor)) {
      scaleFactor <<= 1;
    }
    return static_cast<float>(scaleFactor);
  }

 private:
  bool memory_ = false;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWN_SAMPLING_CENTER_INSIDE_H
