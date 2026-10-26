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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_CIRCLE_WITH_BORDER_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_CIRCLE_WITH_BORDER_H

#include <cmath>
#include "ImageCacheTransformationOption.h"
#include "transformation_base.h"

namespace ImageCachePro {
class TransformationCropCircleWithBorder : public TransformationBase {
 public:
  explicit TransformationCropCircleWithBorder(
      std::shared_ptr<ImageCacheTransformationOption> option) {
    const size_t argc = 4;
    const uint8_t defaultColor = 128;
    const float defaultBorderWidth = 5;
    const size_t greenIndex = 2;
    const size_t blueIndex = 3;

    if (option->f32.size() >= argc) {
      borderWidth_ = option->f32[0] > 0 ? option->f32[0] : 1;
      red_ = option->f32[1];
      green_ = option->f32[greenIndex];
      blue_ = option->f32[blueIndex];
    } else {
      borderWidth_ = defaultBorderWidth;
      red_ = defaultColor;
      green_ = defaultColor;
      blue_ = defaultColor;
    }
  }

  std::string GetTransformInfo() override {
    return "Default Crop Circle With Border:" + std::to_string(borderWidth_) +
        "," + std::to_string(red_) + "," + std::to_string(green_) + "," +
        std::to_string(blue_);
  }

  virtual std::string Process(
      OH_PixelmapNative* pixelmapIn,
      OH_PixelmapNative** pixelmapOut) override {
    info_ = ParsePixelmapInfoReadPixels(pixelmapIn);
    if (info_.innerBuffer == nullptr) {
      return "Read Pixels Failed";
    }

    const uint32_t centerX = info_.width / 2;
    const uint32_t centerY = info_.height / 2;
    const uint32_t radius = centerX > centerY ? centerY : centerX;
    const uint32_t radiusPow2 = radius * radius;
    const uint32_t radiusBorder = radius - borderWidth_;
    const uint32_t radiusBorderPow2 = radiusBorder * radiusBorder;
    const uint32_t paddingY = centerY - radius;
    centerY_ = centerY;

    std::vector<std::pair<uint32_t, uint32_t>> rowBounds;
    std::vector<std::pair<uint32_t, uint32_t>> rowBorderBounds;
    for (int y = 0; y <= centerY_; y++) {
      if (y < paddingY) {
        rowBounds.push_back(std::make_pair(info_.width - 1, info_.width));
        rowBorderBounds.push_back(std::make_pair(0, info_.width));
        continue;
      }
      uint32_t dy = centerY_ - y;
      uint32_t dyPow2 = dy * dy;
      float dxMax = sqrt(radiusPow2 - dyPow2);
      uint32_t xLeft = ceil(centerX - dxMax);
      uint32_t xRight = floor(centerX + dxMax);
      rowBounds.push_back(std::make_pair(xLeft, xRight));

      dxMax = sqrt(radiusBorderPow2 - dyPow2);
      xLeft = ceil(centerX - dxMax);
      xRight = floor(centerX + dxMax);
      rowBorderBounds.push_back(std::make_pair(xLeft, xRight));
    }

    Crop(rowBounds, rowBorderBounds, paddingY);

    CreatePixelmapWithBuffer(info_.innerBuffer, info_, pixelmapOut);
    free(info_.innerBuffer);
    return std::string();
  }

 private:
  PixemapInfo info_;
  uint32_t centerY_;

  uint8_t red_;
  uint8_t green_;
  uint8_t blue_;

  float borderWidth_;

  void Crop(
      std::vector<std::pair<uint32_t, uint32_t>>& rowBounds,
      std::vector<std::pair<uint32_t, uint32_t>>& rowBorderBounds,
      const uint32_t paddingY) {
    for (int y = 0; y <= centerY_; y++) {
      auto row = rowBounds[y];
      auto rowBorder = rowBorderBounds[y];
      for (int x = 0; x <= row.first; x++) {
        SetSymmetricPixels(x, y, false);
      }
      for (int x = row.first + 1; x <= rowBorder.first; x++) {
        SetSymmetricPixels(x, y, true);
      }
      for (int x = info_.width - 1; x >= row.second; x--) {
        SetSymmetricPixels(x, y, false);
      }
      for (int x = row.second - 1; x >= rowBorder.second; x--) {
        SetSymmetricPixels(x, y, true);
      }
      if (y >= paddingY && y < paddingY + borderWidth_) {
        for (int x = row.first; x <= row.second; x++) {
          SetSymmetricPixels(x, y, true);
        }
      }
    }
  }

  void SetSymmetricPixels(int x, int y, bool borderColor) {
    if (x >= info_.width) {
      return;
    }
    SetPixel(x, y, borderColor);

    y = info_.height - 1 - y;
    if (y > centerY_) {
      SetPixel(x, y, borderColor);
    }
  }

  void SetPixel(int x, int y, bool borderColor) {
    const int dimensionality = 4;
    int index = (y * info_.width + x) * dimensionality;
    if (borderColor) {
      const int blueIndex = 2;
      const int alphaIndex = 3;
      const int alphaValue = 255;
      info_.innerBuffer[index] = red_;
      info_.innerBuffer[index + 1] = green_;
      info_.innerBuffer[index + blueIndex] = blue_;
      info_.innerBuffer[index + alphaIndex] = alphaValue;
    } else {
      for (int i = 0; i < dimensionality; i++) {
        info_.innerBuffer[index + i] = 0;
      }
    }
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_CIRCLE_WITH_BORDER_H
