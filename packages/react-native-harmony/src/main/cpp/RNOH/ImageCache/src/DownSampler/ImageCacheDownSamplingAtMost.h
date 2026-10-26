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

#ifndef IMAGE_CACHE_DOWN_SAMPLING_AT_MOST_H
#define IMAGE_CACHE_DOWN_SAMPLING_AT_MOST_H

#include "ImageCacheDownSamplingBase.h"

namespace ImageCachePro {

class ImageCacheDownSamplingAtMost : public ImageCacheDownSamplingBase {
 public:
  float GetScaleFactor(
      const Image_Size& sourceSize,
      const Image_Size& requestSize) override {
    IMAGE_CACHE_CHECK_RETURN(requestSize.width == 0, 1);
    IMAGE_CACHE_CHECK_RETURN(requestSize.height == 0, 1);
    uint32_t intScaleFactor = static_cast<uint32_t>(std::ceil(std::max(
        static_cast<float>(sourceSize.width) / requestSize.width,
        static_cast<float>(sourceSize.height) / requestSize.height)));
    uint32_t lesserOrEqualSampleSize = intScaleFactor > 1 ? intScaleFactor : 1;
    uint32_t greaterOrEqualSampleSize = lesserOrEqualSampleSize;
    if (lesserOrEqualSampleSize < intScaleFactor) {
      greaterOrEqualSampleSize = lesserOrEqualSampleSize << 1;
    }
    greaterOrEqualSampleSize = lesserOrEqualSampleSize
        << (lesserOrEqualSampleSize < intScaleFactor ? 1 : 0);
    IMAGE_CACHE_CHECK_RETURN(greaterOrEqualSampleSize == 0, 1);
    uint32_t width = std::round(
        (1.0f / static_cast<float>(greaterOrEqualSampleSize)) *
        sourceSize.width);
    uint32_t height = std::round(
        (1.0f / static_cast<float>(greaterOrEqualSampleSize)) *
        sourceSize.height);
    IMAGE_CACHE_CHECK_RETURN(width == 0, 1);
    IMAGE_CACHE_CHECK_RETURN(height == 0, 1);
    uint32_t scaleFactor = GetFactorAsBit(
        std::max(sourceSize.width / width, sourceSize.height / height));
    if (scaleFactor < greaterOrEqualSampleSize) {
      scaleFactor <<= 1;
    }
    return static_cast<float>(scaleFactor);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWN_SAMPLING_AT_MOST_H
