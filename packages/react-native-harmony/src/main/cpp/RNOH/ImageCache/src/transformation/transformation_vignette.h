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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_VIGNETTE_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_VIGNETTE_H

#include "ImageCacheTransformationOption.h"
#include "gpu_vignette_filter.h"
#include "transformation_gpu.h"

namespace ImageCachePro {
class TransformationVignette : public TransformationGPU {
 public:
  explicit TransformationVignette(
      std::shared_ptr<ImageCacheTransformationOption> option) {
    const size_t argc = 7;
    const float defaultCenter = 0.5;
    const float defaultColor = 30;
    const float defaultColorRed = 90;
    const float defaultStart = 0.23;
    const float defaultEnd = 4;

    float centerX = defaultCenter;
    float centerY = defaultCenter;
    float red = defaultColorRed;
    float green = defaultColor;
    float blue = defaultColor;
    float start = defaultStart;
    float end = defaultEnd;

    const size_t redIndex = 2;
    const size_t greenIndex = 3;
    const size_t blueIndex = 4;
    const size_t startIndex = 5;
    const size_t endIndex = 6;

    if (option->f32.size() >= argc) {
      centerX = option->f32[0];
      centerY = option->f32[1];
      red = option->f32[redIndex];
      green = option->f32[greenIndex];
      blue = option->f32[blueIndex];
      start = option->f32[startIndex];
      end = option->f32[endIndex];
    }

    info_ = "Default Vignette:" + std::to_string(centerX) + "," +
        std::to_string(centerY) + "," + std::to_string(red) + "," +
        std::to_string(green) + "," + std::to_string(blue) + "," +
        std::to_string(start) + "," + std::to_string(end);
    SetGpuFilter(std::make_shared<GPUVignetteFilter>(
        centerX, centerY, red, green, blue, start, end));
  }

 private:
  std::string info_;

  std::string GetTransformInfo() override {
    return info_;
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_VIGNETTE_H
