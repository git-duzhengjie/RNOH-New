/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "UpdatePropsManager.h"
#include <glog/logging.h>

namespace rnoh {

// 静态成员初始化
rnoh::MountingManager* UpdatePropsManager::s_mountingManager = nullptr;

void UpdatePropsManager::update(
    facebook::jsi::Runtime& rt,
    facebook::jsi::Value const& viewDescriptors,
    facebook::jsi::Value const& updates) {
  if (!s_mountingManager) {
    LOG(ERROR) << "UpdatePropsManager::update - s_mountingManager is null";
    return;
  }

  try {
    // TODO: 从 viewDescriptors 提取 Tag 和 ComponentDescriptor
    // reanimated 的 viewDescriptors 是 ViewDescriptor 对象数组，
    // 包含 tag、shadowNodeWrapper 等信息。
    
    // TODO: 从 updates 提取样式属性（folly::dynamic）
    // reanimated 的 updates 是样式属性对象（{ opacity: 1, transform: [...] }）。
    
    // TODO: 调用 MountingManager::updateView(tag, props, componentDescriptor)
    // 但需要 ComponentDescriptor，当前无法直接获取。
    
    LOG(INFO) << "UpdatePropsManager::update - called with "
              << "viewDescriptors=" << (viewDescriptors.isObject() ? "object" : "other")
              << ", updates=" << (updates.isObject() ? "object" : "other");
    
    // 临时实现：仅记录日志，不实际更新（P0 先行）
    // 完整实现需要：
    // 1. 解析 viewDescriptors（获取 tag + componentDescriptor）
    // 2. 解析 updates（转换为 folly::dynamic）
    // 3. 调用 MountingManager::updateView
    
  } catch (const std::exception& e) {
    LOG(ERROR) << "UpdatePropsManager::update - exception: " << e.what();
  }
}

void UpdatePropsManager::install(
    facebook::jsi::Runtime& rt,
    rnoh::MountingManager* mountingManager) {
  s_mountingManager = mountingManager;
  
  // 创建 global.UpdatePropsManager 对象
  auto updatePropsManager = facebook::jsi::Object(rt);
  
  // 定义 update 方法
  auto updateFunc = facebook::jsi::Function::createFromHostFunction(
      rt,
      facebook::jsi::PropNameID::forAscii(rt, "update"),
      2, // 参数个数：viewDescriptors, updates
      [](facebook::jsi::Runtime& rt,
         facebook::jsi::Value const& thisVal,
         facebook::jsi::Value const* args,
         size_t count) -> facebook::jsi::Value {
        if (count < 2) {
          return facebook::jsi::Value::undefined();
        }
        UpdatePropsManager::update(rt, args[0], args[1]);
        return facebook::jsi::Value::undefined();
      });
  
  updatePropsManager.setProperty(rt, "update", updateFunc);
  
  // 注册到 global
  rt.global().setProperty(rt, "UpdatePropsManager", updatePropsManager);
  
  LOG(INFO) << "UpdatePropsManager::install - installed to global.UpdatePropsManager";
}

} // namespace rnoh
