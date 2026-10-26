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

#include "ImageCacheOption.h"
#include "ImageCacheDownSampler.h"
#include "ImageCacheKeyGenerator.h"
#include "ImageCacheKeyGeneratorDefault.h"
#include "ImageCacheLog.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <unordered_map>

#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
#include "ImageCacheTransformationFactory.h"
#include "ImageCacheTransformationOption.h"
#endif

namespace ImageCachePro {

static std::string ToLowerAscii(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });
  return value;
}

static std::string TrimAscii(std::string value) {
  auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(
      value.begin(), std::find_if(value.begin(), value.end(), notSpace));
  value.erase(
      std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
  return value;
}

static std::string ToUpperAscii(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
      });
  return value;
}

static std::vector<std::string> SplitByChar(
    const std::string& value,
    char delimiter) {
  std::vector<std::string> parts;
  std::stringstream stream(value);
  std::string item;
  while (std::getline(stream, item, delimiter)) {
    item = TrimAscii(item);
    if (!item.empty()) {
      parts.push_back(item);
    }
  }
  return parts;
}

static bool EndsWithIgnoreCase(
    const std::string& value,
    const std::string& suffix) {
  if (value.size() < suffix.size()) {
    return false;
  }
  for (size_t i = 0; i < suffix.size(); ++i) {
    if (std::tolower(
            static_cast<unsigned char>(
                value[value.size() - suffix.size() + i])) !=
        std::tolower(static_cast<unsigned char>(suffix[i]))) {
      return false;
    }
  }
  return true;
}

static bool IsAnimatorUri(const std::string& uri) {
  if (EndsWithIgnoreCase(uri, ".gif")) {
    return true;
  }
  auto lowerUri = ToLowerAscii(uri);
  if (lowerUri.rfind("data:image/gif", 0) == 0) {
    return true;
  }
  return false;
}

static ImageCacheDownSamplingStrategy MapResizeMethod(
    const std::string& resizeMethod) {
  if (resizeMethod.empty()) {
    return ImageCacheDownSamplingStrategy::DEFAULT;
  }
  if (resizeMethod == "auto") {
    return ImageCacheDownSamplingStrategy::DEFAULT;
  }
  if (resizeMethod == "scale" || resizeMethod == "none") {
    return ImageCacheDownSamplingStrategy::NONE;
  }
  if (resizeMethod == "resize") {
    return ImageCacheDownSamplingStrategy::FIT_CENTER_MEMORY;
  }
  if (resizeMethod == "contain") {
    return ImageCacheDownSamplingStrategy::CENTER_INSIDE_MEMORY;
  }
  if (resizeMethod == "cover") {
    return ImageCacheDownSamplingStrategy::FIT_CENTER_QUALITY;
  }
  if (resizeMethod == "centerInside") {
    return ImageCacheDownSamplingStrategy::CENTER_INSIDE_QUALITY;
  }
  if (resizeMethod == "atLeast") {
    return ImageCacheDownSamplingStrategy::AT_LEAST;
  }
  if (resizeMethod == "atMost") {
    return ImageCacheDownSamplingStrategy::AT_MOST;
  }
  return ImageCacheDownSamplingStrategy::DEFAULT;
}

static std::string BuildTransformSpec(float blurRadius) {
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  if (blurRadius > 0.0f) {
    return "BLUR:" + std::to_string(static_cast<int>(blurRadius + 0.5f));
  }
#else
  (void)blurRadius;
#endif
  return {};
}

struct MaskSpec {
  std::string uri;
  std::string signature;
};

static MaskSpec ParseMaskSpec(const std::string& valuePart) {
  MaskSpec result;
  size_t separatorPos = std::string::npos;
  for (size_t i = 0; i < valuePart.size(); ++i) {
    if (valuePart[i] == ':') {
      if (i + 2 < valuePart.size() && valuePart[i + 1] == '/' &&
          valuePart[i + 2] == '/') {
        i += 2;
        continue;
      }
      if (i >= 4 && ToLowerAscii(valuePart.substr(i - 4, 4)) == "data") {
        continue;
      }
      separatorPos = i;
      break;
    }
  }
  if (separatorPos != std::string::npos) {
    result.uri = TrimAscii(valuePart.substr(0, separatorPos));
    result.signature = TrimAscii(valuePart.substr(separatorPos + 1));
  } else {
    result.uri = TrimAscii(valuePart);
  }
  return result;
}

