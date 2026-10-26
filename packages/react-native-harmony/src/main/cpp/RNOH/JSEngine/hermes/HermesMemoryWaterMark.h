/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#ifndef RNOH_FRAMEWORK_VERSION
#define RNOH_FRAMEWORK_VERSION "0.84"
#endif

#if defined(__GNUC__)
#define RNOH_HERMES_WATERMARK_EXPORT __attribute__((visibility("default")))
#else
#define RNOH_HERMES_WATERMARK_EXPORT
#endif

namespace rnoh {

constexpr int64_t DEFAULT_HERMES_MEMORY_WATER_MARK_MB = 1434LL;
constexpr int64_t BYTES_PER_MB = 1024LL * 1024LL;

RNOH_HERMES_WATERMARK_EXPORT void setHermesMemoryWaterMarkMB(
    int64_t hermesMemoryWaterMarkMB);

RNOH_HERMES_WATERMARK_EXPORT int64_t getHermesMemoryWaterMarkMB();

RNOH_HERMES_WATERMARK_EXPORT int64_t getHermesMemoryWaterMarkBytes();

RNOH_HERMES_WATERMARK_EXPORT
bool tryMarkHermesWaterMarkExceededForProcessLifetime();

RNOH_HERMES_WATERMARK_EXPORT const std::string& getHermesFrameworkVersion();

inline double convertBytesToMB(int64_t bytes) {
  return static_cast<double>(bytes) / static_cast<double>(BYTES_PER_MB);
}

inline std::string formatBytesToMBString(int64_t bytes) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << convertBytesToMB(bytes);
  return oss.str();
}

inline std::string buildHermesWaterMarkDescription(
    int64_t currentHeapBytes,
    int64_t waterMarkBytes) {
  std::ostringstream oss;
  oss << "js heap exceeded watermark, current=" << currentHeapBytes
      << " bytes (" << formatBytesToMBString(currentHeapBytes) << " MB)"
      << ", watermark=" << waterMarkBytes << " bytes ("
      << formatBytesToMBString(waterMarkBytes) << " MB)";
  return oss.str();
}

} // namespace rnoh
