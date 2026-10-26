/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/core/EventBeat.h>
#include "RNOH/UITicker.h"

namespace rnoh {

class EventBeat final : public facebook::react::EventBeat {
 public:
  EventBeat(
      std::shared_ptr<facebook::react::EventBeat::OwnerBox> ownerBox,
      std::shared_ptr<facebook::react::RuntimeScheduler> runtimeScheduler,
      UITicker::Shared uiTicker);

  ~EventBeat();

  void request() const override;

 private:
  void induce() const;
  mutable std::function<void()> m_unsubscribeUITickerListener = nullptr;
  mutable std::mutex m_unsubscribeUITickerListenerMtx;
  UITicker::Shared m_uiTicker;
  std::weak_ptr<facebook::react::RuntimeScheduler> m_weakRuntimeScheduler;
};

} // namespace rnoh