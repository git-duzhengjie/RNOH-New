/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ArkUINode.h"
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <algorithm>
#include <cmath>
#include "NativeNodeApi.h"
#include "RNOH/Assert.h"
#include "conversions.h"
#include "react/renderer/graphics/Transform.h"

namespace rnoh {

const std::unordered_map<std::string, ArkUI_NodeType> NODE_TYPE_BY_ROLE_NAME = {
    {"button", ARKUI_NODE_BUTTON},
    {"togglebutton", ARKUI_NODE_TOGGLE},
    {"search", ARKUI_NODE_TEXT_INPUT},
    {"image", ARKUI_NODE_IMAGE},
    {"text", ARKUI_NODE_TEXT},
    {"adjustable", ARKUI_NODE_SLIDER},
    {"imagebutton", ARKUI_NODE_BUTTON},
    {"checkbox", ARKUI_NODE_CHECKBOX},
    {"menuitem", ARKUI_NODE_LIST_ITEM},
    {"progressbar", ARKUI_NODE_PROGRESS},
    {"radio", ARKUI_NODE_RADIO},
    {"scrollbar", ARKUI_NODE_SCROLL},
    {"switch", ARKUI_NODE_TOGGLE},
    {"list", ARKUI_NODE_LIST},
    {"cell", ARKUI_NODE_GRID_ITEM},
    {"grid", ARKUI_NODE_GRID},
    {"img", ARKUI_NODE_IMAGE},
    {"listitem", ARKUI_NODE_LIST_ITEM},
    {"marquee", ARKUI_NODE_IMAGE_ANIMATOR},
    {"meter", ARKUI_NODE_PROGRESS},
    {"option", ARKUI_NODE_LIST_ITEM},
    {"row", ARKUI_NODE_ROW},
    {"searchbox", ARKUI_NODE_TEXT_INPUT},
    {"slider", ARKUI_NODE_SLIDER},
    {"table", ARKUI_NODE_GRID},
};

const std::unordered_map<facebook::react::Role, ArkUI_NodeType>
    ROLE_BY_ROLE_TYPE = {
        {facebook::react::Role::Button, ARKUI_NODE_BUTTON},
        {facebook::react::Role::Cell, ARKUI_NODE_GRID_ITEM},
        {facebook::react::Role::Checkbox, ARKUI_NODE_CHECKBOX},
        {facebook::react::Role::Grid, ARKUI_NODE_GRID},
        {facebook::react::Role::Img, ARKUI_NODE_IMAGE},
        {facebook::react::Role::List, ARKUI_NODE_LIST},
        {facebook::react::Role::Listitem, ARKUI_NODE_LIST_ITEM},
        {facebook::react::Role::Marquee, ARKUI_NODE_IMAGE_ANIMATOR},
        {facebook::react::Role::Meter, ARKUI_NODE_PROGRESS},
        {facebook::react::Role::Option, ARKUI_NODE_LIST_ITEM},
        {facebook::react::Role::Progressbar, ARKUI_NODE_PROGRESS},
        {facebook::react::Role::Radio, ARKUI_NODE_RADIO},
        {facebook::react::Role::Row, ARKUI_NODE_ROW},
        {facebook::react::Role::Scrollbar, ARKUI_NODE_SCROLL},
        {facebook::react::Role::Searchbox, ARKUI_NODE_TEXT_INPUT},
        {facebook::react::Role::Slider, ARKUI_NODE_SLIDER},
        {facebook::react::Role::Switch, ARKUI_NODE_TOGGLE},
        {facebook::react::Role::Table, ARKUI_NODE_GRID},
};

std::optional<ArkUI_NodeType> roleNameToNodeType(const std::string& roleName) {
  auto it = NODE_TYPE_BY_ROLE_NAME.find(roleName);
  if (it != NODE_TYPE_BY_ROLE_NAME.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::optional<ArkUI_NodeType> roleNameToNodeType(facebook::react::Role role) {
  auto it = ROLE_BY_ROLE_TYPE.find(role);
  if (it != ROLE_BY_ROLE_TYPE.end()) {
    return it->second;
  }
  return std::nullopt;
}

const std::unordered_map<std::string, ArkUI_AccessibilityActionType>
    ACTION_TYPE_BY_NAME = {
        {"activate", ARKUI_ACCESSIBILITY_ACTION_CLICK},
        {"longpress", ARKUI_ACCESSIBILITY_ACTION_LONG_CLICK},
        {"cut", ARKUI_ACCESSIBILITY_ACTION_CUT},
        {"copy", ARKUI_ACCESSIBILITY_ACTION_COPY},
        {"paste", ARKUI_ACCESSIBILITY_ACTION_PASTE},
};

std::optional<ArkUI_AccessibilityActionType> actionNameToType(
    const std::string& name) {
  auto it = ACTION_TYPE_BY_NAME.find(name);
  if (it != ACTION_TYPE_BY_NAME.end()) {
    return it->second;
  } else {
    return std::nullopt;
  }
}

std::optional<std::string> actionTypeToName(
    ArkUI_AccessibilityActionType type) {
  for (const auto& pair : ACTION_TYPE_BY_NAME) {
    if (pair.second == type) {
      return pair.first;
    }
  }
  return std::nullopt;
}

static constexpr std::array NODE_EVENT_TYPES{
    NODE_ON_ACCESSIBILITY_ACTIONS,
    NODE_ON_TOUCH_INTERCEPT};

static std::unordered_map<ArkUI_NodeHandle, ArkUINode*> NODE_BY_HANDLE;
static std::mutex NODE_BY_HANDLE_MUTEX;

static void receiveEvent(ArkUI_NodeEvent* event) {
  try {
    auto eventType = OH_ArkUI_NodeEvent_GetEventType(event);
    auto node = OH_ArkUI_NodeEvent_GetNodeHandle(event);
    ArkUINode* target = nullptr;

    {
      std::lock_guard<std::mutex> lock(NODE_BY_HANDLE_MUTEX);
      auto it = NODE_BY_HANDLE.find(node);
      if (it == NODE_BY_HANDLE.end()) {
        DLOG(WARNING) << "Node with handle: " << node << " not found";
        return;
      }
      target = it->second;
    }

    if (eventType == ArkUI_NodeEventType::NODE_TOUCH_EVENT) {
      // Node Touch events are handled in UIInputEventHandler instead
      return;
    }

    if (eventType == ArkUI_NodeEventType::NODE_ON_TOUCH_INTERCEPT) {
      auto inputEvent = OH_ArkUI_NodeEvent_GetInputEvent(event);
      target->onTouchIntercept(inputEvent);
      return;
    }

    auto componentEvent = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
    if (componentEvent != nullptr) {
      target->onNodeEvent(eventType, componentEvent->data);
      return;
    }
    if (eventType ==
            ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CHANGE_WITH_PREVIEW_TEXT ||
        eventType ==
            ArkUI_NodeEventType::NODE_TEXT_AREA_ON_CHANGE_WITH_PREVIEW_TEXT) {
      target->onNodeEvent(eventType, event);
      return;
    }
    auto eventString = OH_ArkUI_NodeEvent_GetStringAsyncEvent(event);
    if (eventString != nullptr) {
      target->onNodeEvent(eventType, std::string_view(eventString->pStr));
      return;
    }
    target->onNodeEvent(eventType, event);
  } catch (std::exception& e) {
    LOG(ERROR) << e.what();
  }
}

ArkUINode::ArkUINode(ArkUI_NodeHandle nodeHandle) : m_nodeHandle(nodeHandle) {
  if (m_nodeHandle == nullptr) {
    throw std::runtime_error("Failed to create ArkUI node: nodeHandle is null");
  }
  DLOG(WARNING) << "ArkUINode constructor with nodeHandle is not recommended."
                << "Use ArkUINode(Context::Shared, ArkUI_NodeType) instead to"
                << "avoid issues with subwindows in custom components.";
  maybeThrow(NativeNodeApi::getInstance()->addNodeEventReceiver(
      m_nodeHandle, receiveEvent));
  {
    std::lock_guard<std::mutex> lock(NODE_BY_HANDLE_MUTEX);
    NODE_BY_HANDLE.emplace(m_nodeHandle, this);
  }
  for (auto eventType : NODE_EVENT_TYPES) {
    this->registerNodeEvent(eventType);
  }
}

ArkUINode::ArkUINode(const Context::Shared context, ArkUI_NodeType nodeType) {
  if (context != nullptr && context->nodeApi != nullptr) {
    m_nodeApi = context->nodeApi;
  }
  m_nodeHandle = m_nodeApi->createNode(nodeType);
  if (m_nodeHandle == nullptr) {
    throw std::runtime_error("Failed to create ArkUI node: nodeHandle is null");
  }
  maybeThrow(NativeNodeApi::getInstance()->addNodeEventReceiver(
      m_nodeHandle, receiveEvent));
  {
    std::lock_guard<std::mutex> lock(NODE_BY_HANDLE_MUTEX);
    NODE_BY_HANDLE.emplace(m_nodeHandle, this);
  }
  for (auto eventType : NODE_EVENT_TYPES) {
    this->registerNodeEvent(eventType);
  }
}

ArkUINode::~ArkUINode() noexcept {
  for (auto eventType : NODE_EVENT_TYPES) {
    this->unregisterNodeEvent(eventType);
  }
  if (m_arkUINodeDelegate != nullptr) {
    m_arkUINodeDelegate->onArkUINodeDestroy(this);
  }
  {
    std::lock_guard<std::mutex> lock(NODE_BY_HANDLE_MUTEX);
    auto it = NODE_BY_HANDLE.find(m_nodeHandle);
    if (it != NODE_BY_HANDLE.end()) {
      NODE_BY_HANDLE.erase(it);
    }
  }
  NativeNodeApi::getInstance()->removeNodeEventReceiver(
      m_nodeHandle, receiveEvent);
  NativeNodeApi::getInstance()->disposeNode(m_nodeHandle);
}

void ArkUINode::setArkUINodeDelegate(ArkUINodeDelegate* delegate) {
  m_arkUINodeDelegate = delegate;
}

ArkUI_NodeHandle ArkUINode::getArkUINodeHandle() {
  return m_nodeHandle;
}

void ArkUINode::markDirty() {
  // TODO: maybe this can be passed as an arg here,
  // and Component Instance can decide which flag to set in each mutation
  NativeNodeApi::getInstance()->markDirty(
      getArkUINodeHandle(), ArkUI_NodeDirtyFlag::NODE_NEED_RENDER);
  NativeNodeApi::getInstance()->markDirty(
      getArkUINodeHandle(), ArkUI_NodeDirtyFlag::NODE_NEED_LAYOUT);
  NativeNodeApi::getInstance()->markDirty(
      getArkUINodeHandle(), ArkUI_NodeDirtyFlag::NODE_NEED_MEASURE);
}

ArkUINode& ArkUINode::setPosition(facebook::react::Point const& position) {
  ArkUI_NumberValue value[] = {
      static_cast<float>(position.x), static_cast<float>(position.y)};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_POSITION, &item);
  return *this;
}

ArkUINode& ArkUINode::setSize(facebook::react::Size const& size) {
  // HACK: ArkUI doesn't handle 0-sized views properly
  ArkUI_NumberValue widthValue[] = {
      static_cast<float>(size.width > 0 ? size.width : 0.01)};
  ArkUI_AttributeItem widthItem = {
      widthValue, sizeof(widthValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_WIDTH, &widthItem);

  // HACK: ArkUI doesn't handle 0-sized views properly
  ArkUI_NumberValue heightValue[] = {
      static_cast<float>(size.height > 0 ? size.height : 0.01)};
  ArkUI_AttributeItem heightItem = {
      heightValue, sizeof(heightValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_HEIGHT, &heightItem);
  return *this;
}

void ArkUINode::saveSize(int32_t width, int32_t height) {
  m_measuredWidth = width;
  m_measuredHeight = height;
}

int32_t ArkUINode::getSavedWidth() {
  return m_measuredWidth;
}

int32_t ArkUINode::getSavedHeight() {
  return m_measuredHeight;
}

ArkUINode& ArkUINode::setHeight(float height) {
  ArkUI_NumberValue heightValue[] = {{.f32 = height}};
  ArkUI_AttributeItem heightItem = {
      heightValue, sizeof(heightValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_HEIGHT, &heightItem);
  return *this;
}

inline int32_t roundToInt(double value) {
  constexpr double HALF = 0.5;
  if (value >= 0) {
    return static_cast<int32_t>(value + HALF);
  } else {
    return -static_cast<int32_t>(-value + HALF);
  }
}

ArkUINode& ArkUINode::setLayoutRect(
    facebook::react::Point const& position,
    facebook::react::Size const& size,
    facebook::react::Float pointScaleFactor) {
  ArkUI_NumberValue value[] = {
      {.i32 = roundToInt(position.x * pointScaleFactor)},
      {.i32 = roundToInt(position.y * pointScaleFactor)},
      {.i32 = roundToInt(size.width * pointScaleFactor)},
      {.i32 = roundToInt(size.height * pointScaleFactor)}};
  this->saveSize(value[2].i32, value[3].i32);
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_LAYOUT_RECT, &item);
  return *this;
}

ArkUINode& ArkUINode::setLayoutRect(
    facebook::react::LayoutMetrics const& layoutMetrics) {
  setLayoutRect(
      layoutMetrics.frame.origin,
      layoutMetrics.frame.size,
      layoutMetrics.pointScaleFactor);
  return *this;
}

ArkUINode& ArkUINode::setWidth(float width) {
  ArkUI_NumberValue widthValue[] = {{.f32 = width}};
  ArkUI_AttributeItem widthItem = {
      widthValue, sizeof(widthValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_WIDTH, &widthItem);
  return *this;
}

ArkUINode& ArkUINode::setBorderWidth(
    facebook::react::BorderWidths const& borderWidths) {
  facebook::react::BorderWidths borderWidth;
  borderWidth.top = std::max(borderWidths.top, 0.0f);
  borderWidth.right = std::max(borderWidths.right, 0.0f);
  borderWidth.bottom = std::max(borderWidths.bottom, 0.0f);
  borderWidth.left = std::max(borderWidths.left, 0.0f);
  ArkUI_NumberValue borderWidthValue[] = {
      {.f32 = static_cast<float>(borderWidth.top)},
      {.f32 = static_cast<float>(borderWidth.right)},
      {.f32 = static_cast<float>(borderWidth.bottom)},
      {.f32 = static_cast<float>(borderWidth.left)}};

  ArkUI_AttributeItem borderWidthItem = {
      borderWidthValue, sizeof(borderWidthValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_BORDER_WIDTH, &borderWidthItem);
  return *this;
}

ArkUINode& ArkUINode::setOutlineWidth(
    facebook::react::BorderWidths const& outlineWidths) {
  facebook::react::BorderWidths outlineWidth;
  outlineWidth.left = std::max(outlineWidths.left, 0.0f);
  outlineWidth.top = std::max(outlineWidths.top, 0.0f);
  outlineWidth.right = std::max(outlineWidths.right, 0.0f);
  outlineWidth.bottom = std::max(outlineWidths.bottom, 0.0f);

  ArkUI_NumberValue outlineWidthValue[] = {
      {.f32 = static_cast<float>(outlineWidth.top)},
      {.f32 = static_cast<float>(outlineWidth.right)},
      {.f32 = static_cast<float>(outlineWidth.bottom)},
      {.f32 = static_cast<float>(outlineWidth.left)}};

  ArkUI_AttributeItem outlineWidthItem = {
      outlineWidthValue, sizeof(outlineWidthValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_OUTLINE_WIDTH, &outlineWidthItem);
  return *this;
}

ArkUINode& ArkUINode::setOutlineColors(
    facebook::react::BorderColors const& outlineColors) {
  uint32_t outlineLeftColor = (uint32_t)*outlineColors.left;
  uint32_t outlineTopColor = (uint32_t)*outlineColors.top;
  uint32_t outlineRightColor = (uint32_t)*outlineColors.right;
  uint32_t outlineBottomColor = (uint32_t)*outlineColors.bottom;

  ArkUI_NumberValue outlineColorValue[] = {
      {.u32 = outlineTopColor},
      {.u32 = outlineRightColor},
      {.u32 = outlineBottomColor},
      {.u32 = outlineLeftColor}};

  ArkUI_AttributeItem outlineColorItem = {
      outlineColorValue, sizeof(outlineColorValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_OUTLINE_COLOR, &outlineColorItem);
  return *this;
}

ArkUINode& ArkUINode::setBorderColor(
    facebook::react::BorderColors const& borderColors) {
  uint32_t borderTopColor = (uint32_t)*borderColors.top;
  uint32_t borderRightColor = (uint32_t)*borderColors.right;
  uint32_t borderBottomColor = (uint32_t)*borderColors.bottom;
  uint32_t borderLeftColor = (uint32_t)*borderColors.left;
  ArkUI_NumberValue borderColorValue[] = {
      {.u32 = borderTopColor},
      {.u32 = borderRightColor},
      {.u32 = borderBottomColor},
      {.u32 = borderLeftColor}};

  ArkUI_AttributeItem borderColorItem = {
      borderColorValue, sizeof(borderColorValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_BORDER_COLOR, &borderColorItem);
  return *this;
}

ArkUINode& ArkUINode::setBorderRadius(
    facebook::react::BorderRadii const& borderRadius) {
  /* React Native introduced partial support for elliptical radii in version
   * 0.76. https://github.com/facebook/react-native/pull/46009.
   *
   * The difference between OH and Android can be observed when a radius
   * uses a "%" unit on a non-square view.*/
  ArkUI_NumberValue borderRadiusValue[] = {
      static_cast<float>(
          (borderRadius.topLeft.horizontal + borderRadius.topLeft.vertical) /
          2),
      static_cast<float>(
          (borderRadius.topRight.horizontal + borderRadius.topRight.vertical) /
          2),
      static_cast<float>(
          (borderRadius.bottomLeft.horizontal +
           borderRadius.bottomLeft.vertical) /
          2),
      static_cast<float>(
          (borderRadius.bottomRight.horizontal +
           borderRadius.bottomRight.vertical) /
          2),
  };

  ArkUI_AttributeItem borderRadiusItem = {
      borderRadiusValue, sizeof(borderRadiusValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_BORDER_RADIUS, &borderRadiusItem);
  return *this;
}

ArkUINode& ArkUINode::setBorderStyle(
    facebook::react::BorderStyles const& borderStyles) {
  ArkUI_NumberValue borderStyleValue[] = {
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.top))},
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.right))},
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.bottom))},
      {.i32 = static_cast<int32_t>(
           rnoh::convertReactBorderStyleToArk(borderStyles.left))}};

  ArkUI_AttributeItem borderStyleItem = {
      borderStyleValue, sizeof(borderStyleValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_BORDER_STYLE, &borderStyleItem);
  return *this;
}

ArkUINode& ArkUINode::setShadow(
    facebook::react::SharedColor const& shadowColor,
    facebook::react::Size const& shadowOffset,
    float const shadowOpacity,
    float const shadowRadius,
    facebook::react::Float pointScaleFactor) {
  if (shadowOpacity <= 0.0) {
    return *this;
  }
  float shadowOpacityValue = shadowOpacity > 1.0 ? 1.0 : shadowOpacity;
  uint32_t shadowColorValue = 0xff000000;
  if (shadowColor) {
    shadowColorValue = *shadowColor;
  }
  auto alpha = static_cast<uint32_t>(
      (float)((shadowColorValue >> 24) & (0xff)) * shadowOpacityValue);
  shadowColorValue = (alpha << 24) + (shadowColorValue & 0xffffff);
  float offsetXInPx = shadowOffset.width * pointScaleFactor;
  float offsetYInPx = shadowOffset.height * pointScaleFactor;
  float shadowRadiusInPx = shadowRadius * pointScaleFactor;
  ArkUI_NumberValue shadowValue[] = {
      {.f32 = shadowRadiusInPx},
      {.i32 = 0},
      {.f32 = offsetXInPx},
      {.f32 = offsetYInPx},
      {.i32 = 0},
      {.u32 = shadowColorValue},
      {.u32 = 0}};
  ArkUI_AttributeItem shadowItem = {
      .value = shadowValue,
      .size = sizeof(shadowValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_CUSTOM_SHADOW, &shadowItem);
  return *this;
}

/**
 * @deprecated This is internal method. Do not use it, it will be removed in
 * the future (latestRNOHVersion: 0.75.2)
 */
ArkUINode& ArkUINode::setHitTestMode(ArkUI_HitTestMode hitTestMode) {
  setAttribute(
      NODE_HIT_TEST_BEHAVIOR, {{.i32 = static_cast<int32_t>(hitTestMode)}});
  return *this;
}

/**
 * @deprecated The matching of pointerEvents and hitTestBehavior has been
 * moved to CppComponentInstance::onArkUINodeTouchIntercept (latestRNOHVersion:
 * 0.75.2)
 */
ArkUINode& ArkUINode::setHitTestMode(
    facebook::react::PointerEventsMode const& pointerEvents) {
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityRole(std::string const& roleName) {
  if (roleName == "none") {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ROLE);
    return *this;
  }
  std::optional<ArkUI_NodeType> maybeNodeType = roleNameToNodeType(roleName);
  if (!maybeNodeType.has_value()) {
    DLOG(WARNING) << "Unsupported accessibility role: " << roleName;
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ROLE);
    return *this;
  }
  auto nodeType = maybeNodeType.value();
  ArkUI_NumberValue value[] = {{.u32 = nodeType}};
  ArkUI_AttributeItem attr = {
      .value = value, .size = sizeof(nodeType) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ROLE, &attr);
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityRole(facebook::react::Role roleName) {
  if (roleName == facebook::react::Role::None) {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ROLE);
    return *this;
  }
  std::optional<ArkUI_NodeType> maybeNodeType = roleNameToNodeType(roleName);
  if (!maybeNodeType.has_value()) {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ROLE);
    return *this;
  }
  auto nodeType = maybeNodeType.value();
  ArkUI_NumberValue value[] = {{.u32 = nodeType}};
  ArkUI_AttributeItem attr = {
      .value = value, .size = sizeof(nodeType) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ROLE, &attr);
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityActions(
    const std::vector<facebook::react::AccessibilityAction>& rnActions) {
  std::vector<ArkUI_NumberValue> actionTypes;
  actionTypes.reserve(rnActions.size());
  for (const auto& rnAction : rnActions) {
    auto actionType = actionNameToType(rnAction.name);
    if (!actionType.has_value()) {
      DLOG(WARNING) << "Unsupported accessibility action: " << rnAction.name;
      continue;
    }
    actionTypes.push_back({.u32 = actionType.value()});
  }
  if (actionTypes.empty()) {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ACTIONS);
  } else {
    ArkUI_AttributeItem attr = {
        .value = actionTypes.data(),
        .size = static_cast<int32_t>(actionTypes.size())};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_ACTIONS, &attr);
  }
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityDescription(
    std::string const& accessibilityDescription) {
  ArkUI_AttributeItem descriptionItem = {
      .string = accessibilityDescription.c_str()};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_ACCESSIBILITY_DESCRIPTION, &descriptionItem);
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityState(
    const facebook::react::AccessibilityState& rnState) {
  std::unique_ptr<
      ArkUI_AccessibilityState,
      decltype(&OH_ArkUI_AccessibilityState_Dispose)>
      state(
          OH_ArkUI_AccessibilityState_Create(),
          OH_ArkUI_AccessibilityState_Dispose);
  OH_ArkUI_AccessibilityState_SetDisabled(state.get(), rnState.disabled);
  OH_ArkUI_AccessibilityState_SetCheckedState(
      state.get(),
      rnState.checked == facebook::react::AccessibilityState::Checked);
  OH_ArkUI_AccessibilityState_SetSelected(state.get(), rnState.selected);
  ArkUI_AttributeItem item = {.object = state.get()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_STATE, &item);
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityState(
    const std::optional<facebook::react::AccessibilityState>& maybeA11yState) {
  if (!maybeA11yState.has_value()) {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_STATE);
    return *this;
  }
  const auto& a11yState = maybeA11yState.value();
  std::unique_ptr<
      ArkUI_AccessibilityState,
      decltype(&OH_ArkUI_AccessibilityState_Dispose)>
      state(
          OH_ArkUI_AccessibilityState_Create(),
          OH_ArkUI_AccessibilityState_Dispose);
  OH_ArkUI_AccessibilityState_SetDisabled(state.get(), a11yState.disabled);
  OH_ArkUI_AccessibilityState_SetCheckedState(
      state.get(),
      a11yState.checked == facebook::react::AccessibilityState::Checked);
  OH_ArkUI_AccessibilityState_SetSelected(state.get(), a11yState.selected);
  ArkUI_AttributeItem item = {.object = state.get()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_STATE, &item);
  return *this;
}

/**
 * @deprecated (latestRNOHVersion: 0.75.1)
 */
ArkUINode& ArkUINode::setAccessibilityLevel(
    facebook::react::ImportantForAccessibility importance) {
  ArkUI_NumberValue levelValue[] = {{.i32 = static_cast<int32_t>(importance)}};
  ArkUI_AttributeItem levelItem = {
      .value = levelValue,
      .size = sizeof(levelValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_MODE, &levelItem);
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityMode(
    facebook::react::ImportantForAccessibility importance) {
  switch (importance) {
    case facebook::react::ImportantForAccessibility::Auto:
      this->setAccessibilityMode(
          ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_AUTO);
      break;
    case facebook::react::ImportantForAccessibility::Yes:
      this->setAccessibilityMode(
          ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_ENABLED);
      break;
    case facebook::react::ImportantForAccessibility::No:
      this->setAccessibilityMode(
          ArkUI_AccessibilityMode::ARKUI_ACCESSIBILITY_MODE_DISABLED);
      break;
    case facebook::react::ImportantForAccessibility::NoHideDescendants:
      this->setAccessibilityMode(
          ArkUI_AccessibilityMode::
              ARKUI_ACCESSIBILITY_MODE_DISABLED_FOR_DESCENDANTS);
      break;
    default:
      DLOG(WARNING) << "Unsupported importantForAccessibility value";
  }
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityMode(ArkUI_AccessibilityMode mode) {
  ArkUI_NumberValue value = {.i32 = mode};
  setAttribute(NODE_ACCESSIBILITY_MODE, {value});
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityText(
    std::string const& accessibilityLabel) {
  ArkUI_AttributeItem textItem = {.string = accessibilityLabel.c_str()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_TEXT, &textItem);
  return *this;
}

ArkUINode& ArkUINode::setAccessibilityGroup(bool enableGroup) {
  ArkUI_NumberValue groupValue[] = {{.i32 = static_cast<int32_t>(enableGroup)}};
  ArkUI_AttributeItem groupItem = {
      .value = groupValue,
      .size = sizeof(groupValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ACCESSIBILITY_GROUP, &groupItem);
  return *this;
}

ArkUINode& ArkUINode::setId(std::string const& id) {
  ArkUI_AttributeItem idItem = {.string = id.c_str()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ID, &idItem);
  return *this;
}

std::string ArkUINode::getId() const {
  auto idItem =
      NativeNodeApi::getInstance()->getAttribute(m_nodeHandle, NODE_ID);
  RNOH_ASSERT(idItem != nullptr);
  return idItem->string;
}

ArkUINode& ArkUINode::setBackgroundColor(
    facebook::react::SharedColor const& color) {
  // TODO: figure out if we need to update (to a clear value), or return early
  // here
  uint32_t colorValue = facebook::react::isColorMeaningful(color)
      ? *color
      : *facebook::react::clearColor();
  setBackgroundColor(colorValue);
  return *this;
}

ArkUINode& ArkUINode::setBackgroundColor(uint32_t color) {
  ArkUI_NumberValue preparedColorValue[] = {{.u32 = color}};
  ArkUI_AttributeItem colorItem = {
      preparedColorValue,
      sizeof(preparedColorValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_BACKGROUND_COLOR, &colorItem);
  return *this;
}

namespace {
// Gradient line length for a linear gradient given the direction and
// container size. Follows the CSS Images spec:
// corner keywords are converted to an aspect-ratio-dependent angle, then the
// length is |W*sin(a)| + |H*cos(a)|. Only needed to resolve pixel (Point)
// stop positions; percent and auto-placed stops do not depend on it.
float computeGradientLineLength(
    facebook::react::LinearGradient const& gradient,
    float width,
    float height) {
  constexpr float kPi = 3.14159265358979323846f;
  constexpr float kRadPerDeg = kPi / 180.0f;
  float angleDeg = 180.0f;
  // RN 0.88: GradientDirection 改为 std::variant<Float, GradientKeyword>
  if (std::holds_alternative<facebook::react::Float>(gradient.direction)) {
    angleDeg = static_cast<float>(
        std::get<facebook::react::Float>(gradient.direction));
  } else {
    auto keyword =
        std::get<facebook::react::GradientKeyword>(gradient.direction);
    switch (keyword) {
      case facebook::react::GradientKeyword::ToTopRight:
        angleDeg = 90.0f - std::atan2(width, height) / kRadPerDeg;
        break;
      case facebook::react::GradientKeyword::ToBottomRight:
        angleDeg = std::atan2(width, height) / kRadPerDeg + 90.0f;
        break;
      case facebook::react::GradientKeyword::ToTopLeft:
        angleDeg = std::atan2(width, height) / kRadPerDeg + 270.0f;
        break;
      case facebook::react::GradientKeyword::ToBottomLeft:
        angleDeg = std::atan2(height, width) / kRadPerDeg + 180.0f;
        break;
    }
  }
  float rad = angleDeg * kRadPerDeg;
  return std::fabs(width * std::sin(rad)) + std::fabs(height * std::cos(rad));
}

// Resolves color stop positions per CSS spec, following the
// getFixedColorStops algorithm:
//  - Percent: value / 100
//  - Point (px): value / gradientLineLength (guarded when length is 0)
//  - Undefined: auto-placed (first->0, last->1, runs evenly distributed
//    between defined neighbors, monotonically clamped)
void resolveColorStops(
    std::vector<facebook::react::ColorStop> const& colorStops,
    float gradientLineLength,
    std::vector<uint32_t>& colors,
    std::vector<float>& stops) {
  struct Resolved {
    uint32_t color;
    float position;
    bool defined;
  };
  std::vector<Resolved> resolved;
  resolved.reserve(colorStops.size());
  for (auto const& cs : colorStops) {
    Resolved r{};
    r.color = *cs.color;
    r.defined = false;
    if (cs.position.unit == facebook::react::UnitType::Percent) {
      r.position = cs.position.value / 100.0f;
      r.defined = true;
    } else if (cs.position.unit == facebook::react::UnitType::Point) {
      if (gradientLineLength > 0.0f) {
        r.position = cs.position.value / gradientLineLength;
        r.defined = true;
      }
      // When length is unknown (not yet laid out), leave undefined so the
      // auto-placement handles it; re-applied on layout change.
    }
    resolved.push_back(r);
  }
  if (resolved.empty()) {
    return;
  }
  // Step 1: first/last undefined -> 0/1.
  if (!resolved.front().defined) {
    resolved.front().position = 0.0f;
    resolved.front().defined = true;
  }
  if (!resolved.back().defined) {
    resolved.back().position = 1.0f;
    resolved.back().defined = true;
  }
  // Step 2: clamp monotonically non-decreasing.
  float maxSoFar = resolved.front().position;
  for (size_t i = 1; i < resolved.size(); ++i) {
    if (resolved[i].defined) {
      if (resolved[i].position < maxSoFar) {
        resolved[i].position = maxSoFar;
      }
      maxSoFar = resolved[i].position;
    }
  }
  // Step 3: evenly distribute runs of undefined between defined stops.
  size_t lastDefined = 0;
  for (size_t i = 1; i < resolved.size(); ++i) {
    if (resolved[i].defined) {
      size_t unpositioned = i - lastDefined - 1;
      if (unpositioned > 0) {
        float start = resolved[lastDefined].position;
        float end = resolved[i].position;
        float increment = (end - start) / static_cast<float>(unpositioned + 1);
        for (size_t j = 1; j <= unpositioned; ++j) {
          resolved[lastDefined + j].position =
              start + increment * static_cast<float>(j);
        }
      }
      lastDefined = i;
    }
  }
  colors.reserve(resolved.size());
  stops.reserve(resolved.size());
  for (auto const& r : resolved) {
    colors.push_back(r.color);
    stops.push_back(r.position);
  }
}
} // namespace

ArkUINode& ArkUINode::setBackgroundImage(
    facebook::react::BackgroundImage const& backgroundImage,
    float width,
    float height) {
  if (!std::holds_alternative<facebook::react::LinearGradient>(
          backgroundImage)) {
    DLOG(INFO) << "[BGIMG] not a LinearGradient, skipping";
    return *this;
  }
  const auto& linearGradient =
      std::get<facebook::react::LinearGradient>(backgroundImage);

  std::vector<uint32_t> colors;
  std::vector<float> stops;
  float gradientLineLength =
      computeGradientLineLength(linearGradient, width, height);
  resolveColorStops(
      linearGradient.colorStops, gradientLineLength, colors, stops);

  DLOG(INFO) << "[BGIMG] LinearGradient colorStops="
             << linearGradient.colorStops.size()
             << " gradientLineLength=" << gradientLineLength
             << " width=" << width << " height=" << height;

  ArkUI_ColorStop colorStop = {
      .colors = colors.data(),
      .stops = stops.data(),
      .size = static_cast<int>(colors.size())};

  // Decide direction parameters.
  // CSS gradient angle: 0deg = bottom->top, 90deg = left->right, clockwise.
  // ArkUI NODE_LINEAR_GRADIENT with ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM:
  //   .value[0].f32 = angle, 0 = bottom->top, 180 = top->bottom, clockwise.
  // This matches CSS semantics directly.
  float angle = 180.0f;
  int32_t direction = static_cast<int32_t>(
      ArkUI_LinearGradientDirection::ARKUI_LINEAR_GRADIENT_DIRECTION_CUSTOM);

  // RN 0.88: GradientDirection 改为 std::variant<Float, GradientKeyword>
  if (std::holds_alternative<facebook::react::Float>(
          linearGradient.direction)) {
    angle = std::get<facebook::react::Float>(linearGradient.direction);
    DLOG(INFO) << "[BGIMG] direction=Angle, angle=" << angle;
  } else {
    auto keyword = std::get<facebook::react::GradientKeyword>(
        linearGradient.direction);
    direction = static_cast<int32_t>(
        ArkUI_LinearGradientDirection::ARKUI_LINEAR_GRADIENT_DIRECTION_BOTTOM);
    switch (keyword) {
      case facebook::react::GradientKeyword::ToTopRight:
        direction =
            static_cast<int32_t>(ArkUI_LinearGradientDirection::
                                     ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_TOP);
        break;
      case facebook::react::GradientKeyword::ToBottomRight:
        direction = static_cast<int32_t>(
            ArkUI_LinearGradientDirection::
                ARKUI_LINEAR_GRADIENT_DIRECTION_RIGHT_BOTTOM);
        break;
      case facebook::react::GradientKeyword::ToTopLeft:
        direction =
            static_cast<int32_t>(ArkUI_LinearGradientDirection::
                                     ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_TOP);
        break;
      case facebook::react::GradientKeyword::ToBottomLeft:
        direction = static_cast<int32_t>(
            ArkUI_LinearGradientDirection::
                ARKUI_LINEAR_GRADIENT_DIRECTION_LEFT_BOTTOM);
        break;
    }
    DLOG(INFO) << "[BGIMG] direction=Keyword, dirEnum=" << direction;
  }

  ArkUI_NumberValue gradientValue[] = {
      {.f32 = angle},
      {.i32 = direction},
      {.i32 = 0},
  };
  ArkUI_AttributeItem gradientItem = {
      .value = gradientValue,
      .size = sizeof(gradientValue) / sizeof(ArkUI_NumberValue),
      .object = &colorStop};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_LINEAR_GRADIENT, &gradientItem);
  DLOG(INFO) << "[BGIMG] setAttribute NODE_LINEAR_GRADIENT done, angle="
             << angle << " dir=" << direction << " numColors=" << colors.size();
  return *this;
}

ArkUINode& ArkUINode::setBackgroundImage(
    std::vector<facebook::react::BackgroundImage> const& backgroundImage,
    float width,
    float height) {
  if (backgroundImage.empty()) {
    return resetBackgroundImage();
  }
  // Only the first background image layer is applied.
  return setBackgroundImage(backgroundImage[0], width, height);
}

ArkUINode &ArkUINode::resetBackgroundImage() {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_LINEAR_GRADIENT);
    return *this;
}

ArkUINode& ArkUINode::setTransform(
    facebook::react::Transform const& transform,
    facebook::react::Float pointScaleFactor) {
  if (pointScaleFactor == 0.0) {
    // NOTE: while pointScaleFactor shouldn't logically be zero, we probably
    // don't want to assert it in case some exotic use case requires it
    return *this;
  }

  // NOTE: ArkUI translation is in `px` units, while React Native uses `vp`
  // units, so we need to correct for the scale factor here
  // We do so by first scaling the view down (to vp), applying the transform,
  // and then scaling the transformed view back up (to px)
  auto preScale = facebook::react::Transform::Scale(
      1 / pointScaleFactor, 1 / pointScaleFactor, 1 / pointScaleFactor);
  auto postScale = facebook::react::Transform::Scale(
      pointScaleFactor, pointScaleFactor, pointScaleFactor);

  auto matrix = (postScale * transform * preScale).matrix;

  std::array<ArkUI_NumberValue, 16> transformValue;
  for (int i = 0; i < 16; i++) {
    transformValue[i] = {.f32 = static_cast<float>(matrix[i])};
  }

  ArkUI_AttributeItem transformItem = {
      transformValue.data(), transformValue.size()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TRANSFORM, &transformItem);
  return *this;
}

ArkUINode& ArkUINode::setTranslate(float x, float y, float z) {
  ArkUI_NumberValue translateValue[] = {{.f32 = x}, {.f32 = y}, {.f32 = z}};
  ArkUI_AttributeItem translateItem = {
      translateValue, sizeof(translateValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TRANSLATE, &translateItem);
  return *this;
}

ArkUINode& ArkUINode::setOpacity(facebook::react::Float opacity) {
  ArkUI_NumberValue opacityValue[] = {{.f32 = (float)opacity}};
  ArkUI_AttributeItem opacityItem = {
      opacityValue, sizeof(opacityValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_OPACITY, &opacityItem);
  return *this;
}

ArkUINode& ArkUINode::setClip(bool clip) {
  auto isClip = static_cast<uint32_t>(clip);
  ArkUI_NumberValue clipValue[] = {{.u32 = isClip}};
  ArkUI_AttributeItem clipItem = {
      clipValue, sizeof(clipValue) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_CLIP, &clipItem);
  return *this;
}

ArkUINode& ArkUINode::setAlignment(Alignment alignment) {
  ArkUI_NumberValue alignmentValue[] = {
      {.i32 = static_cast<int32_t>(alignment)}};
  ArkUI_AttributeItem alignmentItem = {
      alignmentValue, sizeof(alignmentValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ALIGNMENT, &alignmentItem);
  return *this;
}

ArkUINode& ArkUINode::setTranslateTransition(
    float translateX,
    float translateY,
    int32_t animationDurationMillis) {
  std::array<ArkUI_NumberValue, 5> translateValue = {
      ArkUI_NumberValue{.f32 = translateX},
      {.f32 = translateY},
      {.f32 = 0},
      {.i32 = animationDurationMillis},
      {.i32 = static_cast<int32_t>(ARKUI_CURVE_LINEAR)}};
  ArkUI_AttributeItem translateItem = {
      translateValue.data(), translateValue.size()};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TRANSLATE_TRANSITION, &translateItem);
  return *this;
}

ArkUINode& ArkUINode::resetTranslateTransition() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_TRANSLATE_TRANSITION);
  return *this;
}

ArkUINode& ArkUINode::setOpacityTransition(int32_t animationDurationMillis) {
  std::array<ArkUI_NumberValue, 3> args = {
      ArkUI_NumberValue{.f32 = 0},
      {.i32 = animationDurationMillis},
      {.i32 = static_cast<int32_t>(ARKUI_CURVE_LINEAR)}};
  ArkUI_AttributeItem opacityItem = {args.data(), args.size()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_OPACITY_TRANSITION, &opacityItem);
  return *this;
}

ArkUINode& ArkUINode::resetOpacityTransition() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_OPACITY_TRANSITION);
  return *this;
}

ArkUINode& ArkUINode::setOffset(float x, float y) {
  ArkUI_NumberValue offsetValue[] = {{.f32 = x}, {.f32 = y}};
  ArkUI_AttributeItem offsetItem = {
      offsetValue, sizeof(offsetValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_OFFSET, &offsetItem);
  return *this;
}

ArkUINode& ArkUINode::setEnabled(bool enabled) {
  ArkUI_NumberValue value = {.i32 = int32_t(enabled)};
  ArkUI_AttributeItem item = {&value, 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ENABLED, &item);
  return *this;
}

ArkUINode& ArkUINode::resetAccessibilityText() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_ACCESSIBILITY_TEXT);
  return *this;
}

void ArkUINode::onTouchIntercept(const ArkUI_UIInputEvent* event) {
  if (m_arkUINodeDelegate != nullptr) {
    m_arkUINodeDelegate->onArkUINodeTouchIntercept(event);
  }
}

bool ArkUINode::isFocused() {
  return m_nodeApi->getAttribute(m_nodeHandle, NODE_FOCUS_STATUS)->value[0].i32;
}

void ArkUINode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  switch (eventType) {
    case ArkUI_NodeEventType::NODE_ON_ACCESSIBILITY_ACTIONS: {
      auto maybeActionName = actionTypeToName(
          static_cast<ArkUI_AccessibilityActionType>(eventArgs[0].u32));
      if (!maybeActionName.has_value()) {
        DLOG(WARNING) << "Unsupported action type: " << eventArgs[0].u32;
        return;
      }
      const auto& actionName = maybeActionName.value();
      if (m_arkUINodeDelegate == nullptr) {
        DLOG(WARNING) << "Cancelled " << actionName << " � delegate is nullptr";
        return;
      }
      m_arkUINodeDelegate->onArkUINodeAccessibilityAction(this, actionName);
      break;
    }
  }
}

void ArkUINode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    std::string_view eventString) {}

