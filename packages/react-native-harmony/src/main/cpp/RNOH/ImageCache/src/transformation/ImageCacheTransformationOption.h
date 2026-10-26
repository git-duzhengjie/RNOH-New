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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_OPTION_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_OPTION_H

#include <vector>
#include <memory>
#include "ImageCacheSource.h"

namespace ImageCachePro {

/**
 * @brief Default image transformation types.
 */
enum class ImageCacheTransformationType {
  /**
   * @brief Unknown transformation.
   */
  UNKNOWN = 0,
  /**
   * @brief Custom transformation used by ArkTS layer.
   * For C API custom transformations, set the object pointer directly without using this enum.
   */
  CUSTOM_TRANSFORMATION,
  /**
   * @brief Blur effect. Requires parameter f32[0]: blur radius in pixels, default 10.
   * Does not change the original image pixel alpha value. Blur effect is weaker for pixels
   * with transparent backgrounds. Use GAUSSIAN_BLUR for smoother blur.
   * Optional parameters for blur offset affecting blur intensity:
   * f32[1]: X-axis offset, default 0.003
   * f32[2]: Y-axis offset, default 0.003
   */
  BLUR,
  /**
   * @brief Brighten effect. Requires f32 parameter, range 0-1. Value 0 keeps the image unchanged, default 0.25.
   * Values between -1 and 0 produce a darken effect.
   */
  BRIGHTEN,
  /**
   * @brief Grayscale effect.
   */
  GRAY_SCALE,
  /**
   * @brief Invert colors effect.
   */
  INVERT,
  /**
   * @brief Kuwahara filter effect. Requires f32[0] parameter, default 25.
   */
  KUWAHARA,
  /**
   * @brief Pixelation effect. Requires f32[0] parameter, default 10.
   */
  PIXELATION,
  /**
   * @brief Sepia filter effect. Requires f32[0] parameter, default 1.
   */
  SEPIA,
  /**
   * @brief Sketch effect.
   */
  SKETCH,
  /**
   * @brief Swirl effect. Requires f32 parameters:
   * f32[0]: Swirl radius, larger value means larger range, default 200
   * f32[1]: Swirl angle, range 0~1, default 0.9
   * f32[2]: Center X position in image, range 0~1, default 0.5
   * f32[3]: Center Y position in image, range 0~1, default 0.5
   */
  SWIRL,
  /**
   * @brief Toon animation filter effect. Requires f32 parameters:
   * f32[0]: threshold, default 0.2
   * f32[1]: quantization levels, default 4
   */
  TOON,
  /**
   * @brief Vignette effect. Requires f32 parameters:
   * f32[0]: Center X position in image, range 0~1, default 0.5
   * f32[1]: Center Y position in image, range 0~1, default 0.5
   * f32[2]: Red, range 0~255, default 90
   * f32[3]: Green, range 0~255, default 30
   * f32[4]: Blue, range 0~255, default 30
   * f32[5]: Start position, range 0~1, default 0.23
   * f32[6]: End position, default 4
   */
  VIGNETTE,
  /**
   * @brief Scale and crop the top portion of the image based on given width and height. Requires f32 parameters.
   * If width and height are not set, crops the top third of the image.
   * f32[0]: width, default 0 (uses target image width)
   * f32[1]: height, default 0 (uses 1/3 of target image height)
   */
  CROP_TOP,
  /**
   * @brief Scale and crop the center portion of the image based on given width and height. Requires f32 parameters.
   * If width and height are not set, crops the center third of the image.
   * f32[0]: width, default 0 (uses target image width)
   * f32[1]: height, default 0 (uses 1/3 of target image height)
   */
  CROP_CENTER,
  /**
   * @brief Scale and crop the bottom portion of the image based on given width and height. Requires f32 parameters.
   * If width and height are not set, crops the bottom third of the image.
   * f32[0]: width, default 0 (uses target image width)
   * f32[1]: height, default 0 (uses 1/3 of target image height)
   */
  CROP_BOTTOM,
  /**
   * @brief Center crop to a square.
   */
  CROP_SQUARE,
  /**
   * @brief Crop to the largest inscribed circle.
   */
  CROP_CIRCLE,
  /**
   * @brief Crop to the largest inscribed circle with a border. Requires f32 parameters:
   * f32[0]: border width, default 5
   * f32[1]: border color red, default 128
   * f32[2]: border color green, default 128
   * f32[3]: border color blue, default 128
   */
  CROP_CIRCLE_WITH_BORDER,
  /**
   * @brief Mask effect. Requires imageSource parameter for the mask image.
   * Optional parameter: signature, used for mask image loading and Mask transformation description generation.
   * Pixels with non-zero alpha in the mask image will be applied to the corresponding positions on the target image.
   */
  MASK,
  /**
   * @brief Left-right mirror effect.
   */
  MIRROR,
  /**
   * @brief Custom transformation constructor used by ArkTS layer.
   * For C API custom transformations, set the object pointer directly without using this enum.
   * TransformationConstructor allows using ArkTS layer input parameters to construct specific transformation objects.
   */
  CUSTOM_TRANSFORMATION_CONSTRUCTOR,
  /**
   * @brief Gaussian blur effect with smoother blur.
   * Requires parameter f32[0]: blur radius in pixels, default 10.
   */
  GAUSSIAN_BLUR,
};

/**
 * @brief Configuration options for constructing default transformation objects.
 */
struct ImageCacheTransformationOption {
  /**
   * @brief Default transformation type.
   */
  ImageCacheTransformationType type = ImageCacheTransformationType::UNKNOWN;
  /**
   * @brief Float32 type parameters.
   * When the number of parameters does not meet the transformation
   * requirements, default values will be used.
   */
  std::vector<float> f32;
  /**
   * @brief Mask image resource: String, Pixelmap, or Resource.
   */
  ImageCacheSource imageSrc;
  /**
   * @brief Signature information corresponding to imageSrc.
   */
  std::string signature;
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_OPTION_H
