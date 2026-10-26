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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_MASK_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_MASK_H

#include <memory>
#include <string>
#include "ImageCacheDownSampler.h"
#include "ImageCacheTransformationOption.h"
#include "transformation_base.h"

namespace ImageCachePro {
class TransformationMask : public TransformationBase {
 public:
  explicit TransformationMask(
      std::shared_ptr<ImageCacheTransformationOption> option);

  std::shared_ptr<ImageCacheOption> CreateTmpOption();

  bool BeforeProcess(std::shared_ptr<ImageCacheTask> task) override;

  std::string GetTransformInfo() override;

  std::string Process(
      OH_PixelmapNative* pixelmapIn,
      OH_PixelmapNative** pixelmapOut) override;

  void AfterProcess() override;

 private:
  std::shared_ptr<ImageCacheTransformationOption> option_;
  std::shared_ptr<ImageCacheData> maskData_ = nullptr;
  PixemapInfo maskInfo_;
  void Mask(PixemapInfo& info, PixemapInfo& maskInfoScaled);
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_MASK_H
