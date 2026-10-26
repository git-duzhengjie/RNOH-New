/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "CustomNode.h"

#include <glog/logging.h>
#include <chrono>
#include <memory>
#include "ArkUINodeRegistry.h"
#include "NativeNodeApi.h"
#include "RNOH/ApiVersionCheck.h"
#include "RNOH/ComponentInstance.h"
#include "RNOH/CppComponentInstance.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "TouchEventDispatcher.h"

namespace rnoh {
static const int32_t ON_MEASURE_TARGET_ID = 89;
static const int32_t ON_LAYOUT_TARGET_ID = 90;

void CustomNode::receiveCustomEvent(ArkUI_NodeCustomEvent* event) {
  auto* node = reinterpret_cast<CustomNode*>(
      OH_ArkUI_NodeCustomEvent_GetUserData(event));

  auto type = OH_ArkUI_NodeCustomEvent_GetEventType(event);
  switch (type) {
    case ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE:
      node->onMeasure();
      node->onMeasure(type);
      break;
    case ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT:
      node->onLayout();
    default:
      break;
  }
}

CustomNode::CustomNode(const ArkUINode::Context::Shared& context)
    : ArkUINode(context, ArkUI_NodeType::ARKUI_NODE_CUSTOM),
      m_customNodeDelegate(nullptr) {
  // Store TaskExecutor from context if available
  if (context && context->taskExecutor) {
    m_taskExecutor = context->taskExecutor;
  } else {
    LOG(WARNING)
        << "CustomNode: TaskExecutor is not set in context - markDirty may crash on background thread";
  }

#ifdef ALL_CONTAINERS_CLICKABLE
  registerNodeEvent(NODE_ON_CLICK);
#endif
  registerNodeEvent(NODE_ON_HOVER);
  maybeThrow(NativeNodeApi::getInstance()->addNodeCustomEventReceiver(
      m_nodeHandle, receiveCustomEvent));
  maybeThrow(NativeNodeApi::getInstance()->registerNodeCustomEvent(
      m_nodeHandle,
      ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE,
      ON_MEASURE_TARGET_ID,
      this));
  maybeThrow(NativeNodeApi::getInstance()->registerNodeCustomEvent(
      m_nodeHandle,
      ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT,
      ON_LAYOUT_TARGET_ID,
      this));
  if (IsAtLeastApi21()) {
    maybeThrow(NativeNodeApi::getInstance()->registerNodeEvent(
        m_nodeHandle, NODE_ON_SIZE_CHANGE, 0, this));
  }
  /**
   * This is for 2in1 CustomNode focusing problem, focusing would
   * raise the component and setting ZIndex as 2^31-1, which would
   * setting it at the top to display.
   * Setting z-index ahead would prevent focusing drawing to
   * set the z-index again.
   */
  ArkUI_NumberValue zIndexValue[] = {{.i32 = 0}};
  ArkUI_AttributeItem zIndexItem = {.value = zIndexValue, .size = 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_Z_INDEX, &zIndexItem);
}

void CustomNode::insertChild(ArkUINode& child, std::size_t index) {
  m_nodeApi->insertChildAt(
      m_nodeHandle, child.getArkUINodeHandle(), static_cast<int32_t>(index));
  delayMeasure();
}

void CustomNode::addChild(ArkUINode& child) {
  m_nodeApi->addChild(m_nodeHandle, child.getArkUINodeHandle());
}

void CustomNode::removeChild(ArkUINode& child) {
  if (child.isFocused()) {
    child.setFocusStatus(0);
  }
  maybeThrow(NativeNodeApi::getInstance()->removeChild(
      m_nodeHandle, child.getArkUINodeHandle()));
}

void CustomNode::setCustomNodeDelegate(CustomNodeDelegate* customNodeDelegate) {
  m_customNodeDelegate = customNodeDelegate;
}

void CustomNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  ArkUINode::onNodeEvent(eventType, eventArgs);
  if (IsAtLeastApi24()) {
    if (eventType == ArkUI_NodeEventType::NODE_ON_SIZE_CHANGE) {
      float oldW = eventArgs[0].f32;
      float oldH = eventArgs[1].f32;
      float newW = eventArgs[2].f32;
      float newH = eventArgs[3].f32;
      // skip the initial render
      if (oldW > 0 || oldH > 0) {
        if (std::abs(newW - oldW) > 0.01f || std::abs(newH - oldH) > 0.01f) {
          delayMeasure();
        }
      }
    }
  }
  if (eventType == ArkUI_NodeEventType::NODE_ON_CLICK &&
      eventArgs[3].i32 != UI_INPUT_EVENT_SOURCE_TYPE_TOUCH_SCREEN &&
      eventArgs[3].i32 != UI_INPUT_EVENT_SOURCE_TYPE_MOUSE) {
    onClick();
  }
  if (eventType == ArkUI_NodeEventType::NODE_ON_HOVER) {
    if (m_customNodeDelegate != nullptr) {
      if (eventArgs[0].i32 != 0) {
        m_customNodeDelegate->onHoverIn();
      } else {
        m_customNodeDelegate->onHoverOut();
      }
    }
  }

