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

#ifndef IMAGE_CACHE_LOG_H
#define IMAGE_CACHE_LOG_H

#include <glog/logging.h>

namespace ImageCachePro {

/**
 * IMAGE_CACHE_CHECK macros - Defensive programming checks for error conditions.
 *
 * Semantic: These macros check for ERROR/INVALID conditions and return early
 * when detected. Usage pattern: IMAGE_CACHE_CHECK(error_condition) - if
 * error_condition is TRUE, log and return.
 *
 * Example:
 *   IMAGE_CACHE_CHECK(ptr == nullptr)  // If ptr IS nullptr (error), log and
 * return IMAGE_CACHE_CHECK(url.empty())     // If url IS empty (error), log and
 * return
 *
 * This follows the "fail-fast" pattern where invalid conditions trigger
 * immediate exit.
 */

// IMAGE_CACHE_CHECK: Check error condition and return void if condition is true
// @param condition: The ERROR condition to check (returns if TRUE)
// @param msg (optional): Additional error message to log
#define IMAGE_CACHE_CHECK_1(condition)           \
  do {                                           \
    if ((condition)) {                           \
      LOG(ERROR) << "Check failed: " #condition; \
      return;                                    \
    }                                            \
  } while (0)

#define IMAGE_CACHE_CHECK_2(condition, msg)                       \
  do {                                                            \
    if ((condition)) {                                            \
      LOG(ERROR) << "Check failed: " #condition << ": " << (msg); \
      return;                                                     \
    }                                                             \
  } while (0)

#define IMAGE_CACHE_CHECK_IMPL(_1, _2, NAME, ...) NAME
#define IMAGE_CACHE_CHECK(...)                               \
  IMAGE_CACHE_CHECK_IMPL(                                    \
      __VA_ARGS__, IMAGE_CACHE_CHECK_2, IMAGE_CACHE_CHECK_1) \
  (__VA_ARGS__)

// IMAGE_CACHE_CHECK_RETURN: Check error condition and return specified value if
// condition is true
// @param condition: The ERROR condition to check (returns if TRUE)
// @param retval: Value to return when condition is true
// @param msg (optional): Additional error message to log
#define IMAGE_CACHE_CHECK_RETURN_2(condition, retval) \
  do {                                                \
    if ((condition)) {                                \
      LOG(ERROR) << "Check failed: " #condition;      \
      return (retval);                                \
    }                                                 \
  } while (0)

#define IMAGE_CACHE_CHECK_RETURN_3(condition, retval, msg)        \
  do {                                                            \
    if ((condition)) {                                            \
      LOG(ERROR) << "Check failed: " #condition << ": " << (msg); \
      return (retval);                                            \
    }                                                             \
  } while (0)

#define IMAGE_CACHE_CHECK_RETURN_IMPL(_1, _2, _3, NAME, ...) NAME
#define IMAGE_CACHE_CHECK_RETURN(...)                                      \
  IMAGE_CACHE_CHECK_RETURN_IMPL(                                           \
      __VA_ARGS__, IMAGE_CACHE_CHECK_RETURN_3, IMAGE_CACHE_CHECK_RETURN_2) \
  (__VA_ARGS__)

} // namespace ImageCachePro

// Macro to log file/line on SetError, then call SetError normally.
#define TASK_SET_ERROR(task, code)                                           \
  do {                                                                       \
    DLOG(WARNING) << "ImageCache Error " << static_cast<int>(code) << " at " \
                  << __FILE__ << ":" << __LINE__;                            \
    (task)->SetError((code));                                                \
  } while (0)

#endif // IMAGE_CACHE_LOG_H
