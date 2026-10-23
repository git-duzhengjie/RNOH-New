/**
 * RNOH-New: 固定大小内存池实现
 *
 * 预分配一块连续内存，按固定大小切分为若干块，
 * 使用栈式空闲链表管理，O(1) 分配与回收。
 */

#include "RNHMemoryPool.h"

#include <cassert>
#include <cstring>
#include <algorithm>

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHMemoryPool::RNHMemoryPool(size_t blockSize, size_t blockCount)
    : blockSize_(blockSize),
      blockCount_(blockCount),
      buffer_(blockSize * blockCount, 0),
      freeList_(blockCount) {
    assert(blockSize > 0 && "blockSize must be > 0");
    assert(blockCount > 0 && "blockCount must be > 0");

    base_ = buffer_.data();
    end_ = base_ + blockSize_ * blockCount_;

    // 初始化空闲栈：每个块的指针按顺序入栈
    // 栈顶 = 最后一个块（LIFO，分配时从末尾取）
    for (size_t i = 0; i < blockCount_; ++i) {
        freeList_[i] = base_ + i * blockSize_;
    }
}

RNHMemoryPool::~RNHMemoryPool() {
    // buffer_ 自动释放
}

// ---------------------------------------------------------------------------
// 移动构造与移动赋值
// ---------------------------------------------------------------------------

RNHMemoryPool::RNHMemoryPool(RNHMemoryPool&& other) noexcept
    : blockSize_(other.blockSize_),
      blockCount_(other.blockCount_),
      buffer_(std::move(other.buffer_)),
      freeList_(std::move(other.freeList_)) {
    base_ = buffer_.data();
    end_ = base_ + blockSize_ * blockCount_;

    // 重置 other 的状态
    other.blockSize_ = 0;
    other.blockCount_ = 0;
    other.base_ = nullptr;
    other.end_ = nullptr;
}

RNHMemoryPool& RNHMemoryPool::operator=(RNHMemoryPool&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::lock_guard<std::mutex> otherLock(other.mutex_);

        blockSize_ = other.blockSize_;
        blockCount_ = other.blockCount_;
        buffer_ = std::move(other.buffer_);
        freeList_ = std::move(other.freeList_);
        base_ = buffer_.data();
        end_ = base_ + blockSize_ * blockCount_;

        other.blockSize_ = 0;
        other.blockCount_ = 0;
        other.base_ = nullptr;
        other.end_ = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// 分配与回收
// ---------------------------------------------------------------------------

void* RNHMemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (freeList_.empty()) {
        // 池已满，返回 nullptr（调用方应回退到 malloc 或报错）
        return nullptr;
    }

    // 从栈顶取一个空闲块
    void* ptr = freeList_.back();
    freeList_.pop_back();
    return ptr;
}

void RNHMemoryPool::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // 安全检查：指针必须属于本内存池
    assert(owns(ptr) && "deallocate: ptr does not belong to this pool");

    // 检查是否已在空闲列表中（防双重释放，调试模式下断言）
    // （生产模式下不做线性扫描，避免 O(n) 开销）
#ifdef NDEBUG
    (void)ptr; // 消除未使用警告
#else
    for (const auto* p : freeList_) {
        assert(p != ptr && "double deallocate detected");
    }
#endif

    freeList_.push_back(ptr);
}

// ---------------------------------------------------------------------------
// 统计与查询
// ---------------------------------------------------------------------------

size_t RNHMemoryPool::usedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return blockCount_ - freeList_.size();
}

size_t RNHMemoryPool::freeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return freeList_.size();
}

bool RNHMemoryPool::owns(const void* ptr) const {
    if (ptr == nullptr) {
        return false;
    }
    const uint8_t* p = static_cast<const uint8_t*>(ptr);
    // 指针必须在 [base_, end_) 范围内，且对齐到 blockSize 边界
    if (p < base_ || p >= end_) {
        return false;
    }
    size_t offset = static_cast<size_t>(p - base_);
    return (offset % blockSize_) == 0;
}

} // namespace rnoh_new
