/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <arkui/native_node.h>
#include <unordered_map>
#include "RNOH/ArkTSBridge.h"

namespace rnoh {

class ArkUINode;

using EventArgs = ArkUI_NumberValue[MAX_COMPONENT_EVENT_ARG_NUM];

/**
 * @deprecated Do not use it.
 * Use UIInputEventHandler instead.
 */
class TouchEventHandler {
 public:
  virtual void onTouchEvent(ArkUI_UIInputEvent* event) = 0;
  virtual ~TouchEventHandler() = default;
};

/**
 * @deprecated Do not use it.
 * For NODE_TOUCH_EVENT, please use UIInputEventHandler.
 * For other, implement the corresponding logic in ArkUINode::onNodeEvent.
 */
class ArkUINodeRegistry {
  static std::unique_ptr<ArkUINodeRegistry> instance;

 public:
  static void initialize(ArkTSBridge::Shared arkTSBridge);
  static ArkUINodeRegistry& getInstance();
  static bool isInitialized();

  void registerNode(ArkUINode* node){};
  void unregisterNode(ArkUINode* node){};

  void registerTouchHandler(
      ArkUINode* node,
      TouchEventHandler* touchEventHandler);
  void unregisterTouchHandler(ArkUINode* node);

  void reinitializeNodeEventReceiver();

 private:
  ArkUINodeRegistry(ArkTSBridge::Shared arkTSBridge);

  void receiveEvent(ArkUI_NodeEvent* event);

  std::unordered_map<ArkUI_NodeHandle, TouchEventHandler*>
      m_touchHandlerByNodeHandle;
};

} // namespace rnoh
