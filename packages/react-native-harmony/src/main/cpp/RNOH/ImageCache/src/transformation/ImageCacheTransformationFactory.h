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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_FACTORY_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_FACTORY_H

#include "ImageCacheTransformation.h"
#include "ImageCacheTransformationOption.h"

namespace ImageCachePro {
/**
 * @brief Image transformation object construction factory.
 *
 * Use this factory to create default transformation objects.
 * Custom transformation object pointers that inherit from the Transformation base class
 * can be used directly without needing this factory.
 */
class ImageCacheTransformationFactory {
 public:
  /**
   * @brief Create a default transformation object.
   * @param option: Default transformation description object.
   * @return Returns a smart pointer to the base class of the default transformation object.
   */
  static std::shared_ptr<ImageCacheTransformation> CreateTransformation(
      std::shared_ptr<ImageCacheTransformationOption> option);
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_FACTORY_H
