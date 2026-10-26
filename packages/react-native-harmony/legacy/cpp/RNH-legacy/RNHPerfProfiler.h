/**
 * RNOH-New: 性能剖析器
 *
 * 提供 RAII 风格的作用域计时器，用于测量代码块的执行耗时。
 * 支持嵌套计时栈，可输出每个作用域的耗时、调用次数、平均耗时。
 *
 * 使用方式（推荐用宏）：
 *   void myFunction() {
 *       RNH_PERF_SCOPE("myFunction");  // 进入时开始计时，离开时结束
 *       // ... 业务代码
 *   }
 *
 * 宏开关：
 *   - 定义 RNH_PERF_PROFILE → 启用性能剖析
 *   - 未定义 → 所有宏展开为空，零运行时开销
 *
 * 特性：
 *  - RAII 计时器：构造开始，析构结束，异常安全
 *  - 嵌套计时栈：自动识别调用层级，输出树形结构
 *  - 统计聚合：同一名称的多次测量自动聚合（总耗时、次数、平均）
 *  - 可自定义输出回调（默认打印到 stderr）
 */

#pragma once

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <memory>

namespace rnoh_new {

/**
 * 单条计时记录
 */
struct RNHPerfRecord {
    std::string name;           ///< 计时器名称
    int depth = 0;              ///< 嵌套深度（0 = 最外层）
    double totalMs = 0.0;       ///< 总耗时（毫秒）
    double maxMs = 0.0;         ///< 单次最大耗时（毫秒）
    double minMs = 0.0;         ///< 单次最小耗时（毫秒）
    uint64_t callCount = 0;     ///< 调用次数
};

/**
 * 性能剖析器（单例）
 *
 * 管理所有计时记录，提供查询与输出接口。
 */
class RNHPerfProfiler {
public:
    /// 输出回调类型：(record) -> void
    using OutputCallback = std::function<void(const RNHPerfRecord&)>;

    /// 获取单例
    static RNHPerfProfiler& instance();

    /**
     * 开始一个计时作用域（由 RNHScopedTimer 调用）
     * @param name 作用域名称
     * @return 该作用域的起始时间（高分辨率时钟）
     */
    std::chrono::high_resolution_clock::time_point
    beginScope(const std::string& name);

    /**
     * 结束一个计时作用域（由 RNHScopedTimer 析构时调用）
     * @param name   作用域名称
     * @param start  起始时间
     */
    void endScope(const std::string& name,
                  std::chrono::high_resolution_clock::time_point start);

    /// 设置输出回调（每次 endScope 后触发）
    void setOutputCallback(OutputCallback cb) { outputCb_ = std::move(cb); }

    /// 获取所有记录（拷贝）
    std::vector<RNHPerfRecord> allRecords() const;

    /// 获取指定名称的记录（不存在返回 nullptr）
    const RNHPerfRecord* getRecord(const std::string& name) const;

    /// 清空所有记录
    void clear();

    /// 启用/禁用（禁用时 begin/end 直接返回，零开销）
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    /// 当前嵌套深度
    int currentDepth() const;

private:
    RNHPerfProfiler() = default;
    ~RNHPerfProfiler() = default;

    bool enabled_ = true;
    mutable std::mutex mutex_;

    /// 计时栈：保存当前嵌套的作用域名称（用于计算 depth）
    std::vector<std::string> scopeStack_;

    /// 记录字典：name → record
    std::unordered_map<std::string, RNHPerfRecord> records_;

    /// 输出回调
    OutputCallback outputCb_;
};

/**
 * RAII 作用域计时器
 *
 * 构造时开始计时，析构时结束计时并上报到 RNHPerfProfiler。
 * 异常安全：即使作用域因异常退出也能正确记录。
 */
class RNHScopedTimer {
public:
    explicit RNHScopedTimer(const std::string& name)
        : name_(name),
          start_(RNHPerfProfiler::instance().beginScope(name)) {}

    ~RNHScopedTimer() {
        RNHPerfProfiler::instance().endScope(name_, start_);
    }

    // 禁止拷贝
    RNHScopedTimer(const RNHScopedTimer&) = delete;
    RNHScopedTimer& operator=(const RNHScopedTimer&) = delete;

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
};

} // namespace rnoh_new

// ===========================================================================
// 宏定义
// ===========================================================================

/**
 * RNH_PERF_PROFILE 宏控制是否启用性能剖析。
 * 未定义时，所有宏展开为空，零运行时开销。
 */
#ifdef RNH_PERF_PROFILE

/// 在当前作用域启动一个计时器，离开作用域时自动结束
#define RNH_PERF_SCOPE(name) \
    ::rnoh_new::RNHScopedTimer _rnoh_perf_timer_##__LINE__(name)

/// 手动开始/结束（用于非作用域场景，尽量用 RNH_PERF_SCOPE 代替）
#define RNH_PERF_BEGIN(name)  ::rnoh_new::RNHPerfProfiler::instance().beginScope(name)
#define RNH_PERF_END(name, t) ::rnoh_new::RNHPerfProfiler::instance().endScope(name, t)

/// 启用/禁用
#define RNH_PERF_ENABLED(v) ::rnoh_new::RNHPerfProfiler::instance().setEnabled(v)

/// 设置输出回调
#define RNH_PERF_SET_OUTPUT(cb) \
    ::rnoh_new::RNHPerfProfiler::instance().setOutputCallback(cb)

#else // !RNH_PERF_PROFILE

// 禁用时全部展开为空
#define RNH_PERF_SCOPE(name)  ((void)0)
#define RNH_PERF_BEGIN(name)  (std::chrono::high_resolution_clock::time_point{})
#define RNH_PERF_END(name, t) ((void)0)
#define RNH_PERF_ENABLED(v)   ((void)0)
#define RNH_PERF_SET_OUTPUT(cb) ((void)0)

#endif // RNH_PERF_PROFILE