void ArkUINode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    ArkUI_NodeEvent* event) {}

ArkUINode& ArkUINode::setFocusStatus(int32_t focus) {
  std::array<ArkUI_NumberValue, 1> value = {{{.i32 = focus}}};
  ArkUI_AttributeItem item = {value.data(), value.size()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_FOCUS_STATUS, &item);
  return *this;
}

ArkUINode&
ArkUINode::setMargin(float left, float top, float right, float bottom) {
  ArkUI_NumberValue value[] = {
      {.f32 = top}, {.f32 = right}, {.f32 = bottom}, {.f32 = left}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_MARGIN, &item);
  return *this;
}

ArkUINode&
ArkUINode::setPadding(float left, float top, float right, float bottom) {
  ArkUI_NumberValue value[] = {
      {.f32 = top}, {.f32 = right}, {.f32 = bottom}, {.f32 = left}};
  ArkUI_AttributeItem item = {.value = value, .size = 4};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_PADDING, &item);
  return *this;
}

ArkUINode& ArkUINode::setVisibility(ArkUI_Visibility visibility) {
  ArkUI_NumberValue value[] = {{.i32 = visibility}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_VISIBILITY, &item);
  return *this;
}

ArkUINode& ArkUINode::setZIndex(float index) {
  std::array<ArkUI_NumberValue, 1> values = {{{.f32 = index}}};
  ArkUI_AttributeItem item = {values.data(), values.size()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_Z_INDEX, &item);
  return *this;
}

