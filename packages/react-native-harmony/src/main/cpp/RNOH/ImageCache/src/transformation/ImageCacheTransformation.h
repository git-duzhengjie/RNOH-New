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

#ifndef IMAGE_KNIFE_C_TRANSFORMATION_H
#define IMAGE_KNIFE_C_TRANSFORMATION_H

#include <memory>
#include <string>
#include <vector>
#include "ImageCacheTask.h"

namespace ImageCachePro {

/**
 * @brief Base class for image transformations.
 * Custom image transformations should inherit from this class and implement its virtual functions.
 */
class ImageCacheTransformation {
 public:
  /**
   * @brief Perform image transformation.
   * Custom transformations must implement this pure virtual function.
   * @param task: Smart pointer to the image loading task object.
   * The bitmap is obtained via task->product.imageDataList.
   * Replace task->product.imageDataList with the transformed data
   * to complete the transformation.
   * @return Returns true to indicate the transformation will continue
   * (if there are more transform objects in multiTransformation).
   * Returns false to terminate the transformation process
   * for this image loading task.
   */
  virtual bool Transform(std::shared_ptr<ImageCacheTask> task) = 0;
  /**
   * @brief Get the transformation information, used for generating memory cache keys.
   * Custom transformations must implement this pure virtual function.
   * The transformation information should be distinguishable from other transformation objects,
   * including objects of the same class with different parameters.
   * Default transformation objects obtain information prefixed with "Default".
   * @return Returns the transformation information string.
   */
  virtual std::string GetTransformInfo() = 0;
  virtual ~ImageCacheTransformation() = default;
};

/**
 * @brief Multiple image transformation effects.
 * Transformation effects are applied sequentially, one after another.
 */
class ImageCacheMultiTransformation {
 public:
  void Transform(std::shared_ptr<ImageCacheTask> task);

  /**
   * @brief Get the information for multiple transformations, used for generating memory cache keys.
   * @return Information from all contained transformations.
   */
  std::string GetTransformInfo();

  /**
   * @brief Construct a multi-transformation object using an array of transformations.
   * @param transformations: Array of transformations.
   */
  ImageCacheMultiTransformation(
      std::vector<std::shared_ptr<ImageCacheTransformation>> transformations);

 private:
  std::vector<std::shared_ptr<ImageCacheTransformation>> transformations_;
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_TRANSFORMATION_H
