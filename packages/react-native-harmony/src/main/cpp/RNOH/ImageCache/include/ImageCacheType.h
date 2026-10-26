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

#ifndef IMAGE_CACHE_TYPE_H
#define IMAGE_CACHE_TYPE_H

#include <resourcemanager/ohresmgr.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace ImageCachePro {

struct ImageCacheData;
struct FrameDecodeContext;

enum class ImageCacheRequestType {
  MAIN_SRC = 0,
  PLACE_SRC,
  ERROR_SRC,
  THUMBNAIL_SRC,
};

enum class ImageCacheDataType : unsigned int {
  UNDEFINED = 1,
  STRING = 1 << 1,
  RESOURCE = 1 << 4,
};

enum class ImageCacheOrientation {
  AUTO = 0,
  UP,
  RIGHT,
  DOWN,
  LEFT,
};

enum class ImageCacheDynamicRange {
  AUTO = 0,
  SDR,
  HDR,
};

struct ImageCacheResource {
  std::string bundleName;
  std::string moduleName;
  int id = 0;
  std::string param;
};

enum class ImageCacheStrategy {
  MEMORY = 0,
  NONE = 1,
};

enum class ImageCacheTaskType {
  NONE = 0,
  WRITE,
  READ,
};

enum class ImageCacheFormat {
  UNKNOWN = 0,
  JPG,
  PNG,
  WEBP,
  BMP,
  GIF,
  SVG,
  TIFF,
  HEIC,
  ICO,
};

enum class ImageCachePriority {
  HIGH = 0,
  MEDIUM,
  LOW,
};

enum class ImageCacheDownSamplingStrategy {
  NONE = 0,
  DEFAULT,
  FIT_CENTER_MEMORY,
  FIT_CENTER_QUALITY,
  CENTER_INSIDE_MEMORY,
  CENTER_INSIDE_QUALITY,
  AT_LEAST,
  AT_MOST,
};

enum class ImageCacheAnimationDecodeMode {
  AUTO = 0,
  BATCH_MODE,
  FRAME_MODE,
};

enum class ImageCacheErrorCode : uint16_t {
  OK = 0,

  // -- Task lifecycle --
  TASK_ABORTED = 100,
  JOB_ABANDONED = 101,
  DEFAULT_LOADER_NULL = 102,

  // -- Loading errors --
  DOWNLOAD_FAILED = 200,
  EMPTY_URL = 201,
  ALLOCATE_REQUEST_ID_FAILED = 202,
  DOWNLOAD_DESTROYED = 203,
  RESOURCE_MANAGER_NULL = 204,
  OPEN_RAWFILE_FAILED = 205,
  READ_RAWFILE_FAILED = 206,
  OPEN_LOCAL_FILE_FAILED = 207,
  EMPTY_LOCAL_FILE = 208,
  LOCAL_FILE_SIZE_OUT_OF_RANGE = 209,
  READ_LOCAL_FILE_FAILED = 210,
  EMPTY_RAWFILE_PATH = 211,
  INVALID_DATA_URI = 212,
  BASE64_DECODE_FAILED = 213,
  IMAGE_DATA_UNAVAILABLE = 214,
  EMPTY_IMAGE_DATA = 215,

  // -- Decode errors --
  CREATE_DECODE_OPTION_FAILED = 300,
  ANIMATED_DECODE_INVALID_FRAME_COUNT = 301,
  IMAGE_SOURCE_NATIVE_NULL = 302,
  DECODE_FAILED = 303,
  IMAGE_DECODE_ERROR = 304,
  EMPTY_IMAGE_DATA_AFTER_DECODE = 305,
  UNKNOWN_FILE_TYPE = 306,
  EMPTY_IMAGE_BUFFER = 307,
  CREATE_SOURCE_FAILED = 308,

  // -- Transformation errors --
  EMPTY_TRANSFORMATION = 400,
  TASK_IMAGE_DATA_NULL = 401,
  FRAME_COUNT_ZERO = 402,
  PIXELMAP_NULL = 403,
  GL_INIT_FAILED = 404,
  MASK_UNDEFINED = 405,
  MASK_CACHE_KEY_GENERATOR_NULL = 406,
  MASK_IMAGE_DATA_EMPTY = 407,
  MASK_PIXELMAP_NULL = 408,
  MASK_READ_PIXELS_FAILED = 409,
  TRANSFORMATION_FAILED = 410,

  // -- Memory cache errors --
  CACHE_EMPTY_IMAGE_DATA = 500,
  CACHE_EMPTY_MEMORY_KEY = 501,
  IMAGE_TOO_LARGE = 502,
  CACHE_CREATE_FAILED = 503,

  // -- Frame decode errors --
  ANIM_CONTROLLER_DESTROYED = 600,

  // -- Public API aliases --
  SUCCESS = OK,
  UNKNOWN_ERROR = 1,
  QUEUE_CANCEL = 1000,
  LOADING_CANCEL = 1001,
  SUSPEND_TIMEOUT = 1002,
  LOAD_IMAGE_FAILED = DOWNLOAD_FAILED,
};

// Opaque handle returned by GetCacheImage, usable for cancellation.
using ImageCacheRequestHandle = uint64_t;
static constexpr ImageCacheRequestHandle INVALID_REQUEST_HANDLE = UINT64_MAX;

// Opaque handle returned by PlayAnimation, usable for ControlAnimation.
using ImageCacheAnimHandle = ImageCacheRequestHandle;
static constexpr ImageCacheAnimHandle INVALID_ANIM_HANDLE =
    INVALID_REQUEST_HANDLE;

/** Return data type indicator for GetCacheImage callback */
enum class ImageCacheLoadDataType {
  NONE = 0,
  IMAGE_DATA,
  FRAME_DECODE_DATA,
};

// ImageCacheLoadData and ImageCacheLoadResult are defined in ImageCacheData.h

} // namespace ImageCachePro

#endif // IMAGE_CACHE_TYPE_H
