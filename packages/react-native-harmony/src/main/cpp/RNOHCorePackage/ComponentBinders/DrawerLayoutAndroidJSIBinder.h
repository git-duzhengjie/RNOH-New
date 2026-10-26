/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include "RNOHCorePackage/ComponentBinders/ViewComponentJSIBinder.h"

namespace rnoh {
class DrawerLayoutAndroidJSIBinder : public ViewComponentJSIBinder {
  facebook::jsi::Object createNativeProps(facebook::jsi::Runtime& rt) override {
    auto nativeProps = ViewComponentJSIBinder::createNativeProps(rt);
    nativeProps.setProperty(rt, "drawerBackgroundColor", "Color");
    nativeProps.setProperty(rt, "drawerWidth", "number");
    nativeProps.setProperty(rt, "drawerPosition", "string");
    return nativeProps;
  }
  facebook::jsi::Object createCommands(facebook::jsi::Runtime& rt) override {
    auto commands = ViewComponentJSIBinder::createCommands(rt);
    commands.setProperty(rt, "openDrawer", "openDrawer");
    commands.setProperty(rt, "closeDrawer", "closeDrawer");
    return commands;
  }
  facebook::jsi::Object createDirectEventTypes(
      facebook::jsi::Runtime& rt) override {
    facebook::jsi::Object events =
        ViewComponentJSIBinder::createDirectEventTypes(rt);
    events.setProperty(
        rt, "topDrawerClose", createDirectEvent(rt, "onDrawerClose"));
    events.setProperty(
        rt, "topDrawerOpen", createDirectEvent(rt, "onDrawerOpen"));
    return events;
  }
};
} // namespace rnoh