/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
// Stub implementation for UIInputEventHandler 鈥?provides symbols without the
// OH-specific node event API calls. The real .cpp calls OH_ArkUI_NodeEvent_*
// functions that only exist on device.
#include "RNOH/arkui/UIInputEventHandler.h"
namespace rnoh {
UIInputEventHandler::UIInputEventHandler(ArkUINode& node) : m_node(node) {}
UIInputEventHandler::~UIInputEventHandler() noexcept = default;
}
