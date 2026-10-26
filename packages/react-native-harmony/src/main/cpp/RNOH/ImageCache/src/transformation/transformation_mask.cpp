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

#include "transformation_mask.h"
#include "ImageCacheDispatcher.h"
#include "ImageCacheLog.h"
#include "ImageCacheOption.h"
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

TransformationMask::TransformationMask(
    std::shared_ptr<ImageCacheTransformationOption> option)
    : option_(option) {
  IMAGE_CACHE_CHECK(option_->imageSrc.IsEmpty());
}

std::shared_ptr<ImageCacheOption> TransformationMask::CreateTmpOption() {
  auto tmpOption = std::make_shared<ImageCacheOption>();
  tmpOption->signature = option_->signature;
  tmpOption->loadSrc = option_->imageSrc;
  tmpOption->downSamplingInfo = ImageCacheDownSampler::GetDownSamplingInfo(
      {0, 0}, ImageCacheDownSamplingStrategy::DEFAULT);
  return tmpOption;
}

bool TransformationMask::BeforeProcess(std::shared_ptr<ImageCacheTask> task) {
  auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  if (option_->imageSrc.IsEmpty() || taskInternal == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::MASK_UNDEFINED);
    return false;
  }
  auto dispatcher = taskInternal->GetDispatcher();
  if (dispatcher == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::MASK_UNDEFINED);
    return false;
  }
  auto tmpOption = CreateTmpOption();
  if (tmpOption == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::MASK_IMAGE_DATA_EMPTY);
    return false;
  }
  OH_PixelmapNative* maskPixelmap = nullptr;
  maskData_ = dispatcher->LoadImageSync(tmpOption);
  if (maskData_ == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::MASK_IMAGE_DATA_EMPTY);
    return false;
  }
  maskPixelmap = maskData_->GetPixelmap();
  if (maskPixelmap == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::MASK_PIXELMAP_NULL);
    return false;
  }
  maskInfo_ = ParsePixelmapInfoReadPixels(maskPixelmap);
  if (maskInfo_.innerBuffer == nullptr) {
    TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::MASK_READ_PIXELS_FAILED);
    return false;
  }
  return true;
}

std::string TransformationMask::GetTransformInfo() {
  std::string sourceKey;
  if (option_->imageSrc.GetString(sourceKey)) {
    return "Default Mask:" + sourceKey + "," + option_->signature;
  }
  return "Default Mask:" + option_->signature;
}

std::string TransformationMask::Process(
    OH_PixelmapNative* pixelmapIn,
    OH_PixelmapNative** pixelmapOut) {
  OH_PixelmapNative* maskPixelmap = nullptr;
  CreatePixelmapWithBuffer(maskInfo_.innerBuffer, maskInfo_, &maskPixelmap);
  if (maskPixelmap == nullptr) {
    return "Create Mask Pixelmap Failed";
  }

  PixemapInfo info = ParsePixelmapInfoReadPixels(pixelmapIn);
  if (info.innerBuffer == nullptr) {
    OH_PixelmapNative_Release(maskPixelmap);
    return "Read Pixels Failed";
  }

  if (OH_PixelmapNative_Scale(
          maskPixelmap,
          info.width / static_cast<float>(maskInfo_.width),
          info.height / static_cast<float>(maskInfo_.height)) !=
      IMAGE_SUCCESS) {
    free(info.innerBuffer);
    OH_PixelmapNative_Release(maskPixelmap);
    return "Mask Scale Failed";
  }

  PixemapInfo maskInfoScaled = ParsePixelmapInfoReadPixels(maskPixelmap);
  if (maskInfoScaled.innerBuffer == nullptr) {
    free(info.innerBuffer);
    OH_PixelmapNative_Release(maskPixelmap);
    return "Read Scaled Mask Pixels Failed";
  }

  Mask(info, maskInfoScaled);

  CreatePixelmapWithBuffer(info.innerBuffer, info, pixelmapOut);

  free(info.innerBuffer);
  free(maskInfoScaled.innerBuffer);
  OH_PixelmapNative_Release(maskPixelmap);
  return std::string();
}

void TransformationMask::AfterProcess() {
  if (maskInfo_.innerBuffer != nullptr) {
    free(maskInfo_.innerBuffer);
    maskInfo_.innerBuffer = nullptr;
  }
}

void TransformationMask::Mask(PixemapInfo& info, PixemapInfo& maskInfoScaled) {
  const int dimensionality = 4;
  const int blue = 2;
  const int alpha = 3;
  for (uint32_t h = 0; h < info.height; h++) {
    for (uint32_t w = 0; w < info.width; w++) {
      if (h >= maskInfoScaled.height || w >= maskInfoScaled.width) {
        continue;
      }

      int maskIndex = (h * maskInfoScaled.width + w) * dimensionality;
      if (maskInfoScaled.innerBuffer[maskIndex + alpha] == 0) {
        continue;
      }

      int index = (h * info.width + w) * dimensionality;
      info.innerBuffer[index] = maskInfoScaled.innerBuffer[maskIndex];
      info.innerBuffer[index + 1] = maskInfoScaled.innerBuffer[maskIndex + 1];
      info.innerBuffer[index + blue] =
          maskInfoScaled.innerBuffer[maskIndex + blue];
      info.innerBuffer[index + alpha] =
          maskInfoScaled.innerBuffer[maskIndex + alpha];
    }
  }
}

} // namespace ImageCachePro
