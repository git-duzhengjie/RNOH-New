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

#ifndef IMAGE_CACHE_RESOURCE_LOAD_INTERCEPTOR_H
#define IMAGE_CACHE_RESOURCE_LOAD_INTERCEPTOR_H

#include <resourcemanager/ohresmgr.h>
#include <cstdint>
#include <memory>
#include "ImageCacheInterceptor.h"
#include "ImageCacheLog.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheTask.h"

namespace ImageCachePro {

class ImageCacheResourceLoadInterceptor : public ImageCacheLoadInterceptor {
 public:
  explicit ImageCacheResourceLoadInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheLoadInterceptor(manager) {
    name = "ResourceLoadInterceptor";
    isLoadFromRemote = false;
  }

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override {
    ImageCacheResource resource;
    if (!task->GetImageSource().GetResource(resource)) {
      return false;
    }
    return LoadImageFromResource(resource, task);
  }

 private:
  bool LoadImageFromResource(
      const ImageCacheResource& resource,
      const std::shared_ptr<ImageCacheTask>& task) {
    const NativeResourceManager* resourceManager =
        GetManager().GetNativeResourceManager();
    if (resourceManager == nullptr) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::RESOURCE_MANAGER_NULL);
      return false;
    }

    uint64_t length = 0;
    uint8_t* buffer = nullptr;
    ResourceManager_ErrorCode errCode = OH_ResourceManager_GetMedia(
        const_cast<NativeResourceManager*>(resourceManager),
        resource.id,
        &buffer,
        &length);

    if (errCode != ResourceManager_ErrorCode::SUCCESS && resource.id == -1) {
      errCode =
          GetBufferFromOtherModule(resource, resourceManager, &buffer, length);
    }
    if (errCode != ResourceManager_ErrorCode::SUCCESS) {
      errCode =
          GetBufferFromRawfile(resource, resourceManager, &buffer, length);
    }
    if (errCode != ResourceManager_ErrorCode::SUCCESS) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::READ_RAWFILE_FAILED);
      return false;
    }
    if (length == 0 || length > UINT32_MAX) {
      if (buffer != nullptr) {
        free(buffer);
      }
      TASK_SET_ERROR(
          task,
          length == 0 ? ImageCacheErrorCode::EMPTY_LOCAL_FILE
                      : ImageCacheErrorCode::LOCAL_FILE_SIZE_OUT_OF_RANGE);
      return false;
    }

    task->product.imageBuffer =
        std::shared_ptr<uint8_t[]>(buffer, [](void* ptr) {
          if (ptr != nullptr) {
            free(static_cast<uint8_t*>(ptr));
          }
        });
    task->product.imageLength = static_cast<uint32_t>(length);
    return true;
  }

  static ResourceManager_ErrorCode GetBufferFromOtherModule(
      const ImageCacheResource& resource,
      const NativeResourceManager* resourceManager,
      uint8_t** buffer,
      uint64_t& length) {
    const char* param = resource.param.c_str();
    constexpr size_t kMaxFileNameLength = 128;
    char fileName[kMaxFileNameLength] = {0};
    size_t fileLength = 0;
    size_t index = 0;
    for (int i = static_cast<int>(resource.param.size()) - 1; i >= 0; i--) {
      if (param[i] == '.') {
        index = static_cast<size_t>(i + 1);
        break;
      }
    }
    for (size_t i = index; i < resource.param.size(); i++) {
      if (fileLength + 1 >= kMaxFileNameLength) {
        return ResourceManager_ErrorCode::ERROR_CODE_RES_ID_NOT_FOUND;
      }
      fileName[fileLength++] = param[i];
    }
    fileName[fileLength] = '\0';
    return OH_ResourceManager_GetMediaByName(
        const_cast<NativeResourceManager*>(resourceManager),
        fileName,
        buffer,
        &length);
  }

  static ResourceManager_ErrorCode GetBufferFromRawfile(
      const ImageCacheResource& resource,
      const NativeResourceManager* resourceManager,
      uint8_t** buffer,
      uint64_t& length) {
    RawFile64* rFile = OH_ResourceManager_OpenRawFile64(
        const_cast<NativeResourceManager*>(resourceManager),
        resource.param.c_str());
    length = 0;
    if (rFile == nullptr) {
      return ResourceManager_ErrorCode::ERROR_CODE_RES_ID_NOT_FOUND;
    }

    uint64_t len = OH_ResourceManager_GetRawFileRemainingLength64(rFile);
    if (*buffer == nullptr) {
      *buffer = static_cast<uint8_t*>(malloc((len + 1) * sizeof(uint8_t)));
      if (*buffer == nullptr) {
        OH_ResourceManager_CloseRawFile64(rFile);
        return ResourceManager_ErrorCode::ERROR_CODE_RES_ID_NOT_FOUND;
      }
    }
    length = OH_ResourceManager_ReadRawFile64(rFile, *buffer, len);
    (*buffer)[length] = 0;
    OH_ResourceManager_CloseRawFile64(rFile);
    if (length > 0) {
      return ResourceManager_ErrorCode::SUCCESS;
    }
    return ResourceManager_ErrorCode::ERROR_CODE_RES_ID_NOT_FOUND;
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_RESOURCE_LOAD_INTERCEPTOR_H
