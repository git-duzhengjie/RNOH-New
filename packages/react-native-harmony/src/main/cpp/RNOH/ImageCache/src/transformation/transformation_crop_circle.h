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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_CIRCLE_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_CIRCLE_H

#include <cmath>
#include "transformation_base.h"
namespace ImageCachePro {
class TransformationCropCircle : public TransformationBase {
 public:
  std::string GetTransformInfo() override {
    return "Default Crop Circle";
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
    const uint32_t paddingY = centerY - radius;
    centerY_ = centerY;

    // Precompute left and right bounds for each row (top-left quadrant)
    std::vector<std::pair<uint32_t, uint32_t>> rowBounds;
    for (int y = 0; y <= centerY_; y++) {
      if (y < paddingY) {
        rowBounds.push_back(std::make_pair(info_.width - 1, info_.width));
        continue;
      }
      uint32_t dy = centerY_ - y;
      float dxMax = sqrt(radiusPow2 - dy * dy);
      uint32_t xLeft = ceil(centerX - dxMax);
      uint32_t xRight = floor(centerX + dxMax);
      rowBounds.push_back(std::make_pair(xLeft, xRight));
    }

    // Traverse rows with a skipping pattern
    for (int y = 0; y <= centerY_; y++) {
      auto row = rowBounds[y];
      // Process left outer region
      for (int x = 0; x <= row.first; x++) {
        SetSymmetricPixels(x, y);
      }
      // Process right outer region
      for (int x = info_.width - 1; x >= row.second; x--) {
        SetSymmetricPixels(x, y);
      }
    }

    CreatePixelmapWithBuffer(info_.innerBuffer, info_, pixelmapOut);
    free(info_.innerBuffer);
    return std::string();
  }

 private:
  PixemapInfo info_;
  uint32_t centerY_;

  void SetSymmetricPixels(int x, int y) {
    if (x >= info_.width) {
      return;
    }
    SetPixel(x, y);

    y = info_.height - 1 - y;
    if (y > centerY_) {
      SetPixel(x, y);
    }
  }

  void SetPixel(int x, int y) {
    const int dimensionality = 4;
    int index = (y * info_.width + x) * dimensionality;
    for (int i = 0; i < dimensionality; i++) {
      info_.innerBuffer[index + i] = 0;
    }
  }
};

} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_CROP_CIRCLE_H
