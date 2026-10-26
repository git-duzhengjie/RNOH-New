/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <glog/logging.h>
#include <mutex>
#include <vector>
#include "EventEmitRequestHandler.h"

namespace rnoh {

class EventDispatcher {
 public:
  using Context = EventEmitRequestHandler::Context;

  void sendEvent(Context const& ctx) {
    // Copy under lock so concurrent register/unregister cannot invalidate
    // iterators. Also avoids reentrancy issues if handleEvent mutates the list.
    std::vector<EventEmitRequestHandler::Weak> handlersCopy;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      handlersCopy = m_requestHandlers;
    }
    for (auto& weakHandler : handlersCopy) {
      if (auto handler = weakHandler.lock()) {
        handler->handleEvent(ctx);
      }
    }
  }

  void registerEventListener(EventEmitRequestHandler::Shared const& handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requestHandlers.push_back(handler);
  }

  void unregisterEventListener(EventEmitRequestHandler::Shared const& handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(
        m_requestHandlers.begin(),
        m_requestHandlers.end(),
        [&handler](auto& weakHandler) {
          return weakHandler.expired() || weakHandler.lock() == handler;
        });
    if (it != m_requestHandlers.end()) {
      m_requestHandlers.erase(it, m_requestHandlers.end());
    } else {
      LOG(ERROR) << "Trying to unregister a non-registered listener";
    }
  }

  void unregisterExpiredListeners() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(
        m_requestHandlers.begin(),
        m_requestHandlers.end(),
        [](auto& weakHandler) { return weakHandler.expired(); });
    if (it != m_requestHandlers.end()) {
      m_requestHandlers.erase(it, m_requestHandlers.end());
    }
  }

 private:
  std::mutex m_mutex;
  std::vector<EventEmitRequestHandler::Weak> m_requestHandlers;
};

} // namespace rnoh