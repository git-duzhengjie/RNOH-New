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

#ifndef IMAGE_CACHE_OPTION_H
#define IMAGE_CACHE_OPTION_H

#include <multimedia/image_framework/image/image_common.h>
#include <memory>
#include <string>
#include "ImageCacheData.h"
#include "ImageCacheMetrics.h"
#include "ImageCacheSource.h"
#include "ImageCacheType.h"

namespace ImageCachePro {
class ImageCacheTransformation;
class ImageCacheMultiTransformation;

class ImageCacheOption {
  friend class ImageCacheTaskInternal;
  friend class ImageCacheKeyGenerator;
  friend class ImageCacheDispatcher;
  friend class ImageCacheManagerInternal;
  friend class ImageCacheJob;
  friend class ImageCacheRequest;
  friend class ImageCacheAssetLoadInterceptor;
  friend class TransformationMask;
  friend class ImageCacheDownSampler;

 public:
  ImageCacheOption() {
    ImageCacheMetrics::Instance().Objects().Increment("ImageCacheOption", this);
  }
  ~ImageCacheOption() {
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
    transformation = nullptr;
    multiTransformation = nullptr;
#endif
    ImageCacheMetrics::Instance().Objects().Decrement("ImageCacheOption", this);
  }

  ImageCacheOption(const ImageCacheOption& other);

  std::shared_ptr<ImageCacheOption> Clone() const {
    return std::make_shared<ImageCacheOption>(*this);
  }

  bool IsValid() const {
    return !loadSrc.IsEmpty();
  }

  const ImageCacheSource& GetLoadSrc() const {
    return loadSrc;
  }
  const std::string& GetTransformSpec() const {
    return transformSpec;
  }
  bool IsCallbackOnMainThread() const {
    return callbackOnMainThread;
  }
  const std::string& GetAssetsPrefix() const {
    return assetsPrefix;
  }
  void AppendTransformSpec(const std::string& spec) {
    if (transformSpec.empty()) {
      transformSpec = spec;
    } else {
      transformSpec += "+" + spec;
    }
  }

  void PrepareTransformations();
  std::string ResolveTransformationInfo() const;

  /**
   * Factory method: Build a fully configured ImageCacheOption from Image
   * component parameters. Encapsulates the setting logic for loadSrc,
   * assetsPrefix, isAnimator, downSampling, requestImageSize, transformSpec,
   * and other fields, and automatically calls PrepareTransformations.
   *
   * @param uri Image source URI
   * @param assetsPrefix RNOH assets prefix, used for asset:// parsing
   * @param resizeMethod resize method string (empty string means unspecified,
   * uses DEFAULT strategy)
   * @param requestWidth Expected display width (physical pixels)
   * @param requestHeight Expected display height (physical pixels)
   * @param blurRadius Blur radius (0 means no blur)
   * @return Configured option with PrepareOption completed
   */
  static std::shared_ptr<ImageCacheOption> Create(
      const std::string& uri,
      const std::string& assetsPrefix,
      const std::string& resizeMethod,
      uint32_t requestWidth,
      uint32_t requestHeight,
      float blurRadius = 0.0f,
      ResourceManager_ColorMode colorMode = COLOR_MODE_LIGHT,
      ImageCacheOrientation orientation = ImageCacheOrientation::AUTO,
      ImageCacheDynamicRange dynamicRange = ImageCacheDynamicRange::AUTO,
      float resizeMultiplier = 1.0f);

 private:
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  void ApplyTransformSpec(const std::string& spec);
#endif

  ImageCacheSource loadSrc;
  std::string signature;
  std::string transformationInfo;
  std::string transformSpec;
  std::string downSamplingInfo;
  ImageCacheDownSamplingStrategy downSampling =
      ImageCacheDownSamplingStrategy::DEFAULT;
  Image_Size requestImageSize = {0, 0};
  bool isAnimator = false;
  ImageCacheAnimationDecodeMode animationDecodeMode =
      ImageCacheAnimationDecodeMode::AUTO;
  std::string assetsPrefix;
  ResourceManager_ColorMode colorMode = COLOR_MODE_LIGHT;
  ImageCacheOrientation orientation = ImageCacheOrientation::AUTO;
  ImageCacheDynamicRange dynamicRange = ImageCacheDynamicRange::AUTO;
  bool onlyRetrieveFromCache = false;
  ImageCacheStrategy writeCacheStrategy = ImageCacheStrategy::MEMORY;
  ImageCachePriority priority = ImageCachePriority::MEDIUM;
  float resizeMultiplier = 1.0f;
  bool callbackOnMainThread = true;
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  std::shared_ptr<ImageCacheTransformation> transformation = nullptr;
  std::shared_ptr<ImageCacheMultiTransformation> multiTransformation = nullptr;
#endif
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_OPTION_H
