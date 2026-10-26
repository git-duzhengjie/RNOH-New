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

#ifndef IMAGE_CACHE_TASK_H
#define IMAGE_CACHE_TASK_H

#include <memory>
#include <vector>
#include "ImageCacheData.h"
#include "ImageCacheSource.h"
#include "ImageCacheType.h"

struct NativeResourceManager;

namespace rnoh {
class TaskExecutor;
}

namespace ImageCachePro {

struct FrameDecodeContext;

enum class ProductType {
  NONE,
  IMAGE_DATA,
  FRAME_DATA,
};

enum class ImageCacheTaskState : int32_t {
  READY = 0,   // Ready, in READY queue waiting for scheduling
  RUNNING,     // Executing
  SUSPEND,     // Suspended, waiting for async IO (download) to complete
  DONE,        // Execution complete (success or failure), triggers callback
  CLOSED,      // Closed (cancel), does not trigger success callback
};

class ImageCacheTask {
 public:
  struct Product {
    ProductType type{ProductType::NONE};
    std::shared_ptr<uint8_t[]> imageBuffer = nullptr;
    uint32_t imageLength = 0;
    std::vector<std::shared_ptr<ImageCacheData>> imageDataList;
    std::shared_ptr<FrameDecodeContext> frameDecodeContext = nullptr;
  };

  std::string cacheKey;
  ImageCacheTaskType cacheType = ImageCacheTaskType::NONE;

  Product product;

  virtual void SetError(ImageCacheErrorCode code) = 0;
  virtual ImageCacheErrorCode GetError() const = 0;
  virtual const ImageCacheSource& GetImageSource() const = 0;
  virtual const ImageCacheFileTypeInfo& GetFileTypeInfo() const = 0;
  virtual ~ImageCacheTask() = default;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_TASK_H
