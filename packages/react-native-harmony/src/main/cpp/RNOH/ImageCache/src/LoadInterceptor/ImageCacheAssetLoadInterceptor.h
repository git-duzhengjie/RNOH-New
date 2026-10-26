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

#ifndef IMAGE_CACHE_ASSET_LOAD_INTERCEPTOR_H
#define IMAGE_CACHE_ASSET_LOAD_INTERCEPTOR_H

#include "ImageCacheInterceptor.h"

namespace ImageCachePro {

/**
 * Unified handler for RNOH bundle image sources:
 * - asset:// (rawfile first, fallback option.assetsPrefix -> file/rawfile)
 * - resource://RAWFILE/assets/...
 * - Metro hot reload http://host/assets/...?platform=...
 */
class ImageCacheAssetLoadInterceptor : public ImageCacheLoadInterceptor {
 public:
  explicit ImageCacheAssetLoadInterceptor(ImageCacheManagerInternal& manager);

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_ASSET_LOAD_INTERCEPTOR_H
