/**
 * RNOH-New: 双线程池单例
 *
 * 提供两个全局线程池：
 *  - ioPool：  IO 密集型任务（文件读写、网络请求等），默认 4 线程
 *  - cpuPool： CPU 密集型任务（计算、编解码等），默认 CPU 核心数
 *
 * 线程池基于 std::thread + std::queue + condition_variable 实现，
 * submit() 返回 std::future，可等待结果或链式调用。
 *
 * 用法示例：
 *   auto& pool = RNHThreadPools::instance().ioPool();
 *   auto future = pool.submit([]() { return computeHeavy(); });
 *   int result = future.get();
 *
 * 注意：
 *  - 单例在首次访问时创建，程序退出时自动析构（join 所有线程）
 *  - 任务抛异常会被 std::future 捕获，调用 .get() 时重新抛出
 */

#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <memory>
#include <atomic>
#include <cstddef>

namespace rnoh_new {

/**
 * 通用线程池实现
 *
 * 固定数量的工作线程，从任务队列中取任务执行。
 * 支持 submit() 提交任务并返回 std::future。
 */
class RNHThreadPool {
public:
    /**
     * 构造函数：启动 numThreads 个工作线程
     */
    explicit RNHThreadPool(size_t numThreads);

    /**
     * 析构函数：停止所有线程并 join
     */
    ~RNHThreadPool();

    // 禁止拷贝
    RNHThreadPool(const RNHThreadPool&) = delete;
    RNHThreadPool& operator=(const RNHThreadPool&) = delete;

    /**
     * 提交任务到线程池
     * @tparam F  可调用对象类型
     * @tparam Args 参数类型
     * @param f    任务函数
     * @param args 参数
     * @return std::future<ReturnType>，可通过 get() 等待结果
     */
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /// 当前排队的任务数（近似值，仅供监控）
    size_t queuedCount() const;

    /// 线程数
    size_t threadCount() const { return workers_.size(); }

private:
    std::vector<std::thread> workers_;       ///< 工作线程
    std::queue<std::function<void()>> tasks_; ///< 任务队列
    mutable std::mutex queueMutex_;           ///< 队列锁
    std::condition_variable condition_;       ///< 条件变量（通知有新任务）
    std::atomic<bool> stop_{false};           ///< 停止标志
};

/**
 * 全局双线程池单例
 *
 * 提供 ioPool 和 cpuPool 两个专用线程池，避免 IO 任务阻塞 CPU 任务。
 */
class RNHThreadPools {
public:
    /// 获取单例实例（首次调用时创建）
    static RNHThreadPools& instance();

    /// IO 密集型线程池（文件、网络等阻塞操作）
    RNHThreadPool& ioPool() { return *ioPool_; }

    /// CPU 密集型线程池（计算、编解码等）
    RNHThreadPool& cpuPool() { return *cpuPool_; }

    /// 停止所有线程池（一般不需要手动调用，析构时自动执行）
    void shutdown();

private:
    RNHThreadPools();
    ~RNHThreadPools();

    // 禁止拷贝
    RNHThreadPools(const RNHThreadPools&) = delete;
    RNHThreadPools& operator=(const RNHThreadPools&) = delete;

    std::unique_ptr<RNHThreadPool> ioPool_;
    std::unique_ptr<RNHThreadPool> cpuPool_;
};

// ---------------------------------------------------------------------------
// 模板成员实现（必须在头文件中）
// ---------------------------------------------------------------------------

template <typename F, typename... Args>
auto RNHThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    // 用 packaged_task 包装任务，获得 future
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stop_.load()) {
            // 已停止，不接受新任务
            // 抛出异常，调用方可通过 future 捕获
            throw std::runtime_error("RNHThreadPool: submit on stopped pool");
        }
        tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return result;
}

} // namespace rnoh_new
