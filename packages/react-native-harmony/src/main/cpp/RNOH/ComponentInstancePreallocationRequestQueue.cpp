/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ComponentInstancePreallocationRequestQueue.h"
#include "RNOH/FFRTConfig.h"
#include "RNOH/ParallelCheck.h"
#include "RNOH/ParallelComponent.h"
#include "RNOH/RNOHError.h"
#include "ffrt/cpp/pattern/job_partner.h"

namespace rnoh {

void ComponentInstancePreallocationRequestQueue::push(Request request) {
  if (!IsParallelizationEnabled()) {
    // Non-parallel path: enqueue the request and notify the delegate to drain
    // it. Return early so the moved-from `request` is never accessed again,
    // and the parallel branch (which would double-process it) never runs.
    auto lock = std::lock_guard(m_mtx);
    m_queue.push(std::move(request));
    auto delegate = m_weakDelegate.lock();
    if (delegate != nullptr) {
      try {
        delegate->onPushPreallocationRequest();
      } catch (const std::exception& e) {
        LOG(ERROR) << "onPushPreallocationRequest error: " << e.what();
      }
    }
    return;
  }
  auto delegate = m_weakDelegate.lock();
  if (delegate != nullptr) {
    if (IsParallelizationEnabled() &&
        (parallelComponentHandles.find(request.componentHandle) !=
             parallelComponentHandles.end() ||
         ComponentNameManager::getInstance().hasComponentName(
             request.componentName))) {
      auto job_partner =
          ffrt::job_partner<ScenarioID::PRECREATED_COMPONENTS_PARALLELIZATION>::
              get_main_partner(
                  ffrt::job_partner_attr()
                      .max_parallelism(MAX_THREAD_NUM_RECREATED_COMPONENTS)
                      .qos(THREAD_PRIORITY_LEVEL_5));
      job_partner->submit<true>([weakDelegate = m_weakDelegate,
                                 request = std::move(request)] {
        auto delegate = weakDelegate.lock();
        if (delegate == nullptr) {
          LOG(WARNING)
              << "ComponentInstanceProvider already destroyed, skipping preallocation for tag: "
              << request.tag;
          return;
        }
        try {
          delegate->processPreallocationRequest(request);
        } catch (const std::exception& e) {
          LOG(ERROR) << "processPreallocationRequest error for tag: "
                     << request.tag << " component: " << request.componentName
                     << " error: " << e.what();
        }
      });
    } else {
      // Parallel mode but the component is NOT a parallelizable component
      // (e.g. a third-party TurboModule component). The request still needs to
      // be preallocated on the main thread via the UITicker drain path, so it
      // MUST be enqueued here — otherwise onPushPreallocationRequest() would
      // trigger onUITick() -> pop() on an empty queue and silently drop the
      // request, leaving these components without preallocation forever.
      {
        auto lock = std::lock_guard(m_mtx);
        m_queue.push(request);
      }
      try {
        delegate->onPushPreallocationRequest();
      } catch (const std::exception& e) {
        LOG(ERROR) << "onPushPreallocationRequest error: " << e.what();
      }
    }
  }
}

bool ComponentInstancePreallocationRequestQueue::isEmpty() {
  auto lock = std::lock_guard(m_mtx);
  return m_queue.empty();
}

std::optional<ComponentInstancePreallocationRequestQueue::Request>
ComponentInstancePreallocationRequestQueue::pop() {
  auto lock = std::lock_guard(m_mtx);
  if (!m_queue.empty()) {
    auto request = m_queue.front();
    m_queue.pop();
    return request;
  }
  return std::nullopt;
}

void ComponentInstancePreallocationRequestQueue::setDelegate(
    Delegate::Weak weakDelegate) {
  m_weakDelegate = weakDelegate;
}

void ComponentInstancePreallocationRequestQueue::clear() {
  auto lock = std::lock_guard(m_mtx);
  m_queue = {};
}

} // namespace rnoh
