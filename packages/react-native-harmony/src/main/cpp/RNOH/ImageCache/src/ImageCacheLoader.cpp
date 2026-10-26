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

#include "ImageCacheLoader.h"
#include "ImageCacheAssetLoadInterceptor.h"
#include "ImageCacheDataUriLoadInterceptor.h"
#include "ImageCacheDecodeInterceptorDefault.h"
#include "ImageCacheDownloadInterceptor.h"
#include "ImageCacheLoaderInternal.h"
#include "ImageCacheLocalLoadInterceptor.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheMemoryInterceptorDefault.h"
#ifdef IMAGE_CACHE_ENABLE_RESOURCE
#include "ImageCacheResourceLoadInterceptor.h"
#endif

namespace ImageCachePro {

std::shared_ptr<ImageCacheLoader> ImageCacheLoader::CreateDefaultLoader(
    ImageCacheManagerInternal& manager) {
  auto memoryInterceptor =
      std::make_shared<ImageCacheMemoryInterceptorDefault>(manager);
  auto decodeInterceptor =
      std::make_shared<ImageCacheDecodeInterceptorDefault>(manager);

  auto loader = std::make_shared<ImageCacheLoaderInternal>();
  loader->AddMemoryCacheInterceptor(memoryInterceptor);
  loader->AddDecodeInterceptor(decodeInterceptor);
  loader->AddLoadInterceptor(
      std::make_shared<ImageCacheDataUriLoadInterceptor>(manager));
  loader->AddLoadInterceptor(
      std::make_shared<ImageCacheAssetLoadInterceptor>(manager));
  loader->AddLoadInterceptor(
      std::make_shared<ImageCacheLocalLoadInterceptor>(manager));

#ifdef IMAGE_CACHE_ENABLE_RESOURCE
  if (manager.GetNativeResourceManager() != nullptr) {
    loader->AddLoadInterceptor(
        std::make_shared<ImageCacheResourceLoadInterceptor>(manager));
  }
#endif

  loader->AddLoadInterceptor(std::make_shared<ImageCacheDownloadInterceptor>(manager));
  return loader;
}

std::shared_ptr<ImageCacheLoader> ImageCacheLoader::CreateEmptyLoader() {
  return std::make_shared<ImageCacheLoaderInternal>();
}

} // namespace ImageCachePro
