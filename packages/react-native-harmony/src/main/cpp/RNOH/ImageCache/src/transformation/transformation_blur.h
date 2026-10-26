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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BLUR_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BLUR_H

#include "ImageCacheTransformationOption.h"
#include "gpu_blur_filter.h"
#include "transformation_gpu.h"

namespace ImageCachePro {

class TransformationBlur : public TransformationGPU {
 public:
  explicit TransformationBlur(
      std::shared_ptr<ImageCacheTransformationOption> option) {
    const int defaultRadius = 10;
    const float defaultOffset = 0.003f;
    const int offsetIndexY = 2;
    const int argSize = 3;

    int radius = defaultRadius;
    float offsetX = defaultOffset;
    float offsetY = defaultOffset;
    if (option->f32.size() >= 1) {
      radius = option->f32[0];
    }
    // blur optional offset parameter
    if (option->f32.size() >= argSize) {
      offsetX = option->f32[1];
      offsetY = option->f32[offsetIndexY];
    }

    info_ = "Default Blur:" + std::to_string(radius);
    SetGpuFilter(std::make_shared<GPUBlurFilter>(radius, offsetX, offsetY));
  }

 private:
  std::string info_;

  std::string GetTransformInfo() override {
    return info_;
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BLUR_H
