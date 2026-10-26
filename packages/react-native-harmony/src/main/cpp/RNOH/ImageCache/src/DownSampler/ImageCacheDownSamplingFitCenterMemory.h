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

#ifndef IMAGE_CACHE_DOWN_SAMPLING_FIT_CENTER_MEMORY_H
#define IMAGE_CACHE_DOWN_SAMPLING_FIT_CENTER_MEMORY_H

#include "ImageCacheDownSamplingBase.h"

namespace ImageCachePro {

class ImageCacheDownSamplingFitCenterMemory
    : public ImageCacheDownSamplingBase {
 public:
  float GetScaleFactor(
      const Image_Size& sourceSize,
      const Image_Size& requestSize) override {
    IMAGE_CACHE_CHECK_RETURN(sourceSize.width == 0, 1);
    IMAGE_CACHE_CHECK_RETURN(sourceSize.height == 0, 1);
    const float percentageFactor = std::min(
        static_cast<float>(requestSize.width) / sourceSize.width,
        static_cast<float>(requestSize.height) / sourceSize.height);
    uint32_t width = std::round(percentageFactor * sourceSize.width);
    uint32_t height = std::round(percentageFactor * sourceSize.height);
    IMAGE_CACHE_CHECK_RETURN(width == 0, 1);
    IMAGE_CACHE_CHECK_RETURN(height == 0, 1);
    uint32_t scaleFactor = GetFactorAsBit(
        std::min(sourceSize.width / width, sourceSize.height / height));
    if (percentageFactor != 0 && scaleFactor < (1 / percentageFactor)) {
      scaleFactor <<= 1;
    }
    return static_cast<float>(scaleFactor);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWN_SAMPLING_FIT_CENTER_MEMORY_H
