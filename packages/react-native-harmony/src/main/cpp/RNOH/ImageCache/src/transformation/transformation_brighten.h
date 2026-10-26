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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BRIGHTEN_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BRIGHTEN_H

#include "ImageCacheTransformationOption.h"
#include "gpu_brighten_filter.h"
#include "transformation_gpu.h"
namespace ImageCachePro {

class TransformationBrighten : public TransformationGPU {
 public:
  explicit TransformationBrighten(
      std::shared_ptr<ImageCacheTransformationOption> option) {
    const float defaultBrightness = 0.25;
    float brightness = defaultBrightness;
    if (option->f32.size() >= 1) {
      brightness = option->f32[0];
    }

    info_ = "Default Brighten:" + std::to_string(brightness);
    SetGpuFilter(std::make_shared<GPUBrightenFilter>(brightness));
  }

 private:
  std::string info_;

  std::string GetTransformInfo() override {
    return info_;
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BRIGHTEN_H
