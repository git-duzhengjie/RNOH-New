/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "EventBeat.h"
#include "react/renderer/runtimescheduler/RuntimeScheduler.h"

namespace rnoh {

EventBeat::EventBeat(
    std::shared_ptr<facebook::react::EventBeat::OwnerBox> ownerBox,
    std::shared_ptr<facebook::react::RuntimeScheduler> runtimeScheduler,
    UITicker::Shared uiTicker)
    : facebook::react::EventBeat(std::move(ownerBox), *runtimeScheduler),
      m_uiTicker(std::move(uiTicker)),
      m_weakRuntimeScheduler(runtimeScheduler) {}

/**
 * @thread MAIN
 */
EventBeat::~EventBeat() {
  std::lock_guard lock(m_unsubscribeUITickerListenerMtx);
  if (m_unsubscribeUITickerListener != nullptr) {
    m_unsubscribeUITickerListener();
  }
}

/**
 * @thread JS
 */
void EventBeat::request() const {
  facebook::react::EventBeat::request();
  std::lock_guard lock(m_unsubscribeUITickerListenerMtx);
  if (m_unsubscribeUITickerListener != nullptr) {
    return;
  }
  m_unsubscribeUITickerListener =
      m_uiTicker->subscribe([this](UITicker::Timestamp timestamp) {
        this->induce();
        auto lock = std::lock_guard(m_unsubscribeUITickerListenerMtx);
        if (m_unsubscribeUITickerListener != nullptr) {
          m_unsubscribeUITickerListener();
          m_unsubscribeUITickerListener = nullptr;
        }
      });
}

void EventBeat::induce() const {
  // This is the safety check that runs on the VSync thread.
  // The temporary `runtimeScheduler` shared_ptr keeps the object alive
  if (auto runtimeScheduler = m_weakRuntimeScheduler.lock()) {
    facebook::react::EventBeat::induce();
  }
}

} // namespace rnoh