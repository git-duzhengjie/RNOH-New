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

#ifndef IMAGE_CACHE_DATA_URI_LOAD_INTERCEPTOR_H
#define IMAGE_CACHE_DATA_URI_LOAD_INTERCEPTOR_H

#include <cstring>
#include "ImageCacheBase64Util.h"
#include "ImageCacheInterceptor.h"
#include "ImageCacheLog.h"

namespace ImageCachePro {

class ImageCacheDataUriLoadInterceptor : public ImageCacheLoadInterceptor {
 public:
  explicit ImageCacheDataUriLoadInterceptor(ImageCacheManagerInternal& manager)
      : ImageCacheLoadInterceptor(manager) {
    name = "DataUriLoadInterceptor";
    isLoadFromRemote = false;
  }

  bool Resolve(const std::shared_ptr<ImageCacheTask>& task) override {
    std::string url;
    if (!task->GetImageSource().GetString(url)) {
      return false;
    }
    if (!IsDataImageUri(url)) {
      return false;
    }

    std::string payload;
    if (!ExtractBase64Payload(url, payload)) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::INVALID_DATA_URI);
      return false;
    }

    auto decoded = ImageCacheBase64Util::Decode(payload);
    if (decoded.empty()) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::BASE64_DECODE_FAILED);
      return false;
    }

    auto buffer = std::shared_ptr<uint8_t[]>(new uint8_t[decoded.size()]);
    std::memcpy(buffer.get(), decoded.data(), decoded.size());
    task->product.imageBuffer = buffer;
    task->product.imageLength = static_cast<uint32_t>(decoded.size());
    return true;
  }

 private:
  static bool IsDataImageUri(const std::string& url) {
    return url.rfind("data:image/", 0) == 0;
  }

  static bool ExtractBase64Payload(
      const std::string& url,
      std::string& payload) {
    static constexpr const char* base64Marker = ";base64,";
    auto pos = url.find(base64Marker);
    if (pos == std::string::npos) {
      return false;
    }
    payload = url.substr(pos + std::strlen(base64Marker));
    return !payload.empty();
  }
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DATA_URI_LOAD_INTERCEPTOR_H
