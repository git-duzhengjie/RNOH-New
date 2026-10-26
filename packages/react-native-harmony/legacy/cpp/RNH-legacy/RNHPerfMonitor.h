/**
 * RNOH-New: 性能监控器（C++ 侧）
 *
 * 提供 FPS 计数、内存统计、启动时间线打点等能力。
 * 支持 getStats() 输出 JSON 格式统计数据。
 *
 * 设计原则：
 * - 轻量、零额外依赖（仅使用标准库与 POSIX 时间函数）
 * - 单例模式，全局可访问
 * - 线程安全（关键数据加锁）
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace rnoh_new {

/**
 * 性能监控器
 */
class RNHPerfMonitor {
 public:
  /**
   * 获取全局单例
   */
  static RNHPerfMonitor& getInstance();

  // --- FPS 计数 ---

  /**
   * 每帧调用一次，用于统计帧率
   */
  void tick();

  /**
   * 获取最近 1 秒内的平均 FPS
   */
  double getFps() const;

  // --- 内存统计 ---

  /**
   * 获取原生堆总大小（字节）
   * 基于 mallinfo 或自定义统计，不支持时返回 0
   */
  size_t getNativeHeapSize() const;

  /**
   * 获取原生堆已分配大小（字节）
   */
  size_t getNativeHeapAllocatedSize() const;

  /**
   * 获取当前进程 RSS（常驻内存，字节）
   * 读取 /proc/self/status，不支持时返回 0
   */
  size_t getResidentSetSize() const;

  // --- 启动时间线 ---

  /**
   * 标记 T0：进程启动 / 库加载时刻
   */
  void markT0();

  /**
   * 标记 TI：JS 引擎初始化完成时刻
   */
  void markTI();

  /**
   * 标记 TTI：首屏可交互时刻
   */
  void markTTI();

  /**
   * 获取 T0 → TI 耗时（毫秒）
   */
  double getT0ToTI() const;

  /**
   * 获取 TI → TTI 耗时（毫秒）
   */
  double getTIToTTI() const;

  /**
   * 获取 T0 → TTI 总耗时（毫秒）
   */
  double getT0ToTTI() const;

  // --- 统计输出 ---

  /**
   * 获取 JSON 格式的性能统计
   * 格式示例：
   * {
   *   "fps": 59.8,
   *   "memory": { "heapSize": 1048576, "allocated": 524288, "rss": 8388608 },
   *   "startup": { "t0ToTI": 120.5, "tiToTTI": 80.3, "t0ToTTI": 200.8 }
   * }
   */
  std::string getStats() const;

  // --- 自定义打点 ---

  /**
   * 记录一个自定义时间点（用于扩展场景）
   * @param name 打点名称
   */
  void markCustom(const std::string& name);

  /**
   * 获取自定义打点列表（名称 → 相对 T0 的毫秒数）
   */
  std::vector<std::pair<std::string, double>> getCustomMarks() const;

 private:
  RNHPerfMonitor();
  ~RNHPerfMonitor() = default;
  RNHPerfMonitor(const RNHPerfMonitor&) = delete;
  RNHPerfMonitor& operator=(const RNHPerfMonitor&) = delete;

  // 获取当前时间点（毫秒，稳定时钟）
  static double nowMs();

  // FPS 相关
  mutable std::mutex fpsMutex_;
  std::vector<double> frameTimestamps_;  // 最近 1 秒内的帧时间戳（ms）

  // 启动时间线
  std::atomic<double> t0_;
  std::atomic<double> ti_;
  std::atomic<double> tti_;

  // 自定义打点
  mutable std::mutex customMutex_;
  std::vector<std::pair<std::string, double>> customMarks_;
};

}  // namespace rnoh_new
