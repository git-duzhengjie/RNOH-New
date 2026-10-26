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

#ifndef IMAGE_CACHE_DATA_H
#define IMAGE_CACHE_DATA_H

#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include "ImageCacheMetrics.h"
#include "ImageCacheType.h"

namespace ImageCachePro {
class ImageSourceNativeWrapper;

struct ImageCacheData {
 public:
  explicit ImageCacheData(OH_PixelmapNative* pixelmap);
  ImageCacheData(
      std::unique_ptr<OH_PixelmapNative*[]> pixelmapList,
      std::unique_ptr<int32_t[]> delayTimeList,
      size_t frameCount);

  OH_PixelmapNative* GetPixelmap() const;
  OH_PixelmapNative* GetPixelmapAt(size_t index) const;
  OH_PixelmapNative** GetPixelmapList() const;
  int32_t* GetDelayTimeList() const;
  uint32_t GetFrameCount() const;
  Image_Size GetImageSize() const;

  ~ImageCacheData();
  ImageCacheData(const ImageCacheData&) = delete;
  ImageCacheData& operator=(const ImageCacheData&) = delete;
  ImageCacheData(ImageCacheData&&) = delete;
  ImageCacheData& operator=(ImageCacheData&&) = delete;

 private:
  std::unique_ptr<OH_PixelmapNative*[]> pixelmapList_;
  std::unique_ptr<int32_t[]> delayTimeList_;
  uint32_t frameCount_ = 0;
};

struct ImageCacheFileTypeInfo {
  ImageCacheFormat format = ImageCacheFormat::UNKNOWN;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t frameCount = 1;
  int32_t pixelFormat = PIXEL_FORMAT_RGBA_8888;
};

struct FrameDecodeContext {
  ImageCacheFileTypeInfo fileTypeInfo_{};
  Image_Size desiredImageSize_ = {0, 0};
  std::shared_ptr<ImageSourceNativeWrapper> imageSourceNative_;

  uint32_t frameCount_ = 0;
  std::unique_ptr<int32_t[]> animationDelayTimeList_ = nullptr;

  FrameDecodeContext() {
    ImageCacheMetrics::Instance().Objects().Increment(
        "FrameDecodeContext", this);
  }
  ~FrameDecodeContext() {
    ImageCacheMetrics::Instance().Objects().Decrement(
        "FrameDecodeContext", this);
  }
  FrameDecodeContext(const FrameDecodeContext&) = delete;
  FrameDecodeContext& operator=(const FrameDecodeContext&) = delete;
};

struct FrameDecodeRequest {
  int startFrame{0};
  int fetchCount{1};
  std::shared_ptr<FrameDecodeContext> frameContext;
  bool IsBatch() const {
    return fetchCount > 1;
  }
};

struct ImageCacheLoadData {
  ImageCacheLoadDataType type = ImageCacheLoadDataType::NONE;
  std::vector<std::shared_ptr<ImageCacheData>> imageDataList;
  std::shared_ptr<FrameDecodeContext> frameContext;

  bool IsNone() const {
    return type == ImageCacheLoadDataType::NONE;
  }

  bool IsImageData() const {
    return type == ImageCacheLoadDataType::IMAGE_DATA;
  }

  bool IsFrameDecodeData() const {
    return type == ImageCacheLoadDataType::FRAME_DECODE_DATA;
  }

  bool HasFrameContext() const {
    return frameContext != nullptr;
  }

  std::shared_ptr<ImageCacheData> GetImageData() const {
    return imageDataList.empty() ? nullptr : imageDataList.front();
  }

  const std::vector<std::shared_ptr<ImageCacheData>>& GetImageDataList() const {
    return imageDataList;
  }

  std::shared_ptr<FrameDecodeContext> GetFrameContext() const {
    return frameContext;
  }

  static ImageCacheLoadData None() {
    return ImageCacheLoadData{};
  }

  static ImageCacheLoadData From(
      std::vector<std::shared_ptr<ImageCacheData>> dataList,
      std::shared_ptr<FrameDecodeContext> context = nullptr) {
    ImageCacheLoadData rd;
    rd.type = context ? ImageCacheLoadDataType::FRAME_DECODE_DATA
                      : ImageCacheLoadDataType::IMAGE_DATA;
    rd.imageDataList = std::move(dataList);
    rd.frameContext = std::move(context);
    return rd;
  }
};

/** GetCacheImage asynchronous status */
struct ImageCacheLoadResult {
  ImageCacheErrorCode code = ImageCacheErrorCode::SUCCESS;
  uint32_t frameIndex = 0;

  bool IsSuccess() const {
    return code == ImageCacheErrorCode::SUCCESS;
  }

  uint32_t GetFrameIndex() const {
    return frameIndex;
  }

  static ImageCacheLoadResult Ok(uint32_t frameIndex = 0) {
    ImageCacheLoadResult result;
    result.frameIndex = frameIndex;
    return result;
  }

  static ImageCacheLoadResult Failure(ImageCacheErrorCode failedCode) {
    ImageCacheLoadResult result;
    result.code = failedCode;
    return result;
  }
};

using ImageCacheGetCacheCompleteCallback =
    std::function<void(ImageCacheLoadData data, ImageCacheLoadResult result)>;

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DATA_H
