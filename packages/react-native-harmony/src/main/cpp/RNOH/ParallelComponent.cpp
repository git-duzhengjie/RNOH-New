/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#include "ParallelComponent.h"

namespace rnoh {

ComponentNameManager& ComponentNameManager::getInstance() {
  static ComponentNameManager instance;
  return instance;
}

void ComponentNameManager::addComponentName(const std::string& componentName) {
  if (componentName.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(m_mutex);
  parallelComponentNames.insert(componentName);
}

bool ComponentNameManager::hasComponentName(
    const std::string& componentName) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return parallelComponentNames.find(componentName) !=
      parallelComponentNames.end();
}

} // namespace rnoh
