/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CONSTRUCTOR_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CONSTRUCTOR_H

#include "ImageCacheTransformation.h"
#include "ImageCacheTransformationOption.h"

namespace ImageCachePro {
/**
 * @brief Base class for image transformation object constructors.
 *
 * This constructor can be used to create specific transformation objects
 * using transformation parameters from the ArkTS layer.
 * For custom transformations that do not require parameters, you can
 * directly register the transformation object.
 */
class ImageCacheTransformationConstructor {
 public:
  /**
   * @brief Create a default transformation object.
   * @param option: Default transformation description object, parsed from ArkTS layer ImageCacheTransformationOption.
   * @return Returns a smart pointer to the base class of the default transformation object.
   */
  virtual std::shared_ptr<ImageCacheTransformation> CreateTransformation(
      std::shared_ptr<ImageCacheTransformationOption> option) = 0;

  virtual ~ImageCacheTransformationConstructor() = default;
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CONSTRUCTOR_H
