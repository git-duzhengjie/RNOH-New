/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */
#pragma once

#include <deviceinfo.h>
#include <mutex>

namespace rnoh {

constexpr int API_LEVEL_14 = 14;
constexpr int API_LEVEL_15 = 15;
constexpr int API_LEVEL_18 = 18;
constexpr int API_LEVEL_20 = 20;
constexpr int API_LEVEL_21 = 21;
constexpr int API_LEVEL_22 = 22;
constexpr int API_LEVEL_24 = 24;
constexpr int API_LEVEL_26 = 26;

/**
 * @brief Lazily check if API level is at least the given level.
 *
 * @tparam ApiLevel Minimum API level to check.
 * @return true if OH_GetSdkApiVersion() >= ApiLevel, false otherwise.
 */
template <int ApiLevel>
inline bool IsAtLeastApi() {
  static std::once_flag flag;
  static bool cachedResult = false;
  std::call_once(
      flag, [] { cachedResult = OH_GetSdkApiVersion() >= ApiLevel; });
  return cachedResult;
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 20.
 * @return true if API level >= 20, false otherwise.
 */
inline bool IsAtLeastApi20() {
  return IsAtLeastApi<API_LEVEL_20>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 15.
 * @return true if API level >= 15, false otherwise.
 */
inline bool IsAtLeastApi15() {
  return IsAtLeastApi<API_LEVEL_15>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 18.
 * @return true if API level >= 18, false otherwise.
 */
inline bool IsAtLeastApi18() {
  return IsAtLeastApi<API_LEVEL_18>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 14.
 * @return true if API level >= 14, false otherwise.
 */
inline bool IsAtLeastApi14() {
  return IsAtLeastApi<API_LEVEL_14>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 21.
 * @return true if API level >= 21, false otherwise.
 */
inline bool IsAtLeastApi21() {
  return IsAtLeastApi<API_LEVEL_21>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 22.
 * @return true if API level >= 22, false otherwise.
 */
inline bool IsAtLeastApi22() {
  return IsAtLeastApi<API_LEVEL_22>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 24.
 * @return true if API level >= 24, false otherwise.
 */
inline bool IsAtLeastApi24() {
  return IsAtLeastApi<API_LEVEL_24>();
}

/**
 * @ThreadSafe
 *
 * Check if current API level is at least 26.
 * @return true if API level >= 26, false otherwise.
 */
inline bool IsAtLeastApi26() {
  return IsAtLeastApi<API_LEVEL_26>();
}

} // namespace rnoh