ArkUINode& ArkUINode::setRenderGroup(bool renderOffscreen) {
  ArkUI_NumberValue value[] = {{.i32 = (int32_t)renderOffscreen}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_RENDER_GROUP, &item);
  return *this;
}

ArkUI_IntOffset ArkUINode::getLayoutPosition() {
  return NativeNodeApi::getInstance()->getLayoutPosition(m_nodeHandle);
}

void ArkUINode::registerNodeEvent(ArkUI_NodeEventType eventType) {
  maybeThrow(NativeNodeApi::getInstance()->registerNodeEvent(
      m_nodeHandle, eventType, eventType, this));
}

void ArkUINode::unregisterNodeEvent(ArkUI_NodeEventType eventType) {
  NativeNodeApi::getInstance()->unregisterNodeEvent(m_nodeHandle, eventType);
}

const ArkUI_AttributeItem& ArkUINode::getAttribute(
    ArkUI_NodeAttributeType attribute) const {
  auto item =
      NativeNodeApi::getInstance()->getAttribute(m_nodeHandle, attribute);
  if (!item) {
    auto message = std::string("ArkUINode getAttribute failed: ") +
        std::to_string((attribute));
    LOG(ERROR) << message;
    throw std::runtime_error(std::move(message));
  }
  return *item;
}

