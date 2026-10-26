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

#ifndef IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BASE_H
#define IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BASE_H

#include <cstring>
#include "ImageCacheLog.h"
#include "ImageCacheRequest.h"
#include "ImageCacheTaskInternal.h"
#include "ImageCacheTransformation.h"

namespace ImageCachePro {

class TransformationBase : public ImageCacheTransformation {
 public:
  bool Transform(std::shared_ptr<ImageCacheTask> task) final {
    auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
    IMAGE_CACHE_CHECK_RETURN(taskInternal == nullptr, false);
    if (task->product.imageDataList.empty()) {
      TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::TASK_IMAGE_DATA_NULL);
      return false;
    }

    EndCallback callback;
    callback.func = [this]() { this->AfterProcess(); };
    if (!BeforeProcess(task)) {
      return false;
    }

    std::vector<std::shared_ptr<ImageCacheData>> transformed;
    for (auto& data : task->product.imageDataList) {
      auto result = TransformSingle(taskInternal, data);
      if (result == nullptr || !taskInternal->IsSuccess()) {
        return false;
      }
      transformed.push_back(result);
    }
    task->product.imageDataList = std::move(transformed);
    callback.Invoke();
    return true;
  }

 protected:
  struct PixemapInfo {
    uint8_t* innerBuffer = nullptr;
    size_t bufferLength = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t format = PIXEL_FORMAT_UNKNOWN;
  };

  /**
   * @brief Override this function to handle business logic before image
   * transformation. This function is executed only once per transformation
   * session.
   * @param task The loading task object.
   * @return Returns false to end the transformation, or true to continue.
   */
  virtual bool BeforeProcess(std::shared_ptr<ImageCacheTask> task) {
    return true;
  }

  /**
   * @brief Override this function to handle business logic after image
   * transformation. This function is executed only once per transformation
   * session.
   */
  virtual void AfterProcess() {}

