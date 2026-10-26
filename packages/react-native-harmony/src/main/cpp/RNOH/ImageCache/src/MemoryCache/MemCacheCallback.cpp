/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "MemCacheCallback.h"
#include "ImageCacheLog.h"

namespace ImageCachePro {

std::vector<MemCacheCallback::Callback> MemCacheCallback::s_callbacks;
std::mutex MemCacheCallback::s_mutex;

void MemCacheCallback::registerCallback(Callback callback) {
    if (!callback) {
        return;
    }
    std::lock_guard<std::mutex> lock(s_mutex);
    s_callbacks.push_back(std::move(callback));
}

void MemCacheCallback::notifyAll(int memoryLevel) {
    std::vector<Callback> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        callbacksCopy = s_callbacks;
    }
    for (auto& cb : callbacksCopy) {
        if (cb) {
            cb(memoryLevel);
        }
    }
}

void MemCacheCallback::notifyBackground() {
    std::vector<Callback> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        callbacksCopy = s_callbacks;
    }
    for (auto& cb : callbacksCopy) {
        if (cb) {
            cb(MEMORY_LEVEL_CRITICAL);
        }
    }
}

} // namespace ImageCachePro
