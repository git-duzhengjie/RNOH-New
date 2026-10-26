/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <folly/dynamic.h>
#include <glog/logging.h>
#include <string>
#include "ArkJS.h"
#include "TaskExecutor/NapiTaskRunner.h"
#include "TaskExecutor/TaskExecutor.h"

namespace rnoh {
class ArkTSChannel {
  ArkJS m_arkJS;
  NapiRef m_napi_event_dispatcher_ref;
  std::shared_ptr<NapiTaskRunner> m_taskRunner;

 public:
  using Shared = std::shared_ptr<ArkTSChannel>;
  using Weak = std::weak_ptr<ArkTSChannel>;

  ArkTSChannel(
      std::shared_ptr<NapiTaskRunner> taskRunner,
      ArkJS arkJS,
      NapiRef napiEventDispatcherRef)
      : m_arkJS(arkJS),
        m_napi_event_dispatcher_ref(std::move(napiEventDispatcherRef)),
        m_taskRunner(std::move(taskRunner)) {}

  ~ArkTSChannel() {
    if (m_taskRunner) {
      m_taskRunner->runAsyncTask(
          [ref = std::move(m_napi_event_dispatcher_ref)] {});
    } else {
      // Since m_taskRunner is not valid reference, we can't schedule cleanup of
      // the event dispatcher ref, it's safest to just leak the reference
      new NapiRef(std::move(m_napi_event_dispatcher_ref));
    }
  }

  void postMessage(std::string type, folly::dynamic payload) {
    if (m_taskRunner) {
      m_taskRunner->runAsyncTask([env = m_arkJS.getEnv(),
                                  eventDispatcherRef =
                                      m_napi_event_dispatcher_ref,
                                  type = std::move(type),
                                  payload = std::move(payload)]() {
        ArkJS arkJS(env);
        auto napi_event_handler = arkJS.getReferenceValue(eventDispatcherRef);
        arkJS.call<2>(
            napi_event_handler,
            {arkJS.createString(type), arkJS.createFromDynamic(payload)});
      });
    }
  }
};
} // namespace rnoh
