/**
 * RNOH-New: 性能监控器（C++ 侧实现）
 *
 * 轻量性能统计，零外部依赖。
 * 内存统计优先使用 mallinfo（POSIX），不可用时返回 0。
 */

#include "RNHPerfMonitor.h"

#include <algorithm>
#include <sstream>
#include <string>

#if defined(__unix__) || defined(__APPLE__) || defined(__OHOS__) || defined(__OpenHarmony__)
#include <malloc.h>
#include <sys/resource.h>
#include <unistd.h>
#define RNH_HAS_MALLINFO 1
#else
#define RNH_HAS_MALLINFO 0
#endif

namespace rnoh_new {

namespace {

// 滑动窗口大小（秒）
constexpr double kFpsWindowSeconds = 1.0;

}  // namespace

// ---------- 单例 ----------

RNHPerfMonitor& RNHPerfMonitor::getInstance() {
  static RNHPerfMonitor instance;
  return instance;
}

RNHPerfMonitor::RNHPerfMonitor()
    : t0_(0.0), ti_(0.0), tti_(0.0) {}

// ---------- 时间工具 ----------

double RNHPerfMonitor::nowMs() {
  auto now = std::chrono::steady_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch())
                .count();
  return static_cast<double>(ms) / 1000.0;
}

// ---------- FPS ----------

void RNHPerfMonitor::tick() {
  const double now = nowMs();
  std::lock_guard<std::mutex> lock(fpsMutex_);
  frameTimestamps_.push_back(now);

  // 移除 1 秒窗口外的旧帧
  const double cutoff = now - kFpsWindowSeconds * 1000.0;
  while (!frameTimestamps_.empty() && frameTimestamps_.front() < cutoff) {
    frameTimestamps_.erase(frameTimestamps_.begin());
  }
}

double RNHPerfMonitor::getFps() const {
  std::lock_guard<std::mutex> lock(fpsMutex_);
  if (frameTimestamps_.size() < 2) {
    return 0.0;
  }
  const double spanMs =
      frameTimestamps_.back() - frameTimestamps_.front();
  if (spanMs <= 0.0) {
    return 0.0;
  }
  // 帧数 / 时间（秒）
  const double frames = static_cast<double>(frameTimestamps_.size() - 1);
  return frames * 1000.0 / spanMs;
}

// ---------- 内存 ----------

size_t RNHPerfMonitor::getNativeHeapSize() const {
#if RNH_HAS_MALLINFO
  struct mallinfo info = mallinfo();
  return static_cast<size_t>(info.arena);
#else
  return 0;
#endif
}

size_t RNHPerfMonitor::getNativeHeapAllocatedSize() const {
#if RNH_HAS_MALLINFO
  struct mallinfo info = mallinfo();
  return static_cast<size_t>(info.uordblks);
#else
  return 0;
#endif
}

size_t RNHPerfMonitor::getResidentSetSize() const {
#if defined(__unix__) || defined(__OHOS__) || defined(__OpenHarmony__)
  // 读取 /proc/self/status 中的 VmRSS
  FILE* fp = fopen("/proc/self/status", "r");
  if (!fp) {
    return 0;
  }
  char line[256];
  size_t rssKb = 0;
  while (fgets(line, sizeof(line), fp) != nullptr) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      // 格式: VmRSS:    12345 kB
      const char* p = line + 6;
      while (*p && (*p == ' ' || *p == '\t')) ++p;
      rssKb = static_cast<size_t>(strtoul(p, nullptr, 10));
      break;
    }
  }
  fclose(fp);
  return rssKb * 1024;
#else
  return 0;
#endif
}

// ---------- 启动时间线 ----------

void RNHPerfMonitor::markT0() {
  t0_.store(nowMs());
}

void RNHPerfMonitor::markTI() {
  ti_.store(nowMs());
}

void RNHPerfMonitor::markTTI() {
  tti_.store(nowMs());
}

double RNHPerfMonitor::getT0ToTI() const {
  const double t0 = t0_.load();
  const double ti = ti_.load();
  return (t0 > 0.0 && ti > 0.0) ? (ti - t0) : 0.0;
}

double RNHPerfMonitor::getTIToTTI() const {
  const double ti = ti_.load();
  const double tti = tti_.load();
  return (ti > 0.0 && tti > 0.0) ? (tti - ti) : 0.0;
}

double RNHPerfMonitor::getT0ToTTI() const {
  const double t0 = t0_.load();
  const double tti = tti_.load();
  return (t0 > 0.0 && tti > 0.0) ? (tti - t0) : 0.0;
}

// ---------- 自定义打点 ----------

void RNHPerfMonitor::markCustom(const std::string& name) {
  const double now = nowMs();
  std::lock_guard<std::mutex> lock(customMutex_);
  customMarks_.emplace_back(name, now);
}

std::vector<std::pair<std::string, double>>
RNHPerfMonitor::getCustomMarks() const {
  std::lock_guard<std::mutex> lock(customMutex_);
  const double t0 = t0_.load();
  std::vector<std::pair<std::string, double>> result;
  result.reserve(customMarks_.size());
  for (const auto& mark : customMarks_) {
    result.emplace_back(mark.first,
                        t0 > 0.0 ? (mark.second - t0) : mark.second);
  }
  return result;
}

// ---------- JSON 输出 ----------

std::string RNHPerfMonitor::getStats() const {
  std::ostringstream oss;
  oss.precision(2);
  oss << std::fixed;

  oss << "{";

  // FPS
  oss << "\"fps\":" << getFps() << ",";

  // 内存
  oss << "\"memory\":{";
  oss << "\"heapSize\":" << getNativeHeapSize() << ",";
  oss << "\"allocated\":" << getNativeHeapAllocatedSize() << ",";
  oss << "\"rss\":" << getResidentSetSize();
  oss << "},";

  // 启动时间线
  oss << "\"startup\":{";
  oss << "\"t0ToTI\":" << getT0ToTI() << ",";
  oss << "\"tiToTTI\":" << getTIToTTI() << ",";
  oss << "\"t0ToTTI\":" << getT0ToTTI();
  oss << "},";

  // 自定义打点
  const auto marks = getCustomMarks();
  oss << "\"custom\":[";
  for (size_t i = 0; i < marks.size(); ++i) {
    if (i > 0) oss << ",";
    oss << "{\"name\":\"" << marks[i].first << "\",\"t\":" << marks[i].second
        << "}";
  }
  oss << "]";

  oss << "}";
  return oss.str();
}

}  // namespace rnoh_new
