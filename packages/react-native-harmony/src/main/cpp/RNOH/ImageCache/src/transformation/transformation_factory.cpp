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

#include <functional>
#include <unordered_map>
#include "ImageCacheTransformationFactory.h"
#include "transformation_base.h"
#include "transformation_blur.h"
#include "transformation_brighten.h"
#include "transformation_crop.h"
#include "transformation_crop_circle.h"
#include "transformation_crop_circle_with_border.h"
#include "transformation_crop_square.h"
#include "transformation_gaussian_blur.h"
#include "transformation_gray_scale.h"
#include "transformation_invert.h"
#include "transformation_kuwahara.h"
#include "transformation_mask.h"
#include "transformation_mirror.h"
#include "transformation_pixelation.h"
#include "transformation_sepia.h"
#include "transformation_sketch.h"
#include "transformation_swirl.h"
#include "transformation_toon.h"
#include "transformation_vignette.h"

namespace ImageCachePro {

typedef std::shared_ptr<ImageCacheTransformation> TransformPtr;
typedef std::shared_ptr<ImageCacheTransformationOption> OptionPtr;

TransformPtr ImageCacheTransformationFactory::CreateTransformation(
    OptionPtr option) {
  IMAGE_CACHE_CHECK_RETURN(option == nullptr, nullptr);
  typedef std::function<TransformPtr(OptionPtr)> Builder;
  static std::unordered_map<ImageCacheTransformationType, Builder> builderMap =
      {
          {ImageCacheTransformationType::UNKNOWN,
           [](OptionPtr option) -> TransformPtr { return nullptr; }},
          {ImageCacheTransformationType::BLUR,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationBlur>(option);
           }},
          {ImageCacheTransformationType::GAUSSIAN_BLUR,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationGaussianBlur>(option);
           }},
          {ImageCacheTransformationType::BRIGHTEN,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationBrighten>(option);
           }},
          {ImageCacheTransformationType::GRAY_SCALE,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationGrayScale>();
           }},
          {ImageCacheTransformationType::INVERT,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationInvert>();
           }},
          {ImageCacheTransformationType::KUWAHARA,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationKuwahara>(option);
           }},
          {ImageCacheTransformationType::PIXELATION,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationPixelation>(option);
           }},
          {ImageCacheTransformationType::SEPIA,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationSepia>(option);
           }},
          {ImageCacheTransformationType::SKETCH,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationSketch>();
           }},
          {ImageCacheTransformationType::SWIRL,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationSwirl>(option);
           }},
          {ImageCacheTransformationType::TOON,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationToon>(option);
           }},
          {ImageCacheTransformationType::VIGNETTE,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationVignette>(option);
           }},
          {ImageCacheTransformationType::CROP_SQUARE,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationCropSquare>();
           }},
          {ImageCacheTransformationType::CROP_CIRCLE,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationCropCircle>();
           }},
          {ImageCacheTransformationType::CROP_CIRCLE_WITH_BORDER,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationCropCircleWithBorder>(
                 option);
           }},
          {ImageCacheTransformationType::MASK,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationMask>(option);
           }},
          {ImageCacheTransformationType::CROP_TOP,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationCrop>(option);
           }},
          {ImageCacheTransformationType::CROP_CENTER,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationCrop>(option);
           }},
          {ImageCacheTransformationType::CROP_BOTTOM,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationCrop>(option);
           }},
          {ImageCacheTransformationType::MIRROR,
           [](OptionPtr option) -> TransformPtr {
             return std::make_shared<TransformationMirror>();
           }},
      };

  IMAGE_CACHE_CHECK_RETURN(
      builderMap.find(option->type) == builderMap.end(), nullptr);
  auto func = builderMap[option->type];
  return func(option);
}
} // namespace ImageCachePro
