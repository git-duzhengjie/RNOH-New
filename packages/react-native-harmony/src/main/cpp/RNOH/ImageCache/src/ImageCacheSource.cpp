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

#include "ImageCacheSource.h"

namespace ImageCachePro {

void ImageCacheSource::CleanData() {
  url_.clear();
  resource_ = ImageCacheResource{};
  type_ = ImageCacheDataType::UNDEFINED;
}

void ImageCacheSource::SetUndefined() {
  CleanData();
}

void ImageCacheSource::SetString(const std::string& url) {
  CleanData();
  url_ = url;
  type_ = ImageCacheDataType::STRING;
}

void ImageCacheSource::SetResource(const ImageCacheResource& resource) {
  CleanData();
  resource_ = resource;
  type_ = ImageCacheDataType::RESOURCE;
}

bool ImageCacheSource::GetString(std::string& url) const {
  if (type_ != ImageCacheDataType::STRING) {
    return false;
  }
  url = url_;
  return true;
}

bool ImageCacheSource::GetResource(ImageCacheResource& resource) const {
  if (type_ != ImageCacheDataType::RESOURCE) {
    return false;
  }
  resource = resource_;
  return true;
}

ImageCacheDataType ImageCacheSource::GetType() const {
  return type_;
}

bool ImageCacheSource::IsEmpty() const {
  return type_ == ImageCacheDataType::UNDEFINED;
}

std::string ImageCacheSource::GetUrl() const {
  if (type_ != ImageCacheDataType::STRING) {
    return "";
  }
  return url_;
}

} // namespace ImageCachePro
