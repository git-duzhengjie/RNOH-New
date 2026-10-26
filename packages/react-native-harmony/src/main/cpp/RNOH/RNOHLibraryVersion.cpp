/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#include <cstdint>

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#endif
#ifndef VERSION_MINOR
#define VERSION_MINOR 0
#endif
#ifndef VERSION_PATCH
#define VERSION_PATCH 0
#endif
#ifndef VERSION_BUILD
#define VERSION_BUILD 0
#endif
#ifndef VERSION_STRING
#define VERSION_STRING "0.0.0.0"
#endif

extern "C" {
// 定义版本信息结构体，嵌入到.rodata 段
typedef struct {
  uint32_t major;
  uint32_t minor;
  uint32_t patch;
  uint32_t build;
  const char* full_version;
} version_info_t;

const version_info_t lib_version
    __attribute__((used, section(".rodata"))) = {
        .major = VERSION_MAJOR,
        .minor = VERSION_MINOR,
        .patch = VERSION_PATCH,
        .build = VERSION_BUILD,
        .full_version = VERSION_STRING,
};

} // extern "C"

// Greppable text marker for `strings`-based scanners that don't parse the
// version_info_t struct. Lives in .rodata next to lib_version.
__attribute__((used, section(".rodata"))) static const char
    kRnohLibraryVersionMarker[] = "RNOH_LIB_VERSION=" VERSION_STRING;
