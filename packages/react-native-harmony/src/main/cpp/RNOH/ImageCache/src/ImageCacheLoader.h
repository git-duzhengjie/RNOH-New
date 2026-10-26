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

#ifndef IMAGE_CACHE_LOADER_H
#define IMAGE_CACHE_LOADER_H

#include "ImageCacheInterceptor.h"

namespace ImageCachePro {

class ImageCacheManagerInternal;

class ImageCacheLoader {
 public:
  enum class Position {
    START,
    END,
  };

  virtual void AddMemoryCacheInterceptor(
      std::shared_ptr<ImageCacheMemoryInterceptor> interceptor,
      Position position = Position::START) = 0;
  virtual void AddLoadInterceptor(
      std::shared_ptr<ImageCacheLoadInterceptor> interceptor,
      Position position = Position::START) = 0;
  virtual void AddDecodeInterceptor(
      std::shared_ptr<ImageCacheDecodeInterceptor> interceptor,
      Position position = Position::START) = 0;

  static std::shared_ptr<ImageCacheLoader> CreateDefaultLoader(
      ImageCacheManagerInternal& manager);
  static std::shared_ptr<ImageCacheLoader> CreateEmptyLoader();
  virtual ~ImageCacheLoader() = default;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_LOADER_H
