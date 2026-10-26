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

#ifndef IMAGE_CACHE_DOWN_SAMPLER_H
#define IMAGE_CACHE_DOWN_SAMPLER_H

#include <multimedia/image_framework/image/image_common.h>
#include <cmath>
#include "ImageCacheDownSamplingAtLeast.h"
#include "ImageCacheDownSamplingAtMost.h"
#include "ImageCacheDownSamplingCenterInside.h"
#include "ImageCacheDownSamplingDefault.h"
#include "ImageCacheDownSamplingFitCenterMemory.h"
#include "ImageCacheDownSamplingFitCenterQuality.h"
#include "ImageCacheLog.h"
#include "ImageCacheType.h"

namespace ImageCachePro {

class ImageCacheDownSampler {
 public:
  static std::string GetDownSamplingInfo(
      Image_Size requestSize,
      ImageCacheDownSamplingStrategy strategy,
      float resizeMultiplier = 1.0f) {
    if (strategy == ImageCacheDownSamplingStrategy::NONE) {
      return std::string();
    }
    if (strategy == ImageCacheDownSamplingStrategy::DEFAULT) {
      return "DownSampling:Default";
    }
    std::string info =
        "DownSampling:" + std::to_string(static_cast<int>(strategy)) + "," +
        std::to_string(requestSize.width) + "," +
        std::to_string(requestSize.height);
    if (resizeMultiplier != 1.0f) {
      info += ",rm:" + std::to_string(resizeMultiplier);
    }
    return info;
  }

  static Image_Size CalculateSize(
      ImageCacheFormat format,
      Image_Size sourceSize,
      Image_Size requestSize,
      ImageCacheDownSamplingStrategy strategy,
      float resizeMultiplier = 1.0f) {
    Image_Size imageSize = {.width = 0, .height = 0};
    // §6.3: return a 1-pixel placeholder when size estimation fails
    if (sourceSize.width == 0 || sourceSize.height == 0) {
      return {.width = 1, .height = 1};
    }

    if (format == ImageCacheFormat::SVG) {
      return imageSize;
    }

    // §6.1 large-image guard: downgrade NONE -> DEFAULT
    if (strategy == ImageCacheDownSamplingStrategy::NONE) {
      constexpr size_t kLargeImagePixels = 4096 * 4096;
      size_t sourcePixels =
          static_cast<size_t>(sourceSize.width) * sourceSize.height;
      if (sourcePixels > kLargeImagePixels) {
        strategy = ImageCacheDownSamplingStrategy::DEFAULT;
      }
    }

    ImageCacheDownSamplingBase* downSampling = GetDownSampling(strategy);
    if (downSampling == nullptr) {
      return imageSize;
    }

    if ((requestSize.width == 0 || requestSize.height == 0) &&
        strategy != ImageCacheDownSamplingStrategy::DEFAULT) {
      return imageSize;
    }

    Image_Size actualRequestSize = {
        static_cast<uint32_t>(requestSize.width * resizeMultiplier),
        static_cast<uint32_t>(requestSize.height * resizeMultiplier)};

    float scaleFactor =
        downSampling->GetScaleFactor(sourceSize, actualRequestSize);
    if (scaleFactor == 0) {
      return {.width = 1, .height = 1};
    }

    if (format == ImageCacheFormat::PNG) {
      imageSize.width =
          static_cast<uint32_t>(std::floor(sourceSize.width / scaleFactor));
      imageSize.height =
          static_cast<uint32_t>(std::floor(sourceSize.height / scaleFactor));
    } else if (format == ImageCacheFormat::WEBP) {
      imageSize.width =
          static_cast<uint32_t>(std::round(sourceSize.width / scaleFactor));
      imageSize.height =
          static_cast<uint32_t>(std::round(sourceSize.height / scaleFactor));
    } else {
      imageSize.width = static_cast<uint32_t>(sourceSize.width / scaleFactor);
      imageSize.height = static_cast<uint32_t>(sourceSize.height / scaleFactor);
    }

    // §6.3: return a placeholder if the result is still {0,0}
    if (imageSize.width == 0 || imageSize.height == 0) {
      return {.width = 1, .height = 1};
    }
    return imageSize;
  }

 private:
  static ImageCacheDownSamplingBase* GetDownSampling(
      ImageCacheDownSamplingStrategy strategy) {
    static ImageCacheDownSamplingDefault defaultSampler;
    static ImageCacheDownSamplingFitCenterMemory fitCenterMemory;
    static ImageCacheDownSamplingFitCenterQuality fitCenterQuality;
    static ImageCacheDownSamplingCenterInside centerInsideMemory(true);
    static ImageCacheDownSamplingCenterInside centerInsideQuality(false);
    static ImageCacheDownSamplingAtLeast atLeast;
    static ImageCacheDownSamplingAtMost atMost;

    switch (strategy) {
      case ImageCacheDownSamplingStrategy::NONE:
        return nullptr;
      case ImageCacheDownSamplingStrategy::DEFAULT:
        return &defaultSampler;
      case ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY:
        return &fitCenterMemory;
      case ImageCacheDownSamplingStrategy::FIT_CENTER_QUALITY:
        return &fitCenterQuality;
      case ImageCacheDownSamplingStrategy::CENTER_INSIDE_MEMORY:
        return &centerInsideMemory;
      case ImageCacheDownSamplingStrategy::CENTER_INSIDE_QUALITY:
        return &centerInsideQuality;
      case ImageCacheDownSamplingStrategy::AT_LEAST:
        return &atLeast;
      case ImageCacheDownSamplingStrategy::AT_MOST:
        return &atMost;
      default:
        return nullptr;
    }
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DOWN_SAMPLER_H
