/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ActivityIndicatorComponentInstance.h"

#include <glog/logging.h>
#include <react/renderer/components/FBReactNativeSpec/Props.h>
#include <react/renderer/core/ConcreteState.h>
#include <sstream>

namespace rnoh {

ActivityIndicatorComponentInstance::ActivityIndicatorComponentInstance(
    Context context)
    : CppComponentInstance(std::move(context)),
      m_loadingProgressNode(m_arkUINodeCtx) {}

void ActivityIndicatorComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  auto old = m_props;
  CppComponentInstance::onPropsChanged(props);
  if (props->animating != old->animating) {
    getLocalRootArkUINode().setAnimating(props->animating);
  }
  if (props->color && (*(props->color) != *(old->color))) {
    getLocalRootArkUINode().setColor(props->color);
  }
}

LoadingProgressNode&
ActivityIndicatorComponentInstance::getLocalRootArkUINode() {
  return m_loadingProgressNode;
}

} // namespace rnoh
