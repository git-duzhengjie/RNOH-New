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

#include "ImageCacheUriUtil.h"

namespace ImageCachePro {

namespace {

constexpr const char* ASSET_PREFIX = "asset://";
constexpr const char* RAWFILE_PREFIX = "resource://RAWFILE/assets/";
constexpr const char* FILE_PREFIX = "file://";

} // namespace

bool ImageCacheUriUtil::IsRemoteUri(const std::string& uri) {
  return !uri.empty() &&
      (uri.rfind("http://", 0) == 0 || uri.rfind("https://", 0) == 0);
}

ImageCacheRnohSourceKind ImageCacheUriUtil::ClassifyUri(
    const std::string& uri) {
  if (uri.empty()) {
    return ImageCacheRnohSourceKind::Unknown;
  }
  if (IsRemoteUri(uri)) {
    return ImageCacheRnohSourceKind::Remote;
  }
  if (uri.rfind("data:image/", 0) == 0) {
    return ImageCacheRnohSourceKind::DataUri;
  }
  if (uri.rfind(ASSET_PREFIX, 0) == 0) {
    return ImageCacheRnohSourceKind::Asset;
  }
  if (uri.rfind(RAWFILE_PREFIX, 0) == 0) {
    return ImageCacheRnohSourceKind::Rawfile;
  }
  if (uri.rfind(FILE_PREFIX, 0) == 0) {
    return ImageCacheRnohSourceKind::File;
  }
  if (uri.rfind("/data/storage", 0) == 0) {
    return ImageCacheRnohSourceKind::BundleStorage;
  }
  return ImageCacheRnohSourceKind::Unknown;
}

bool ImageCacheUriUtil::IsLocalUri(const std::string& uri) {
  auto kind = ClassifyUri(uri);
  return kind != ImageCacheRnohSourceKind::Unknown &&
      kind != ImageCacheRnohSourceKind::Remote;
}

} // namespace ImageCachePro