  std::shared_ptr<ImageCacheData> TransformSingle(
      std::shared_ptr<ImageCacheTaskInternal> taskInternal,
      std::shared_ptr<ImageCacheData> data) {
    if (data == nullptr) {
      return nullptr;
    }
    uint32_t count = data->GetFrameCount();
    if (count > 1) {
      auto transformed = TransformAnimation(taskInternal, data);
      if (transformed == nullptr) {
        return data;
      }
      return transformed;
    } else if (count <= 0) {
      TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::FRAME_COUNT_ZERO);
      return data;
    } else {
      OH_PixelmapNative* pixelmapIn = data->GetPixelmap();
      if (pixelmapIn == nullptr) {
        TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::PIXELMAP_NULL);
        return data;
      }
      OH_PixelmapNative* pixelmapOut = nullptr;
      std::string errorInfo = Process(pixelmapIn, &pixelmapOut);
      if (!errorInfo.empty()) {
        TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
        return data;
      }
      if (pixelmapOut != nullptr) {
        return std::make_shared<ImageCacheData>(pixelmapOut);
      }
      return data;
    }
  }

  /**
   * @brief Override this function to process a single-frame pixelmap
   * transformation.
   * @param pixelmapIn The image to be processed, must not be nullptr.
   * @param pixelmapOut The processed image copy. If pixelmapOut is written back
   * as nullptr, it means the transformation is performed on the original
   * pixelmapIn.
   * @return Returns an error message. An empty string indicates success.
   */
  virtual std::string Process(
      OH_PixelmapNative* pixelmapIn,
      OH_PixelmapNative** pixelmapOut) = 0;

  PixemapInfo ParsePixelmapInfoReadPixels(
      OH_PixelmapNative* pixelmap,
      bool readPixels = true) {
    const int number4 = 4;
    const int number2 = 2;
    PixemapInfo info;
    IMAGE_CACHE_CHECK_RETURN(pixelmap == nullptr, info);
    OH_Pixelmap_ImageInfo* imageInfo = nullptr;

    Image_ErrorCode errorCode = OH_PixelmapImageInfo_Create(&imageInfo);
    if (errorCode != IMAGE_SUCCESS) {
      LOG(ERROR) << "Parse Pixelmap Create ImageInfo Failed: " << errorCode;
      return info;
    }
    errorCode = OH_PixelmapNative_GetImageInfo(pixelmap, imageInfo);
    if (errorCode != IMAGE_SUCCESS) {
      LOG(ERROR) << "Parse Pixelmap Get ImageInfo Failed: " << errorCode;
      OH_PixelmapImageInfo_Release(imageInfo);
      return info;
    }

    OH_PixelmapImageInfo_GetWidth(imageInfo, &info.width);
    OH_PixelmapImageInfo_GetHeight(imageInfo, &info.height);

    OH_PixelmapImageInfo_GetPixelFormat(imageInfo, &info.format);
    OH_PixelmapImageInfo_Release(imageInfo);
    IMAGE_CACHE_CHECK_RETURN(info.width <= 0 || info.height <= 0, info);
    IMAGE_CACHE_CHECK_RETURN(
        info.format != PIXEL_FORMAT_RGBA_8888,
        info,
        "Default Transformation Only Support RGBA For Now");
    IMAGE_CACHE_CHECK_RETURN(info.format == PIXEL_FORMAT_UNKNOWN, info);

    size_t pixelCount =
        static_cast<size_t>(info.width) * static_cast<size_t>(info.height);
    if (info.format == PIXEL_FORMAT_NV21 || info.format == PIXEL_FORMAT_NV12) {
      if (pixelCount > SIZE_MAX / number2) {
        LOG(ERROR) << "Image dimensions too large, potential overflow";
        return info;
      }
      info.bufferLength = pixelCount +
          ((info.width + 1) / number2) * ((info.height + 1) / number2) *
              number2;
    } else {
      if (pixelCount > SIZE_MAX / number4) {
        LOG(ERROR) << "Image dimensions too large, potential overflow";
        return info;
      }
      info.bufferLength = pixelCount * number4;
    }

    if (readPixels) {
      if (info.bufferLength == 0) {
        LOG(ERROR) << "Invalid buffer length after size calculation";
        return info;
      }
      info.innerBuffer = (uint8_t*)malloc(info.bufferLength);
      if (info.innerBuffer == nullptr) {
        LOG(ERROR) << "Failed to allocate buffer of size " << info.bufferLength;
        return info;
      }
      errorCode = OH_PixelmapNative_ReadPixels(
          pixelmap, info.innerBuffer, &info.bufferLength);
      if (errorCode != IMAGE_SUCCESS) {
        free(info.innerBuffer);
        info.innerBuffer = nullptr;
      }
    }
    return info;
  }

  void CreatePixelmapWithBuffer(
      uint8_t* buffer,
      PixemapInfo info,
      OH_PixelmapNative** pixelmap) {
    OH_Pixelmap_InitializationOptions* options = nullptr;
    Image_ErrorCode errorCode =
        OH_PixelmapInitializationOptions_Create(&options);
    if (errorCode != IMAGE_SUCCESS) {
      LOG(ERROR) << "Create Pixelmap Initialization Options Failed:"
                 << errorCode;
      return;
    }
    OH_PixelmapInitializationOptions_SetWidth(options, info.width);
    OH_PixelmapInitializationOptions_SetHeight(options, info.height);
    OH_PixelmapInitializationOptions_SetPixelFormat(options, info.format);
    OH_PixelmapInitializationOptions_SetSrcPixelFormat(options, info.format);

    errorCode = OH_PixelmapNative_CreatePixelmap(
        buffer, info.bufferLength, options, pixelmap);
    if (errorCode != IMAGE_SUCCESS) {
      LOG(ERROR) << "Create Pixelmap Failed:" << errorCode;
    }
    OH_PixelmapInitializationOptions_Release(options);
  }

 private:
  struct EndCallback {
    std::function<void()> func = nullptr;
    ~EndCallback() {
      Invoke();
    }
    void Invoke() {
      if (func != nullptr) {
        func();
        func = nullptr;
      }
    }
  };

  std::shared_ptr<ImageCacheData> TransformAnimation(
      std::shared_ptr<ImageCacheTaskInternal> taskInternal,
      std::shared_ptr<ImageCacheData> data) {
    auto option = taskInternal->GetOption();
    IMAGE_CACHE_CHECK_RETURN(option == nullptr, nullptr);

    const uint32_t count = data->GetFrameCount();
    OH_PixelmapNative** pixelmapListIn = data->GetPixelmapList();
    int32_t* timeListIn = data->GetDelayTimeList();
    if (pixelmapListIn == nullptr) {
      TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::PIXELMAP_NULL);
      return nullptr;
    }

    IMAGE_CACHE_CHECK_RETURN(count <= 0, nullptr);
    auto pixelmapListOut = std::make_unique<OH_PixelmapNative*[]>(count);
    auto timeListOut = std::make_unique<int32_t[]>(count);
    (void)memset(pixelmapListOut.get(), 0, sizeof(OH_PixelmapNative*) * count);
    (void)memset(timeListOut.get(), 0, sizeof(int32_t) * count);
    int countOut = 0;
    for (uint32_t i = 0; i < count; i++) {
      if (taskInternal->ShouldAbort() ||
          taskInternal->GetError() != ImageCacheErrorCode::OK) {
        break;
      }
      if (pixelmapListIn[i] == nullptr) {
        TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::PIXELMAP_NULL);
        continue;
      }

      std::string errorInfo =
          Process(pixelmapListIn[i], &pixelmapListOut[countOut]);
      if (errorInfo.empty()) {
        if (pixelmapListOut[countOut] == nullptr) {
          pixelmapListOut[countOut] = pixelmapListIn[i];
          pixelmapListIn[i] = nullptr;
        }
        if (timeListIn != nullptr) {
          timeListOut[countOut] = timeListIn[i];
        }
        countOut++;
      } else {
        TASK_SET_ERROR(taskInternal, ImageCacheErrorCode::DECODE_FAILED);
      }
    }

    if (countOut <= 0) {
      return nullptr;
    }
    return std::make_shared<ImageCacheData>(
        std::move(pixelmapListOut),
        std::move(timeListOut),
        static_cast<size_t>(countOut));
  }
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_TRANSFORMATION_BASE_H
