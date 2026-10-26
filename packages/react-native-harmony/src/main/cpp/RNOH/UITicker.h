/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <folly/Function.h>
#include <glog/logging.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "NativeVsyncHandle.h"
#include "RNOH/ArkTSMessageHub.h"
#include "TaskExecutor/TaskExecutor.h"

namespace rnoh {
/**
 * @internal
 */
class UITicker : public std::enable_shared_from_this<UITicker> {
  using Task = folly::Function<void()>;

 public:
  using Timestamp = std::chrono::
      time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

  static void onTick(long long _timestamp, void* data) {
    auto weakSelf = std::unique_ptr<std::weak_ptr<UITicker>>(
        static_cast<std::weak_ptr<UITicker>*>(data));
    if (auto self = weakSelf->lock()) {
      self->m_isFrameScheduled.store(false, std::memory_order_release);
      self->tick(std::chrono::steady_clock::now());
    }
  }

  UITicker(
      TaskExecutor::Shared taskExecutor,
      ArkTSMessageHub::Shared arkTsMessageHub)
      : m_taskExecutor(std::move(taskExecutor)),
        m_vsyncHandle("UITicker"),
        m_lifecycleObserver(
            std::make_shared<LifecycleObserver>(this, arkTsMessageHub)) {}

  ~UITicker() noexcept {
    if (m_lifecycleObserver != nullptr) {
      m_lifecycleObserver->detach();
    }
    m_taskExecutor->runTask(
        TaskThread::MAIN,
        [lifecycleObserver = std::move(m_lifecycleObserver)] {});
  }

  using Shared = std::shared_ptr<UITicker>;

  std::function<void()> subscribe(std::function<void(Timestamp)>&& listener) {
    std::lock_guard lock(listenersMutex);
    auto id = m_nextListenerId;
    m_nextListenerId++;
    auto listenersCount = m_listenerById.size();
    m_listenerById.insert_or_assign(id, std::move(listener));
    if (listenersCount == 0) {
      this->requestNextTick();
    }
    return [id, this]() {
      std::lock_guard lock(listenersMutex);
      this->m_listenerById.erase(id);
    };
  }

  void runTask(Task&& task) {
    auto lock = std::lock_guard(m_taskMtx);
    m_tasks.push_back(std::move(task));
    if (m_tasks.size() == 0) {
      requestNextTick();
    }
  }

  std::optional<int64_t> getPeriodNs() const {
    return m_vsyncHandle.getPeriodNs();
  }

 private:
  class LifecycleObserver : public ArkTSMessageHub::Observer {
   public:
    LifecycleObserver(
        UITicker* uiTicker,
        ArkTSMessageHub::Shared const& messageHub)
        : ArkTSMessageHub::Observer(messageHub), m_uiTicker(uiTicker) {}

    void detach() {
      std::lock_guard lock(m_uiTickerMtx);
      m_uiTicker = nullptr;
    }

    void onMessageReceived(ArkTSMessage const& message) override {
      std::lock_guard lock(m_uiTickerMtx);
      if (m_uiTicker == nullptr) {
        return;
      }
      if (message.name == "FOREGROUND") {
        m_uiTicker->onForeground();
      } else if (message.name == "BACKGROUND") {
        m_uiTicker->onBackground();
      }
    }

   private:
    std::mutex m_uiTickerMtx;
    UITicker* m_uiTicker;
  };

  TaskExecutor::Shared m_taskExecutor;
  std::mutex m_taskMtx;
  std::vector<Task> m_tasks;
  std::unordered_map<int, std::function<void(Timestamp)>> m_listenerById;
  std::mutex listenersMutex;
  std::atomic<bool> m_isPaused{false};
  std::atomic<bool> m_isFrameScheduled{false};
  NativeVsyncHandle m_vsyncHandle;
  int m_nextListenerId = 0;
  std::shared_ptr<LifecycleObserver> m_lifecycleObserver = nullptr;

  void requestNextTick() {
    if (m_isFrameScheduled.exchange(true, std::memory_order_acq_rel)) {
      return;
    }
    auto data = std::make_unique<std::weak_ptr<UITicker>>(weak_from_this());
    auto ret = m_vsyncHandle.requestFrame(onTick, data.get());
    if (ret != 0) {
      LOG(ERROR) << "[UITicker] Failed to request VSync frame. Error code: "
                 << ret;
      m_isFrameScheduled.store(false, std::memory_order_release);
      return;
    }
    data.release();
  }

  void onForeground() {
    if (m_isPaused.exchange(false, std::memory_order_acq_rel)) {
      this->requestNextTick();
    }
  }

  void onBackground() {
    m_isPaused.store(true, std::memory_order_release);
  }

  void tick(Timestamp timestamp) {
    if (m_isPaused.load(std::memory_order_acquire)) {
      return;
    }

    {
      auto lock = std::lock_guard(m_taskMtx);
      for (auto& task : m_tasks) {
        task();
      }
      m_tasks.clear();
    }

    decltype(m_listenerById) listenerById;
    {
      std::lock_guard lock(listenersMutex);
      if (m_listenerById.empty()) {
        return;
      }
      /**
       * TODO: Eliminate copy by value
       * https://gl.swmansion.com/rnoh/react-native-harmony/-/issues/1478
       *
       * This change was introduced in the RN 0.77 update. As of this comment,
       * rnoh::EventBeat unsubscribes UITicker in a listener, causing a deadlock
       * if listenersMutex is locked during listener execution.
       */
      listenerById = m_listenerById;
    }
    for (const auto& idAndListener : listenerById) {
      idAndListener.second(timestamp);
    }
    if (!m_isPaused.load(std::memory_order_acquire)) {
      this->requestNextTick();
    }
  }
};

} // namespace rnoh
