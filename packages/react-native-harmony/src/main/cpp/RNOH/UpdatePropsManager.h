/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jsi/jsi.h>
#include <folly/dynamic.h>
#include "RNOH/MountingManager.h"

namespace rnoh {

/**
 * UpdatePropsManager
 *
 * RNOH 0.86 无 reanimated 原生模块（UpdatePropsManager 缺失），
 * 本类提供 reanimated 的 UpdatePropsManager 的 C++ 实现。
 *
 * 功能：
 * 1. 接收 JS 调用：global.UpdatePropsManager.update(viewDescriptors, updates)
 * 2. 更新 View 样式：根据 updates 更新 viewDescriptors 指定的 View 的样式
 * 3. 支持 worklet：在 UI 线程执行（'worklet' 标记）
 */
class UpdatePropsManager {
 public:
  /**
   * JS 调用入口：global.UpdatePropsManager.update(viewDescriptors, updates)
   *
   * @param rt JSI Runtime
   * @param viewDescriptors View 描述符数组（包含 tag + componentHandle）
   * @param updates 样式属性更新（jsi::Object）
   */
  static void update(
      facebook::jsi::Runtime& rt,
      facebook::jsi::Value const& viewDescriptors,
      facebook::jsi::Value const& updates);

  /**
   * 注册到 global（JS 端：global.UpdatePropsManager = { update: ... }）
   *
   * @param rt JSI Runtime
   * @param mountingManager RNOH MountingManager 实例
   */
  static void install(
      facebook::jsi::Runtime& rt,
      rnoh::MountingManager* mountingManager);

 private:
  static rnoh::MountingManager* s_mountingManager;
};

} // namespace rnoh
