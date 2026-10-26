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

#ifndef IMAGE_CACHE_DOWN_SAMPLING_DEFAULT_H
#define IMAGE_CACHE_DOWN_SAMPLING_DEFAULT_H

#include "ImageCacheDownSamplingBase.h"

namespace ImageCachePro {

class ImageCacheDownSamplingDefault : public ImageCacheDownSamplingBase {
 public:
  float GetScaleFactor(
      const Image_Size& sourceSize,
      const Image_Size& requestSize) override {
    (void)requestSize;
    const uint32_t resolutionMax8k = 7680 * 4320;
    uint32_t resolutionSource = sourceSize.width * sourceSize.height;
    if (resolutionSource <= resolutionMax8k) {
      return 1;
    }
    return static_cast<float>(resolutionSource) /
        static_cast<float>(resolutionMax8k);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWN_SAMPLING_DEFAULT_H
