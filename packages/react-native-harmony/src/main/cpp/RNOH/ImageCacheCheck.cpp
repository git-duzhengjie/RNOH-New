/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */
#include "RNOH/ImageCacheCheck.h"
#include <glog/logging.h>
#ifdef IMAGE_CACHE_ENABLED
#include "RNOH/ImageCache/include/MemCacheCallback.h"
#endif

namespace rnoh {

bool IMAGE_CACHE_RUNTIME_SWITCH = false;

void SetImageCacheEnabled(bool enabled) {
  bool oldValue = IMAGE_CACHE_RUNTIME_SWITCH;
  IMAGE_CACHE_RUNTIME_SWITCH = enabled;
  if (oldValue && !enabled) {
#ifdef IMAGE_CACHE_ENABLED
    DLOG(INFO)
        << "SetImageCacheEnabled: turned OFF, flushing shared memory cache";
    ImageCachePro::MemCacheCallback::notifyAll(MEMORY_LEVEL_CRITICAL);
#endif
  }
}

} // namespace rnoh
