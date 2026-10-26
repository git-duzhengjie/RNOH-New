/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/FBReactNativeSpec/EventEmitters.h>
#include "RNOH/ComponentInstance.h"
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/ToggleNode.h"
#include "react/renderer/components/FBReactNativeSpec/ShadowNodes.h"

namespace rnoh {
class SwitchComponentInstance
    : public CppComponentInstance<facebook::react::SwitchShadowNode>,
      public ToggleNodeDelegate {
 private:
  ToggleNode m_toggleNode;

 public:
  SwitchComponentInstance(Context context);

  void onPropsChanged(SharedConcreteProps const& props) override;

  void onValueChange(int32_t& value) override;

  ToggleNode& getLocalRootArkUINode() override;
};
} // namespace rnoh