#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
void ImageCacheOption::ApplyTransformSpec(const std::string& spec) {
  if (spec.empty()) {
    return;
  }

  static const std::unordered_map<std::string, ImageCacheTransformationType>
      typeMap = {
          {"BLUR", ImageCacheTransformationType::BLUR},
          {"GAUSSIAN_BLUR", ImageCacheTransformationType::GAUSSIAN_BLUR},
          {"BRIGHTEN", ImageCacheTransformationType::BRIGHTEN},
          {"GRAY_SCALE", ImageCacheTransformationType::GRAY_SCALE},
          {"INVERT", ImageCacheTransformationType::INVERT},
          {"KUWAHARA", ImageCacheTransformationType::KUWAHARA},
          {"PIXELATION", ImageCacheTransformationType::PIXELATION},
          {"SEPIA", ImageCacheTransformationType::SEPIA},
          {"SKETCH", ImageCacheTransformationType::SKETCH},
          {"SWIRL", ImageCacheTransformationType::SWIRL},
          {"TOON", ImageCacheTransformationType::TOON},
          {"VIGNETTE", ImageCacheTransformationType::VIGNETTE},
          {"CROP_TOP", ImageCacheTransformationType::CROP_TOP},
          {"CROP_CENTER", ImageCacheTransformationType::CROP_CENTER},
          {"CROP_BOTTOM", ImageCacheTransformationType::CROP_BOTTOM},
          {"CROP_SQUARE", ImageCacheTransformationType::CROP_SQUARE},
          {"CROP_CIRCLE", ImageCacheTransformationType::CROP_CIRCLE},
          {"CROP_CIRCLE_WITH_BORDER",
           ImageCacheTransformationType::CROP_CIRCLE_WITH_BORDER},
          {"MASK", ImageCacheTransformationType::MASK},
          {"MIRROR", ImageCacheTransformationType::MIRROR},
      };

  std::vector<std::shared_ptr<ImageCacheTransformation>> transforms;
  for (const auto& segmentRaw : SplitByChar(spec, '+')) {
    const auto colonParts = SplitByChar(segmentRaw, ':');
    if (colonParts.empty()) {
      continue;
    }
    const auto typeName = ToUpperAscii(colonParts[0]);
    auto typeIt = typeMap.find(typeName);
    if (typeIt == typeMap.end()) {
      continue;
    }

    auto transformOption = std::make_shared<ImageCacheTransformationOption>();
    transformOption->type = typeIt->second;
    if (typeIt->second == ImageCacheTransformationType::MASK) {
      auto valuePart = segmentRaw.substr(typeName.size() + 1);
      auto maskSpec = ParseMaskSpec(valuePart);
      transformOption->imageSrc.SetString(maskSpec.uri);
      transformOption->signature = maskSpec.signature;
    } else {
      for (size_t i = 1; i < colonParts.size(); ++i) {
        try {
          transformOption->f32.push_back(std::stof(colonParts[i]));
        } catch (const std::exception& e) {
          DLOG(WARNING) << "ImageCache: failed to parse transform parameter '"
                        << colonParts[i] << "': " << e.what();
        }
      }
    }
    auto transform =
        ImageCacheTransformationFactory::CreateTransformation(transformOption);
    if (transform != nullptr) {
      transforms.push_back(std::move(transform));
    }
  }

  if (transforms.empty()) {
    return;
  }
  if (transforms.size() == 1) {
    transformation = transforms.front();
    return;
  }
  multiTransformation =
      std::make_shared<ImageCacheMultiTransformation>(std::move(transforms));
}
#endif

std::shared_ptr<ImageCacheOption> ImageCacheOption::Create(
    const std::string& uri,
    const std::string& assetsPrefix,
    const std::string& resizeMethod,
    uint32_t requestWidth,
    uint32_t requestHeight,
    float blurRadius,
    ResourceManager_ColorMode colorMode,
    ImageCacheOrientation orientation,
    ImageCacheDynamicRange dynamicRange,
    float resizeMultiplier) {
  auto option = std::make_shared<ImageCacheOption>();
  option->loadSrc.SetString(uri);
  option->assetsPrefix = assetsPrefix;
  option->isAnimator = IsAnimatorUri(uri);
  option->downSampling = MapResizeMethod(resizeMethod);
  option->requestImageSize = {requestWidth, requestHeight};
  option->transformSpec = BuildTransformSpec(blurRadius);
  option->colorMode = colorMode;
  option->orientation = orientation;
  option->dynamicRange = dynamicRange;
  option->resizeMultiplier = resizeMultiplier;
  return option;
}

ImageCacheOption::ImageCacheOption(const ImageCacheOption& other)
    : loadSrc(other.loadSrc),
      signature(other.signature),
      transformationInfo(other.transformationInfo),
      downSampling(other.downSampling),
      requestImageSize(other.requestImageSize),
      isAnimator(other.isAnimator),
      animationDecodeMode(other.animationDecodeMode),
      assetsPrefix(other.assetsPrefix),
      colorMode(other.colorMode),
      orientation(other.orientation),
      dynamicRange(other.dynamicRange),
      onlyRetrieveFromCache(other.onlyRetrieveFromCache),
      resizeMultiplier(other.resizeMultiplier),
      transformSpec(other.transformSpec) {
  ImageCacheMetrics::Instance().Objects().Increment("ImageCacheOption", this);
}

void ImageCacheOption::PrepareTransformations() {
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  if (transformSpec.empty()) {
    return;
  }
  if (transformation != nullptr || multiTransformation != nullptr) {
    return;
  }
  ApplyTransformSpec(transformSpec);
#endif
}

std::string ImageCacheOption::ResolveTransformationInfo() const {
  if (!transformationInfo.empty()) {
    return transformationInfo;
  }
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  if (multiTransformation != nullptr) {
    return multiTransformation->GetTransformInfo();
  }
  if (transformation != nullptr) {
    return transformation->GetTransformInfo();
  }
#endif

  if (!transformSpec.empty()) {
    return "transformSpec=" + transformSpec + ";";
  }
  return std::string();
}

} // namespace ImageCachePro
