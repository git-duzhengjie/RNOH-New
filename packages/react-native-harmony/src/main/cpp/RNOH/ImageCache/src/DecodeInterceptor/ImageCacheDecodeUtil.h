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

#ifndef IMAGE_CACHE_DECODE_UTIL_H
#define IMAGE_CACHE_DECODE_UTIL_H

#include <memory>
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

class ImageCacheDecodeUtil {
 public:
  static bool Decode(
      const std::shared_ptr<ImageCacheTaskInternal>& taskInternal);

  static std::shared_ptr<ImageCacheData> DecodeFrame(
      const std::shared_ptr<ImageCacheTaskInternal>& taskInternal,
      int frameIndex);

  static bool ParseLazyHeader(
      const std::shared_ptr<ImageCacheTaskInternal>& taskInternal);

  static bool IsAnimatedFormat(ImageCacheFormat format);

  static std::unique_ptr<int32_t[]> GetDelayTimeList(
      const std::shared_ptr<ImageCacheTaskInternal>& taskInternal,
      uint32_t& frameCount);

 private:
  static bool DecodeAnimated(
      const std::shared_ptr<ImageCacheTaskInternal>& taskInternal);
  static bool DecodeStatic(
      const std::shared_ptr<ImageCacheTaskInternal>& taskInternal);
  static OH_DecodingOptions* CreateDecodeOptions(const Image_Size& desiredSize);
  static void ReleasePixelmapList(
      OH_PixelmapNative** pixelmapList,
      size_t frameCount);
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_DECODE_UTIL_H
