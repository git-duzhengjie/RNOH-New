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

#ifndef IMAGE_CACHE_DOWN_SAMPLING_BASE_H
#define IMAGE_CACHE_DOWN_SAMPLING_BASE_H

#include <multimedia/image_framework/image/image_common.h>
#include <algorithm>
#include <cstdint>
#include "ImageCacheLog.h"

namespace ImageCachePro {

class ImageCacheDownSamplingBase {
 public:
  virtual float GetScaleFactor(
      const Image_Size& sourceSize,
      const Image_Size& requestSize) = 0;
  virtual ~ImageCacheDownSamplingBase() = default;

 protected:
  uint32_t GetFactorAsBit(uint32_t num) {
    uint32_t res = HighestOneBit(num);
    return res > 1 ? res : 1;
  }

  float GetFactor(const Image_Size& sourceSize, const Image_Size& requestSize) {
    IMAGE_CACHE_CHECK_RETURN(requestSize.width == 0, 1);
    IMAGE_CACHE_CHECK_RETURN(requestSize.height == 0, 1);
    uint32_t maxIntFactor = std::max(
        sourceSize.width / requestSize.width,
        sourceSize.height / requestSize.height);
    return maxIntFactor == 0
        ? 1.0f
        : 1.0f / static_cast<float>(HighestOneBit(maxIntFactor));
  }

 private:
  uint32_t HighestOneBit(uint32_t num) {
    int pos = 1;
    const int count = 5;
    for (int i = 0; i < count; i++) {
      num |= (num >> pos);
      pos += pos;
    }
    return num - (num >> 1);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWN_SAMPLING_BASE_H
