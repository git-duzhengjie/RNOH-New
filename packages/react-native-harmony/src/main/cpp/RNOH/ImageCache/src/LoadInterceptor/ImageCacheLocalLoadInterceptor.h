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

#ifndef IMAGE_CACHE_LOCAL_LOAD_INTERCEPTOR_H
#define IMAGE_CACHE_LOCAL_LOAD_INTERCEPTOR_H

#include "ImageCacheInterceptor.h"
#include "ImageCacheResourceLoader.h"
#include "ImageCacheUriUtil.h"

namespace ImageCachePro {

class ImageCacheLocalLoadInterceptor : public ImageCacheLoadInterceptor {
 public:
  explicit ImageCacheLocalLoadInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheLoadInterceptor(manager) {
    name = "LocalLoadInterceptor";
    isLoadFromRemote = false;
  }

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override {
    std::string path;
    if (!task->GetImageSource().GetString(path)) {
      return false;
    }
    auto kind = ImageCacheUriUtil::ClassifyUri(path);
    if (kind != ImageCacheRnohSourceKind::File &&
        kind != ImageCacheRnohSourceKind::BundleStorage) {
      return false;
    }
    return ImageCacheResourceLoader::LoadFromFilePath(path, task);
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_LOCAL_LOAD_INTERCEPTOR_H
