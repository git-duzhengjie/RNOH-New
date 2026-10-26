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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_SQUARE_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_SQUARE_H

#include "transformation_base.h"
namespace ImageCachePro {
class TransformationCropSquare : public TransformationBase {
 public:
  std::string GetTransformInfo() override {
    return "Default Crop Square";
  }

  virtual std::string Process(
      OH_PixelmapNative* pixelmapIn,
      OH_PixelmapNative** pixelmapOut) override {
    IMAGE_CACHE_CHECK_RETURN(pixelmapIn == nullptr, "pixelmap is nullptr");
    OH_Pixelmap_ImageInfo* imageInfo = nullptr;
    Image_ErrorCode errorCode = OH_PixelmapImageInfo_Create(&imageInfo);
    if (errorCode != IMAGE_SUCCESS) {
      return "Create ImageInfo Failed:" + std::to_string(errorCode);
    }
    uint32_t width = 0;
    uint32_t height = 0;
    OH_PixelmapNative_GetImageInfo(pixelmapIn, imageInfo);
    OH_PixelmapImageInfo_GetWidth(imageInfo, &width);
    OH_PixelmapImageInfo_GetHeight(imageInfo, &height);
    OH_PixelmapImageInfo_Release(imageInfo);
    IMAGE_CACHE_CHECK_RETURN(
        width <= 0 || height <= 0, "Get pixelmap size failed");

    uint32_t squareSize = width > height ? height : width;
    Image_Region region = {
        .x = width / 2 - squareSize / 2,
        .y = height / 2 - squareSize / 2,
        .width = squareSize,
        .height = squareSize};
    errorCode = OH_PixelmapNative_Crop(pixelmapIn, &region);
    if (errorCode != IMAGE_SUCCESS) {
      return "pixelmap Crop failed:" + std::to_string(errorCode);
    }
    pixelmapOut = nullptr;
    return std::string();
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_SQUARE_H
