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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_TOON_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_TOON_H

#include "ImageCacheTransformationOption.h"
#include "gpu_toon_filter.h"
#include "transformation_gpu.h"

namespace ImageCachePro {
class TransformationToon : public TransformationGPU {
 public:
  explicit TransformationToon(
      std::shared_ptr<ImageCacheTransformationOption> option) {
    const size_t argc = 2;
    const float defaultThreshold = 0.2;
    const float defaultQuantizationLevels = 4;
    float threshold = defaultThreshold;
    float quantizationLevels = defaultQuantizationLevels;

    if (option->f32.size() >= argc) {
      threshold = option->f32[0];
      quantizationLevels = option->f32[1];
    }

    info_ = "Default Toon:" + std::to_string(threshold) + "," +
        std::to_string(quantizationLevels);
    SetGpuFilter(
        std::make_shared<GPUToonFilter>(threshold, quantizationLevels));
  }

 private:
  std::string info_;

  std::string GetTransformInfo() override {
    return info_;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_TOON_H
