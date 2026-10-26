/**
 * RNOH-New: 性能剖析器实现
 *
 * 单例管理所有计时记录，支持嵌套栈与聚合统计。
 */

#include "RNHPerfProfiler.h"

#include <cstdio>
#include <algorithm>
#include <cassert>

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 单例
// ---------------------------------------------------------------------------

RNHPerfProfiler& RNHPerfProfiler::instance() {
    static RNHPerfProfiler inst;
    return inst;
}

// ---------------------------------------------------------------------------
// 作用域管理
// ---------------------------------------------------------------------------

std::chrono::high_resolution_clock::time_point
RNHPerfProfiler::beginScope(const std::string& name) {
    auto now = std::chrono::high_resolution_clock::now();

    if (!enabled_) {
        return now;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    scopeStack_.push_back(name);
    return now;
}

void RNHPerfProfiler::endScope(
    const std::string& name,
    std::chrono::high_resolution_clock::time_point start) {
    if (!enabled_) {
        return;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs =
        std::chrono::duration<double, std::milli>(end - start).count();

    std::lock_guard<std::mutex> lock(mutex_);

    // 从栈顶弹出（应当与 name 匹配）
    if (!scopeStack_.empty() && scopeStack_.back() == name) {
        scopeStack_.pop_back();
    }
    // depth = 弹出后的栈大小（即进入该 scope 时的栈大小）
    int depth = static_cast<int>(scopeStack_.size());

    // 更新记录
    auto it = records_.find(name);
    if (it == records_.end()) {
        RNHPerfRecord rec;
        rec.name = name;
        rec.depth = depth;
        rec.totalMs = elapsedMs;
        rec.maxMs = elapsedMs;
        rec.minMs = elapsedMs;
        rec.callCount = 1;
        records_.emplace(name, rec);

        // 输出回调
        if (outputCb_) {
            outputCb_(rec);
        }
    } else {
        auto& rec = it->second;
        rec.totalMs += elapsedMs;
        rec.callCount += 1;
        if (elapsedMs > rec.maxMs) rec.maxMs = elapsedMs;
        if (elapsedMs < rec.minMs) rec.minMs = elapsedMs;

        // 输出回调（使用更新后的记录）
        if (outputCb_) {
            outputCb_(rec);
        }
    }
}

// ---------------------------------------------------------------------------
// 查询与统计
// ---------------------------------------------------------------------------

std::vector<RNHPerfRecord> RNHPerfProfiler::allRecords() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RNHPerfRecord> result;
    result.reserve(records_.size());
    for (const auto& [name, rec] : records_) {
        result.push_back(rec);
    }
    // 按总耗时降序排列，方便查看热点
    std::sort(result.begin(), result.end(),
              [](const RNHPerfRecord& a, const RNHPerfRecord& b) {
                  return a.totalMs > b.totalMs;
              });
    return result;
}

const RNHPerfRecord* RNHPerfProfiler::getRecord(
    const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(name);
    if (it != records_.end()) {
        return &it->second;
    }
    return nullptr;
}

int RNHPerfProfiler::currentDepth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(scopeStack_.size());
}

void RNHPerfProfiler::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
    scopeStack_.clear();
}

} // namespace rnoh_new
