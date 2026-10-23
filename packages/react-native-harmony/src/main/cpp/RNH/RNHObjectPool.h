/**
 * RNOH-New: 通用对象池模板
 *
 * 用于频繁创建/销毁的小对象（如 MountingItem、ShadowNode、事件对象等），
 * 避免反复分配/释放内存带来的开销与碎片。
 *
 * 特性：
 *  - 模板类，支持任意可默认构造的类型 T
 *  - acquire() 获取对象，release() 归还对象到池中
 *  - 内部用 std::stack 存储空闲对象（LIFO，缓存友好）
 *  - 支持最大池大小配置（超过上限的归还直接 delete）
 *  - 线程安全（std::mutex 保护）
 *  - 提供 make_unique() 辅助，返回带自定义 deleter 的 unique_ptr，
 *    离开作用域自动归还到池
 *
 * 用法示例：
 *   RNHObjectPool<MyObj> pool(64); // 最多缓存 64 个
 *   auto obj = pool.acquire();      // 获取一个对象
 *   obj->doSomething();
 *   pool.release(obj);              // 归还（或 unique_ptr 自动归还）
 *
 *   // 更安全的 unique_ptr 方式：
 *   auto ptr = pool.make_unique();
 *   ptr->doSomething();
 *   // ptr 析构时自动归还到池
 */

#pragma once

#include <stack>
#include <mutex>
#include <memory>
#include <utility>
#include <cassert>

namespace rnoh_new {

template <typename T>
class RNHObjectPool {
public:
    /**
     * 构造函数
     * @param maxPoolSize 池中最多缓存的对象数量（0 表示不限制）
     */
    explicit RNHObjectPool(size_t maxPoolSize = 0)
        : maxPoolSize_(maxPoolSize) {}

    ~RNHObjectPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        // 析构时释放池中所有缓存对象
        while (!pool_.empty()) {
            T* obj = pool_.top();
            pool_.pop();
            delete obj;
        }
    }

    // 禁止拷贝
    RNHObjectPool(const RNHObjectPool&) = delete;
    RNHObjectPool& operator=(const RNHObjectPool&) = delete;

    /**
     * 从池中获取一个对象
     * - 池中有空闲对象 → 取出并返回
     * - 池中没有 → new 一个新对象
     * @return 对象指针（调用方负责通过 release() 归还）
     */
    T* acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pool_.empty()) {
            T* obj = pool_.top();
            pool_.pop();
            return obj;
        }
        // 池空，分配新对象
        return new T();
    }

    /**
     * 归还对象到池中
     * - 池未满 → 入栈缓存
     * - 池已满 → 直接 delete
     * @param obj 必须是本池 acquire() 返回的指针
     */
    void release(T* obj) {
        if (obj == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (maxPoolSize_ == 0 || pool_.size() < maxPoolSize_) {
            pool_.push(obj);
        } else {
            delete obj;
        }
    }

    /**
     * 自定义 deleter，用于 unique_ptr 自动归还
     */
    struct Deleter {
        RNHObjectPool* pool;

        explicit Deleter(RNHObjectPool* p) : pool(p) {}

        void operator()(T* obj) const {
            if (pool && obj) {
                pool->release(obj);
            }
        }
    };

    using UniquePtr = std::unique_ptr<T, Deleter>;

    /**
     * 获取对象并包装为 unique_ptr，离开作用域自动归还
     * @return 带自定义 deleter 的 unique_ptr
     */
    UniquePtr make_unique() {
        T* obj = acquire();
        return UniquePtr(obj, Deleter(this));
    }

    // -----------------------------------------------------------------------
    // 统计与查询
    // -----------------------------------------------------------------------

    /// 池中当前缓存的对象数（近似值，多线程下仅供参考）
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    /// 最大池大小（0 = 不限制）
    size_t maxPoolSize() const { return maxPoolSize_; }

    /// 清空池中所有缓存对象
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            T* obj = pool_.top();
            pool_.pop();
            delete obj;
        }
    }

private:
    size_t maxPoolSize_;       ///< 最大缓存数量（0 = 不限制）
    std::stack<T*> pool_;      ///< 空闲对象栈
    mutable std::mutex mutex_; ///< 线程安全锁
};

} // namespace rnoh_new
