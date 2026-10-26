/**
 * RNOH-New: 固定大小内存池
 *
 * 用于频繁分配/释放的小对象（如 MountingItem、ShadowNode 等），
 * 避免反复 malloc/free 带来的系统调用开销与内存碎片。
 *
 * 特性：
 *  - 预分配一块连续内存，按固定 blockSize 切分为 blockCount 个块
 *  - 空闲块用栈式管理（O(1) 分配/回收）
 *  - 线程安全（std::mutex 保护）
 *  - 不调用构造/析构函数（纯内存分配，配合 placement new 使用）
 *
 * 用法示例：
 *   RNHMemoryPool pool(sizeof(MyObj), 1024);
 *   void* p = pool.allocate();
 *   MyObj* obj = new (p) MyObj(...);
 *   obj->~MyObj();
 *   pool.deallocate(p);
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>

namespace rnoh_new {

class RNHMemoryPool {
public:
    /**
     * 构造函数
     * @param blockSize  每个内存块的字节数（必须 > 0）
     * @param blockCount 内存块总数（必须 > 0）
     */
    RNHMemoryPool(size_t blockSize, size_t blockCount);

    ~RNHMemoryPool();

    // 禁止拷贝（持有原始内存指针，拷贝语义不明确）
    RNHMemoryPool(const RNHMemoryPool&) = delete;
    RNHMemoryPool& operator=(const RNHMemoryPool&) = delete;

    // 允许移动
    RNHMemoryPool(RNHMemoryPool&& other) noexcept;
    RNHMemoryPool& operator=(RNHMemoryPool&& other) noexcept;

    /**
     * 分配一个内存块
     * @return 成功返回内存块指针，池满返回 nullptr
     */
    void* allocate();

    /**
     * 回收一个内存块
     * @param ptr 必须是本池 allocate() 返回的指针，否则行为未定义
     */
    void deallocate(void* ptr);

    // -----------------------------------------------------------------------
    // 统计与查询
    // -----------------------------------------------------------------------

    /// 每个块的字节数
    size_t blockSize() const { return blockSize_; }

    /// 总块数
    size_t blockCount() const { return blockCount_; }

    /// 当前已分配块数（近似值，多线程下仅供参考）
    size_t usedCount() const;

    /// 当前空闲块数（近似值，多线程下仅供参考）
    size_t freeCount() const;

    /// 检查指针是否属于本内存池（用于调试/断言）
    bool owns(const void* ptr) const;

private:
    size_t blockSize_;           ///< 每个块的字节数
    size_t blockCount_;          ///< 总块数
    std::vector<uint8_t> buffer_;///< 连续内存缓冲区
    std::vector<void*> freeList_;///< 空闲块栈（栈顶 = 下一个可分配块）
    mutable std::mutex mutex_;   ///< 线程安全锁

    /// 缓冲区起始地址（便于 owns() 检查）
    uint8_t* base_ = nullptr;
    /// 缓冲区结束地址（base_ + blockSize_ * blockCount_）
    uint8_t* end_ = nullptr;
};

} // namespace rnoh_new
