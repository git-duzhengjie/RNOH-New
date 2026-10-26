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

#include "transformation_gaussian_blur.h"
#include "ImageCacheLog.h"
#include "native_effect/effect_filter.h"
#include <mutex>

namespace ImageCachePro {
TransformationGaussianBlur::TransformationGaussianBlur(
    std::shared_ptr<ImageCacheTransformationOption> option) {
  if (option->f32.size() >= 1) {
    blurRadius_ = option->f32[0];
  }
}

std::string TransformationGaussianBlur::GetTransformInfo() {
  return "Default GaussianBlur:" + std::to_string(blurRadius_);
}

std::string TransformationGaussianBlur::Process(
    OH_PixelmapNative* pixelmapIn,
    OH_PixelmapNative** pixelmapOut) {
  if (pixelmapIn == nullptr) {
    return "Pixelmap In is nullptr";
  }

  OH_Filter* filter = nullptr;
  EffectErrorCode errorCode = OH_Filter_CreateEffect(pixelmapIn, &filter);
  if (errorCode != EFFECT_SUCCESS) {
    return "Create Effect Failed:" + std::to_string(errorCode);
  }

  errorCode = OH_Filter_Blur(filter, blurRadius_);
  if (errorCode != EFFECT_SUCCESS) {
    OH_Filter_Release(filter);
    return "Add Effect blur Failed:" + std::to_string(errorCode);
  }

  // Only this one effect transformation is used.
  // The effect transformation interface is not thread-safe, so locking is required.
  static std::mutex lock;
  {
    std::lock_guard<std::mutex> guard(lock);
    errorCode = OH_Filter_GetEffectPixelMap(filter, pixelmapOut);
  }
  OH_Filter_Release(filter);
  if (errorCode != EFFECT_SUCCESS) {
    return "Get Effect pixelmap Failed:" + std::to_string(errorCode);
  }

  return std::string();
}
} // namespace ImageCachePro
