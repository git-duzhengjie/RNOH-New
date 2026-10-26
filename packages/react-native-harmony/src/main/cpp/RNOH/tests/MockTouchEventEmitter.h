/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include "react/renderer/components/view/TouchEventEmitter.h"

namespace rnoh {

/**
 * @brief A TouchEventEmitter whose EventDispatcher is an empty weak_ptr.
 *
 * onTouch* -> EventEmitter::dispatchEvent -> eventDispatcher_.lock() returns
 * empty -> early-returns (EventEmitter.cpp), so calls become no-ops without
 * crashing. This lets TouchEventDispatcher be exercised end-to-end without
 * the full RN event-delivery pipeline (EventQueue/EventBeat/jsi).
 *
 * NOTE: Because onTouch* are non-virtual, the actual emitter invocations
 * cannot be spied on; tests can only assert routing/state, not emitter calls.
 */
class MockTouchEventEmitter : public facebook::react::TouchEventEmitter {
 public:
  MockTouchEventEmitter()
      : facebook::react::TouchEventEmitter(
            nullptr,
            facebook::react::EventDispatcher::Weak{}) {}
};

} // namespace rnoh
