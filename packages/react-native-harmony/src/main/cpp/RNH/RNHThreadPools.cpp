/**
 * RNOH-New: 双线程池单例实现
 *
 *  - RNHThreadPool：通用固定大小线程池
 *  - RNHThreadPools：全局单例，持有 ioPool 和 cpuPool
 */

#include "RNHThreadPools.h"

#include <cassert>

namespace rnoh_new {

// ===========================================================================
// RNHThreadPool 实现
// ===========================================================================

RNHThreadPool::RNHThreadPool(size_t numThreads) {
    assert(numThreads > 0 && "thread pool must have at least 1 thread");

    workers_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] {
            // 工作线程主循环
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    // 等待：有新任务 或 停止标志被设置
                    condition_.wait(lock, [this] {
                        return stop_.load() || !tasks_.empty();
                    });

                    if (stop_.load() && tasks_.empty()) {
                        // 停止且队列空 → 退出线程
                        return;
                    }

                    // 取出一个任务
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                // 执行任务（在锁外执行，避免持锁时间过长）
                try {
                    task();
                } catch (...) {
                    // 任务异常由 std::future 捕获并在 get() 时重抛，
                    // 这里静默吞掉，避免线程崩溃
                }
            }
        });
    }
}

RNHThreadPool::~RNHThreadPool() {
    // 设置停止标志
    stop_.store(true);
    // 通知所有等待线程
    condition_.notify_all();
    // 等待所有线程退出
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t RNHThreadPool::queuedCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

// ===========================================================================
// RNHThreadPools 单例实现
// ===========================================================================

RNHThreadPools& RNHThreadPools::instance() {
    // Meyers' Singleton：C++11 保证静态局部变量初始化线程安全
    static RNHThreadPools instance;
    return instance;
}

RNHThreadPools::RNHThreadPools() {
    // IO 池：默认 4 线程（适合阻塞型 IO 操作）
    constexpr size_t kIoPoolSize = 4;
    ioPool_ = std::make_unique<RNHThreadPool>(kIoPoolSize);

    // CPU 池：默认 CPU 核心数（适合计算密集型操作）
    size_t cpuCount = std::thread::hardware_concurrency();
    if (cpuCount == 0) {
        cpuCount = 2; // 兜底：获取失败时默认 2 线程
    }
    cpuPool_ = std::make_unique<RNHThreadPool>(cpuCount);
}

RNHThreadPools::~RNHThreadPools() {
    // 显式停止（unique_ptr 析构也会触发，这里只是为了明确顺序）
    shutdown();
}

void RNHThreadPools::shutdown() {
    if (ioPool_) {
        ioPool_.reset();
    }
    if (cpuPool_) {
        cpuPool_.reset();
    }
}

} // namespace rnoh_new
