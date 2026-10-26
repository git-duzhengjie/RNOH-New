/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */
#pragma once

namespace rnoh {

extern bool IMAGE_CACHE_RUNTIME_SWITCH;

inline bool GetImageCacheEnabled() {
  return IMAGE_CACHE_RUNTIME_SWITCH;
}

void SetImageCacheEnabled(bool enabled);

inline bool IsImageCacheEnabled() {
#ifdef IMAGE_CACHE_ENABLED
  return GetImageCacheEnabled();
#else
  return false;
#endif
}

} // namespace rnoh