ArkUINode& ArkUINode::setDirection(ArkUI_Direction direction) {
  ArkUI_NumberValue value = {.u32 = direction};
  ArkUI_AttributeItem item = {.value = &value, .size = 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_DIRECTION, &item);
  return *this;
}
ArkUINode& ArkUINode::setBlur(facebook::react::Float blur) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(blur)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_BLUR, &item);
  return *this;
}

ArkUINode& ArkUINode::setGrayscale(facebook::react::Float grayscale) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(grayscale)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_GRAY_SCALE, &item);
  return *this;
}

ArkUINode& ArkUINode::setBrightness(facebook::react::Float brightness) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(brightness)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_BRIGHTNESS, &item);
  return *this;
}

ArkUINode& ArkUINode::setInvert(facebook::react::Float invert) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(invert)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_INVERT, &item);
  return *this;
}

ArkUINode& ArkUINode::setSaturate(facebook::react::Float saturate) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(saturate)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_SATURATION, &item);
  return *this;
}

ArkUINode& ArkUINode::setSepia(facebook::react::Float sepia) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(sepia)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_SEPIA, &item);
  return *this;
}

ArkUINode& ArkUINode::setContrast(facebook::react::Float contrast) {
  ArkUI_NumberValue value[] = {{.f32 = static_cast<float>(contrast)}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_CONTRAST, &item);
  return *this;
}

