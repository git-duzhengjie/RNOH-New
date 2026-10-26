/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef IMAGE_CACHE_MEM_CACHE_CALLBACK_H
#define IMAGE_CACHE_MEM_CACHE_CALLBACK_H

#include <functional>
#include <vector>
#include <mutex>

namespace ImageCachePro {

#define MEMORY_LEVEL_MODERATE  0
#define MEMORY_LEVEL_LOW       1
#define MEMORY_LEVEL_CRITICAL  2

/**
 * @brief Memory alert callback management class.
 * Used to register and manage memory alert callbacks, notifying all registered
 * callbacks when the system issues a memory alert.
 */
class MemCacheCallback {
public:
    using Callback = std::function<void(int)>;

    /**
     * @brief Register a memory alert callback.
     * @param callback Callback function, parameter is the memory alert level.
     */
    static void registerCallback(Callback callback);

    /**
     * @brief Notify all registered callbacks.
     * @param memoryLevel Memory alert level.
     *   - 0: Memory is tight, recommend clearing some cache.
     *   - 2: Memory is critically low, recommend clearing all cache.
     */
    static void notifyAll(int memoryLevel);

    /**
     * @brief Callback notification when the app goes to background.
     * Used to clear memory cache.
     */
    static void notifyBackground();

private:
    static std::vector<Callback> s_callbacks;
    static std::mutex s_mutex;
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_MEM_CACHE_CALLBACK_H
