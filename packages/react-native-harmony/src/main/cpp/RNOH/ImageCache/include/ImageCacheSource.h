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

#ifndef IMAGE_CACHE_SOURCE_H
#define IMAGE_CACHE_SOURCE_H

#include <string>
#include "ImageCacheType.h"

namespace ImageCachePro {

class ImageCacheSource {
 public:
  void SetString(const std::string& url);
  void SetResource(const ImageCacheResource& resource);
  bool GetString(std::string& url) const;
  bool GetResource(ImageCacheResource& resource) const;
  ImageCacheDataType GetType() const;
  bool IsEmpty() const;
  void SetUndefined();
  std::string GetUrl() const;

 private:
  void CleanData();

  ImageCacheDataType type_ = ImageCacheDataType::UNDEFINED;
  std::string url_;
  ImageCacheResource resource_;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_SOURCE_H