  // Handle custom overflow scroll event
  if (eventType ==
      static_cast<ArkUI_NodeEventType>(CUSTOM_OVERFLOW_SCROLL_EVENT)) {
    int32_t currentSourceId = eventArgs[0].i32;
    float offset = eventArgs[1].f32;

    if (m_activeScrollChildId == -1) {
      m_activeScrollChildId = currentSourceId;
    }

    if (currentSourceId != m_activeScrollChildId) {
      return;
    }

    auto* delegate = this->getArkUINodeDelegate();
    if (delegate != nullptr) {
      auto* componentInstance =
          dynamic_cast<rnoh::ComponentInstance*>(delegate);
      if (componentInstance != nullptr) {
        facebook::react::Tag rnTag = componentInstance->getTag();
        LOG(ERROR) << "CustomNode Scroll Tag: " << rnTag
                   << " SourceId: " << currentSourceId << " Offset: " << offset;
        m_customNodeDelegate->onScroll(rnTag, offset);
      }
    }
  }
}

void CustomNode::onClick() {
  if (m_customNodeDelegate != nullptr) {
    m_customNodeDelegate->onClick();
  }
}

void CustomNode::onMeasure() {
  int32_t width = getMeasuredWidth();
  int32_t height = getMeasuredHeight();
  m_nodeApi->setMeasuredSize(m_nodeHandle, width, height);
}
void CustomNode::onMeasure(ArkUI_NodeCustomEventType eventType) {}

void CustomNode::onLayout() {}

CustomNode::~CustomNode() {
  // Cancel delayed measure task before destruction
  if (m_delayedMeasureTask.has_value()) {
    if (auto taskExecutor = m_taskExecutor.lock()) {
      taskExecutor->cancelDelayedTask(m_delayedMeasureTask.value());
    }
  }
  unregisterNodeEvent(NODE_ON_CLICK);
  unregisterNodeEvent(NODE_ON_HOVER);
  NativeNodeApi::getInstance()->unregisterNodeCustomEvent(
      m_nodeHandle, ARKUI_NODE_CUSTOM_EVENT_ON_MEASURE);
  NativeNodeApi::getInstance()->unregisterNodeCustomEvent(
      m_nodeHandle, ARKUI_NODE_CUSTOM_EVENT_ON_LAYOUT);
  if (IsAtLeastApi21()) {
    NativeNodeApi::getInstance()->unregisterNodeEvent(
        m_nodeHandle, NODE_ON_SIZE_CHANGE);
  }
  NativeNodeApi::getInstance()->removeNodeCustomEventReceiver(
      m_nodeHandle, receiveCustomEvent);
  NativeNodeApi::getInstance()->removeAllChildren(
      m_nodeHandle); // memory leaks if not called, can't be called in ArkUINode
                     // because ArkUI crashes
}

CustomNode& CustomNode::setAlign(int32_t align) {
  ArkUI_NumberValue value[] = {{.i32 = align}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_STACK_ALIGN_CONTENT, &item);
  return *this;
}

CustomNode& CustomNode::setLayoutRect(
    facebook::react::Point const& position,
    facebook::react::Size const& size,
    facebook::react::Float pointScaleFactor) {
  auto width = static_cast<int32_t>(size.width * pointScaleFactor + 0.5);
  auto height = static_cast<int32_t>(size.height * pointScaleFactor + 0.5);
  updateMeasuredSize(width, height);
  ArkUINode::setLayoutRect(position, size, pointScaleFactor);
  return *this;
}

CustomNode& CustomNode::setFocusable(bool focusable) {
#ifndef ALL_CONTAINERS_CLICKABLE
  if (focusable) {
    registerNodeEvent(NODE_ON_CLICK);
  } else {
    unregisterNodeEvent(NODE_ON_CLICK);
  }
#endif
  int32_t focusableValue = focusable;
  ArkUI_NumberValue preparedFocusable[] = {{.i32 = focusableValue}};
  ArkUI_AttributeItem focusItem = {
      preparedFocusable, sizeof(preparedFocusable) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_FOCUSABLE, &focusItem);
  return *this;
}

void CustomNode::updateMeasuredSize(int32_t width, int32_t height) {
  m_measuredWidth = width;
  m_measuredHeight = height;
}

int32_t CustomNode::getMeasuredWidth() {
  return m_measuredWidth;
}

int32_t CustomNode::getMeasuredHeight() {
  return m_measuredHeight;
}

void CustomNode::delayMeasure() {
  if (!IsAtLeastApi24()) {
    return;
  }

  auto taskExecutor = m_taskExecutor.lock();
  if (!taskExecutor) {
    return;
  }

  // Cancel any existing delayed task
  if (m_delayedMeasureTask.has_value()) {
    taskExecutor->cancelDelayedTask(m_delayedMeasureTask.value());
  }

  // Schedule new delayed task
  auto nodeHandle = getArkUINodeHandle();
  m_delayedMeasureTask = taskExecutor->runDelayedTask(
      TaskThread::MAIN,
      [nodeHandle] {
        NativeNodeApi::getInstance()->markDirty(
            nodeHandle, ArkUI_NodeDirtyFlag::NODE_NEED_MEASURE);
        LOG(INFO) << "CustomNode: markDirty executed on main thread";
      },
      50);
}
} // namespace rnoh
