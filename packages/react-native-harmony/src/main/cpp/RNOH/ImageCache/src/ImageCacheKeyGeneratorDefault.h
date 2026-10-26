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

#ifndef IMAGE_CACHE_KEY_GENERATOR_DEFAULT_H
#define IMAGE_CACHE_KEY_GENERATOR_DEFAULT_H

#include <sstream>
#include "ImageCacheKeyGenerator.h"
#include "ImageCacheLog.h"

namespace ImageCachePro {

inline std::string GenerateKey(const ImageCacheKeyGenerator::MemoryKeyParams& params, bool isAnimator) {
    IMAGE_CACHE_CHECK_RETURN(params.imageSource == nullptr, "");
    std::string sourceKey;
    ImageCacheResource resource;
    std::ostringstream key;
    key << "src=";
    if (params.imageSource->GetString(sourceKey)) {
      key << sourceKey << ";";
    } else {
      params.imageSource->GetResource(resource);
      key << resource.bundleName << "/" << resource.moduleName << "/"
          << params.colorMode << "/" << resource.id << "/" << resource.param
          << ";";
    }

    if (params.type == ImageCacheRequestType::MAIN_SRC) {
      key << "o:" << static_cast<int>(params.orientation) << ";"
          << ";d:" << static_cast<int>(params.dynamicRange) << ";";
      if (!params.transformationInfo.empty()) {
        key << params.transformationInfo << ";";
      }
      if (!params.downSamplingInfo.empty()) {
        key << params.downSamplingInfo << ";";
      }
      if (params.resizeMultiplier != 1.0f) {
        key << "rm:" << params.resizeMultiplier << ";";
      }
      if (!params.signature.empty()) {
        key << "signature=" << params.signature << ";";
      }
    }
    if (isAnimator) {
      key << "animator=true";
    }
    return key.str();
  }

class ImageCacheKeyGeneratorDefault : public ImageCacheKeyGenerator {
 public:
  std::string GenerateRequestKey(const ImageCacheKeyGenerator::MemoryKeyParams& params) override {
    IMAGE_CACHE_CHECK_RETURN(params.imageSource == nullptr, "");
    return GenerateKey(params, false);
  }

  std::string GenerateMemoryKey(const ImageCacheKeyGenerator::MemoryKeyParams& params) override {
    IMAGE_CACHE_CHECK_RETURN(params.imageSource == nullptr, "");
    return GenerateKey(params, params.isAnimator);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_KEY_GENERATOR_DEFAULT_H
