/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOH/arkui/NativeNodeApi.h"
#include "RNOH/arkui/NodeApi.h"
#include "deviceinfo.h"

namespace rnoh {

NodeApi::NodeApi(ArkUI_ContextHandle uiContext) : m_uiContext(uiContext) {}

ArkUI_NodeHandle NodeApi::createNode(ArkUI_NodeType nodeType) {
  return NativeNodeApi::getInstance()->createNode(nodeType);
}

void NodeApi::setAttribute(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute,
    const ArkUI_AttributeItem* item) {
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(node, attribute, item));
}

void NodeApi::resetAttribute(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute) {
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(node, attribute));
}

const ArkUI_AttributeItem* NodeApi::getAttribute(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute) {
  return NativeNodeApi::getInstance()->getAttribute(node, attribute);
}

void NodeApi::setMeasuredSize(
    ArkUI_NodeHandle node,
    int32_t width,
    int32_t height) {
  maybeThrow(
      NativeNodeApi::getInstance()->setMeasuredSize(node, width, height));
}

void NodeApi::insertChildAt(
    ArkUI_NodeHandle parent,
    ArkUI_NodeHandle child,
    int32_t position) {
  maybeThrow(
      NativeNodeApi::getInstance()->insertChildAt(parent, child, position));
}

void NodeApi::addChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child) {
  maybeThrow(NativeNodeApi::getInstance()->addChild(parent, child));
}

} // namespace rnoh

extern "C" int OH_GetSdkApiVersion() {
#if defined(RNOH_TEST_SDK_API_VERSION)
  return RNOH_TEST_SDK_API_VERSION;
#else
  return 24;
#endif
}
