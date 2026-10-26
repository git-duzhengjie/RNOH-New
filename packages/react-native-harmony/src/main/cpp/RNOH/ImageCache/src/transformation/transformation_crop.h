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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_H

#include "ImageCacheTransformationOption.h"
#include "transformation_base.h"

namespace ImageCachePro {
class TransformationCrop : public TransformationBase {
 public:
  explicit TransformationCrop(
      std::shared_ptr<ImageCacheTransformationOption> option)
      : cropType_(option->type) {
    const size_t argc = 2;
    if (option->f32.size() >= argc) {
      cropWidth_ = option->f32[0];
      cropHeight_ = option->f32[1];
    } else {
      cropWidth_ = 0;
      cropHeight_ = 0;
    }
  }

  std::string GetTransformInfo() override {
    return "Default Crop:" + std::to_string(static_cast<int>(cropType_)) + "," +
        std::to_string(cropWidth_) + "," + std::to_string(cropHeight_);
  }

  virtual std::string Process(
      OH_PixelmapNative* pixelmapIn,
      OH_PixelmapNative** pixelmapOut) override {
    PixemapInfo info = ParsePixelmapInfoReadPixels(pixelmapIn, false);
    if (info.bufferLength == 0) {
      return "Get Pixelmap Info Failed";
    }

    float cropWidth = cropWidth_ == 0 ? info.width : cropWidth_;
    float cropHeight = cropHeight_ == 0 ? info.height / 3 : cropHeight_;

    float scaleX = cropWidth / info.width;
    float scaleY = cropHeight / info.height;
    float scale = scaleX > scaleY ? scaleX : scaleY;
    uint32_t scaledWidth = scale * info.width;
    uint32_t scaledHeight = scale * info.height;

    const float left = (cropWidth - scaledWidth) / 2;
    const float top = GetTop(cropHeight - scaledHeight);

    OH_PixelmapNative_Scale(pixelmapIn, scale, scale);

    Image_Region region = {
        .width = static_cast<uint32_t>(cropWidth),
        .height = static_cast<uint32_t>(cropHeight)};
    region.x = left < 0 ? 0 : left;
    region.y = top < 0 ? 0 : top;
    OH_PixelmapNative_Crop(pixelmapIn, &region);

    pixelmapOut = nullptr;
    return std::string();
  }

 private:
  float cropWidth_;
  float cropHeight_;
  ImageCacheTransformationType cropType_;

  float GetTop(float num) {
    const int two = 2;
    switch (cropType_) {
      case ImageCacheTransformationType::CROP_TOP:
        return 0;
      case ImageCacheTransformationType::CROP_CENTER:
        return abs(num / two);
      case ImageCacheTransformationType::CROP_BOTTOM:
        return abs(num);
      default:
        return 0;
    }
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_H
