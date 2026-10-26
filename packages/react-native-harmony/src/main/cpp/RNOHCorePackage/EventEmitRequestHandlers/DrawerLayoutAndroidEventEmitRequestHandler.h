/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#pragma once
#include <react/renderer/components/FBReactNativeSpec/EventEmitters.h>

#include "RNOH/EventEmitRequestHandler.h"
namespace rnoh {
class DrawerLayoutAndroidEventEmitRequestHandler
    : public EventEmitRequestHandler {
 public:
  void handleEvent(EventEmitRequestHandler::Context const& ctx) override {
    auto eventName = ctx.eventName;
    auto eventEmitter =
        ctx.shadowViewRegistry
            ->getEventEmitter<facebook::react::AndroidDrawerLayoutEventEmitter>(
                ctx.tag);
    if (eventEmitter == nullptr) {
      return;
    }

    if (eventName == "drawerOpen") {
      eventEmitter->onDrawerOpen({});
    } else if (eventName == "drawerClose") {
      eventEmitter->onDrawerClose({});
    } else {
      LOG(ERROR) << "Unsupported drawer layout event name: " << eventName;
    }
  }
};

} // namespace rnoh