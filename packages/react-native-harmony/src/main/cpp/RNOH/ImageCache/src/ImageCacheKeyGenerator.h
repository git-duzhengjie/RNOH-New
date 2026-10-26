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

#ifndef IMAGE_CACHE_KEY_GENERATOR_H
#define IMAGE_CACHE_KEY_GENERATOR_H

#include <string>
#include "ImageCacheSource.h"
#include "ImageCacheOption.h"
#include "ImageCacheType.h"

namespace ImageCachePro {

class ImageCacheKeyGenerator {
 public:
  struct MemoryKeyParams {
    const ImageCacheSource* imageSource = nullptr;
    ImageCacheRequestType type = ImageCacheRequestType::MAIN_SRC;
    std::string signature;
    std::string transformationInfo;
    std::string downSamplingInfo;
    bool isAnimator = false;
    ResourceManager_ColorMode colorMode = COLOR_MODE_LIGHT;
    ImageCacheOrientation orientation = ImageCacheOrientation::AUTO;
    ImageCacheDynamicRange dynamicRange = ImageCacheDynamicRange::AUTO;
    float resizeMultiplier = 1.0f;

    MemoryKeyParams() = default;

    // Build from option + imageSource (shared_ptr overload)
    MemoryKeyParams(
        const ImageCacheSource* src,
        ImageCacheRequestType reqType,
        const std::shared_ptr<class ImageCacheOption>& opt,
        bool animated)
        : imageSource(src), type(reqType), isAnimator(animated) {
      if (opt) {
        signature = opt->signature;
        downSamplingInfo = opt->downSamplingInfo;
        colorMode = opt->colorMode;
        orientation = opt->orientation;
        dynamicRange = opt->dynamicRange;
        resizeMultiplier = opt->resizeMultiplier;
      }
    }

    // Build from option + imageSource (const reference overload)
    MemoryKeyParams(
        const ImageCacheSource* src,
        ImageCacheRequestType reqType,
        const class ImageCacheOption& opt,
        bool animated)
        : imageSource(src),
          type(reqType),
          signature(opt.signature),
          downSamplingInfo(opt.downSamplingInfo),
          isAnimator(animated),
          colorMode(opt.colorMode),
          orientation(opt.orientation),
          dynamicRange(opt.dynamicRange),
          resizeMultiplier(opt.resizeMultiplier) {}
  };

  virtual ~ImageCacheKeyGenerator() = default;
  /** Request merge / in-flight deduplication key, excludes isAnimator */
  virtual std::string GenerateRequestKey(const MemoryKeyParams& params) = 0;
  /** Memory cache key, isAnimator indicates whether stored as animated image */
  virtual std::string GenerateMemoryKey(const MemoryKeyParams& params) = 0;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_KEY_GENERATOR_H