ArkUINode& ArkUINode::resetBlur() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_BLUR);
  return *this;
}

ArkUINode& ArkUINode::resetGrayscale() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_GRAY_SCALE);
  return *this;
}

ArkUINode& ArkUINode::resetBrightness() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_BRIGHTNESS);
  return *this;
}

ArkUINode& ArkUINode::resetInvert() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_INVERT);
  return *this;
}

ArkUINode& ArkUINode::resetSaturate() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_SATURATION);
  return *this;
}

ArkUINode& ArkUINode::resetSepia() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_SEPIA);
  return *this;
}

ArkUINode& ArkUINode::resetContrast() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_CONTRAST);
  return *this;
}

ArkUINode& ArkUINode::resetOutlineWidth() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_OUTLINE_WIDTH);
  return *this;
}

ArkUINode& ArkUINode::resetOutlineColors() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_OUTLINE_COLOR);
  return *this;
}

void ArkUINode::setAttribute(
    ArkUI_NodeAttributeType attribute,
    ArkUI_AttributeItem const& item) {
  m_nodeApi->setAttribute(m_nodeHandle, attribute, &item);
}

void ArkUINode::setAttribute(
    ArkUI_NodeAttributeType attribute,
    std::initializer_list<ArkUI_NumberValue> values) {
  int32_t size = values.size();
  ArkUI_AttributeItem item{.value = std::data(values), .size = size};
  setAttribute(attribute, item);
}

} // namespace rnoh
