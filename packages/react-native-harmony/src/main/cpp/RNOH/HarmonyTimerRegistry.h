/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <react/runtime/PlatformTimerRegistry.h>
#include <react/runtime/TimerManager.h>
#include <mutex>
#include "RNOH/ArkTSMessageHub.h"
#include "TaskExecutor/TaskExecutor.h"
#include "VSyncListener.h"

namespace rnoh {

class HarmonyTimerRegistry final
    : public facebook::react::PlatformTimerRegistry {
 public:
  HarmonyTimerRegistry(
      TaskExecutor::Shared taskExecutor,
      ArkTSMessageHub::Shared arkTsMessageHub);

  ~HarmonyTimerRegistry() noexcept override;

  void createTimer(uint32_t timerId, double delayMs) override;

  void deleteTimer(uint32_t timerId) override;

  void createRecurringTimer(uint32_t timerId, double delayMs) override;

  void setTimerManager(
      std::weak_ptr<facebook::react::TimerManager> timerManager);

 private:
  /**
   * @thread: MAIN
   * This class listens for lifecycle events and pauses/resumes timers.
   */
  class LifecycleObserver : public ArkTSMessageHub::Observer {
   public:
    LifecycleObserver(
        HarmonyTimerRegistry* timerRegistry,
        ArkTSMessageHub::Shared const& messageHub)
        : ArkTSMessageHub::Observer(messageHub),
          m_timerRegistry(timerRegistry) {}
    void detach();
    void onMessageReceived(ArkTSMessage const& message) override;

   private:
    std::mutex m_timerRegistryMtx;
    HarmonyTimerRegistry* m_timerRegistry;
  };

  TaskExecutor::Shared m_taskExecutor;

  struct Timer {
    uint32_t id;
    double deadlineMs;
    double durationMs;
    bool repeats;
  };

  void triggerExpiredTimers();
  void triggerTimers(std::vector<uint32_t> const& timerIds);
  void resumeTimers();
  void pauseTimers();
  void scheduleWakeUp();
  void cancelWakeUp();
  double getNextDeadline();

  void onForeground();
  void onBackground();

  void assertJSThread() const;

  std::weak_ptr<HarmonyTimerRegistry> getWeakSelf();

  bool isForeground{true};
  std::shared_ptr<VSyncListener> m_vsyncListener =
      std::make_shared<VSyncListener>("HarmonyTimerRegistry");
  std::optional<TaskExecutor::DelayedTask> m_wakeUpTask = std::nullopt;
  double m_nextTimerDeadline = std::numeric_limits<double>::max();
  std::unordered_map<uint32_t, Timer> m_activeTimerById{};
  std::shared_ptr<LifecycleObserver> m_lifecycleObserver = nullptr;
  std::weak_ptr<facebook::react::TimerManager> m_timerManager{};
};

} // namespace rnoh
