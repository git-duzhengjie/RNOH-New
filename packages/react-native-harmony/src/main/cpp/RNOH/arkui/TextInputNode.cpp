/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextInputNode.h"
#include <cmath>
#include "NativeNodeApi.h"
#include "RNOH/ApiVersionCheck.h"
#include "RNOH/arkui/DynamicArkUILoader.h"
#include "RNOH/arkui/conversions.h"
#include "conversions.h"

static constexpr std::array TEXT_INPUT_NODE_EVENT_TYPES = {
    NODE_TEXT_INPUT_ON_PASTE,
    NODE_TEXT_INPUT_ON_CUT,
    NODE_TEXT_INPUT_ON_SUBMIT,
    NODE_ON_FOCUS,
    NODE_ON_BLUR,
    NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE,
    NODE_TEXT_INPUT_ON_CONTENT_SCROLL,
    NODE_TEXT_INPUT_ON_CONTENT_SIZE_CHANGE,
    NODE_TEXT_INPUT_ON_WILL_DELETE,
    NODE_EVENT_ON_APPEAR};
namespace rnoh {

// ARKUI_TEXTAREA_TYPE_ONE_TIME_CODE only support
// API version >= 20, using const to avoid possible compiling issue.
const int32_t TEXT_INPUT_TYPE_ONE_TIME_CODE = 14;

TextInputNode::TextInputNode(const ArkUINode::Context::Shared& context)
    : TextInputNodeBase(context, ArkUI_NodeType::ARKUI_NODE_TEXT_INPUT),
      m_textInputNodeDelegate(nullptr) {
  if (IsAtLeastApi15()) {
    registerNodeEvent(NODE_TEXT_INPUT_ON_CHANGE_WITH_PREVIEW_TEXT);
  } else {
    registerNodeEvent(NODE_TEXT_INPUT_ON_CHANGE);
  }
  for (auto eventType : TEXT_INPUT_NODE_EVENT_TYPES) {
    registerNodeEvent(eventType);
  }

  ArkUI_NumberValue value = {.i32 = 1};
  ArkUI_AttributeItem item = {&value, 1};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_ENABLE_KEYBOARD_ON_FOCUS, &item);
}

TextInputNode::~TextInputNode() {
  if (IsAtLeastApi15()) {
    unregisterNodeEvent(NODE_TEXT_INPUT_ON_CHANGE_WITH_PREVIEW_TEXT);
  } else {
    unregisterNodeEvent(NODE_TEXT_INPUT_ON_CHANGE);
  }
  for (auto eventType : TEXT_INPUT_NODE_EVENT_TYPES) {
    unregisterNodeEvent(eventType);
  }
}

void TextInputNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    EventArgs& eventArgs) {
  ArkUINode::onNodeEvent(eventType, eventArgs);
  if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_SUBMIT) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onSubmit();
      m_textInputNodeDelegate->onSubmit(this);
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_ON_FOCUS) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onFocus();
      m_textInputNodeDelegate->onFocus(this);
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_ON_BLUR) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onBlur();
      m_textInputNodeDelegate->onBlur(this);
    }
  } else if (
      eventType ==
      ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_TEXT_SELECTION_CHANGE) {
    if (m_textInputNodeDelegate != nullptr) {
      int32_t selectionLocation = eventArgs[0].i32;
      int32_t selectionLength = eventArgs[1].i32 - eventArgs[0].i32;
      m_textInputNodeDelegate->onTextSelectionChange(
          selectionLocation, selectionLength);
      m_textInputNodeDelegate->onTextSelectionChange(
          this, selectionLocation, selectionLength);
    }
  } else if (
      eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CONTENT_SCROLL) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onContentScroll();
      m_textInputNodeDelegate->onContentScroll(this);
    }
  } else if (
      eventType ==
      ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CONTENT_SIZE_CHANGE) {
    if (m_textInputNodeDelegate != nullptr) {
      float width = eventArgs[0].f32;
      float height = eventArgs[1].f32;
      m_textInputNodeDelegate->onContentSizeChange(width, height, true);
      m_textInputNodeDelegate->onContentSizeChange(this, width, height);
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_EVENT_ON_APPEAR) {
    if (m_autoFocus) {
      setFocusStatus(1);
    }
  }
}

void TextInputNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    std::string_view eventString) {
  if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CHANGE) {
    // If your app requires a minimum API version of 15, you can delete this
    // code, as changes to the input field will be handled via
    // NODE_TEXT_INPUT_ON_CHANGE_WITH_PREVIEW_TEXT.
    std::string text(eventString);
    onChange(text);
  } else if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_PASTE) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onPasteOrCut();
      m_textInputNodeDelegate->onPasteOrCut(this);
    }
  } else if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CUT) {
    if (m_textInputNodeDelegate != nullptr) {
      m_textInputNodeDelegate->onPasteOrCut();
      m_textInputNodeDelegate->onPasteOrCut(this);
    }
  }
}

void TextInputNode::onNodeEvent(
    ArkUI_NodeEventType eventType,
    ArkUI_NodeEvent* event) {
  if (eventType == ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_WILL_DELETE) {
    ArkUI_NumberValue arkUiValues[2];
    maybeThrow(OH_ArkUI_NodeEvent_GetNumberValue(event, 0, arkUiValues));
    maybeThrow(OH_ArkUI_NodeEvent_GetNumberValue(event, 1, arkUiValues));

    ArkUI_NumberValue ret[] = {
        ArkUI_NumberValue{1},
    };
    // OH_ArkUI_NodeEvent_SetReturnNumberValue copies the value, so passing this
    // as a pointer to a variable on the stack should be safe.  (check the
    // arkui_ace_engine repository).
    // We need to set this or else the TextInput won't delete the character.
    maybeThrow(OH_ArkUI_NodeEvent_SetReturnNumberValue(event, ret, 1));
    m_textInputNodeDelegate->onWillDelete(
        this, static_cast<int>(round(arkUiValues[0].f32)), arkUiValues[1].i32);
  }
  if (DynamicArkUILoader::getTextChangeEventFun() &&
      eventType ==
          ArkUI_NodeEventType::NODE_TEXT_INPUT_ON_CHANGE_WITH_PREVIEW_TEXT) {
    auto changeEvent = DynamicArkUILoader::getTextChangeEventFun()(event);
    auto position = changeEvent->number;
    std::string content(changeEvent->pStr);
    std::string extendStr(changeEvent->pExtendStr);
    if (position >= 0) {
      auto bytePos = charToByteIndex(content, position);
      content.insert(bytePos, extendStr);
    }
    onChange(content, extendStr);
  }
}

void TextInputNode::onChange(
    const std::string& text,
    const std::string& extendStr) {
  if (m_textInputNodeDelegate != nullptr) {
    if (m_hasRNSetTextContext && text == m_textContent) {
      m_hasRNSetTextContext = false;
    } else {
      m_hasRNSetTextContext = false;
      m_textInputNodeDelegate->onChange(this, text, std::move(extendStr));
      m_textInputNodeDelegate->onChange(std::move(text), std::move(extendStr));
    }
  }
}

void TextInputNode::setTextInputNodeDelegate(
    TextInputNodeDelegate* textInputNodeDelegate) {
  m_textInputNodeDelegate = textInputNodeDelegate;
}

facebook::react::Point TextInputNode::getTextInputOffset() const {
  auto value = NativeNodeApi::getInstance()
                   ->getAttribute(m_nodeHandle, NODE_OFFSET)
                   ->value;
  facebook::react::Float x = value->i32;
  value++;
  facebook::react::Float y = value->i32;
  return facebook::react::Point{x, y};
}

facebook::react::Rect TextInputNode::getTextContentRect() const {
  auto value = getAttribute(NODE_TEXT_INPUT_CONTENT_RECT).value;
  facebook::react::Float x = value[0].f32;
  facebook::react::Float y = value[1].f32;
  facebook::react::Float width = value[2].f32;
  facebook::react::Float height = value[3].f32;
  return {x, y, width, height};
}

void TextInputNode::setTextContent(std::string const& textContent) {
  ArkUI_AttributeItem item = {.string = textContent.c_str()};
  m_hasRNSetTextContext = true;
  m_textContent = textContent;
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_TEXT, &item);
}

void TextInputNode::setSelectedBackgroundColor(
    facebook::react::SharedColor const& color) {
  ArkUI_NumberValue selectedBackgroundColor = {
      .u32 = TextInputNodeBase::convertColorToTranslucentSelection(color)};
  ArkUI_AttributeItem colorItem = {
      &selectedBackgroundColor, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR, &colorItem);
}

void TextInputNode::setCaretHidden(bool hidden) {
  if (hidden) {
    ArkUI_NumberValue value = {.f32 = 0};
    ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CARET_STYLE, &item);

    /**
     * Following setting is trying to set caret as transparent when
     * user sets caretHidden as true and selects input content
     * in TextInput Component.
     */
    value = {.u32 = 0};
    item = {&value, sizeof(ArkUI_NumberValue)};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CARET_COLOR, &item);
  } else {
    ArkUI_NumberValue value = {
        .f32 = 2}; // The default width of the cursor in ArkUI is 2 vp
    ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CARET_STYLE, &item);

    value = {.u32 = m_caretColorValue};
    item = {&value, sizeof(ArkUI_NumberValue)};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CARET_COLOR, &item);
  }
}

void TextInputNode::setInputType(
    facebook::react::KeyboardType rawKeyboardType,
    bool securityEntry) {
  ArkUI_TextInputType keyboardType =
      TextInputNodeBase::convertInputType(rawKeyboardType);
  if (securityEntry) {
    keyboardType = ARKUI_TEXTINPUT_TYPE_PASSWORD;
    this->setPasswordIconVisibility(false);
  }
  ArkUI_NumberValue value = {.i32 = keyboardType};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_TYPE, &item);
}

void TextInputNode::setInputType(ArkUI_TextInputType keyboardType) {
  ArkUI_NumberValue value = {.i32 = keyboardType};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_TYPE, &item);
}

void TextInputNode::setPasswordIconVisibility(bool isVisible) {
  ArkUI_NumberValue value = {.i32 = isVisible ? 1 : 0};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SHOW_PASSWORD_ICON, &item);
}

void TextInputNode::setEnterKeyType(
    facebook::react::ReturnKeyType rawReturnKeyType,
    std::string rawReturnKeyLabel) {
  auto returnKeyType = TextInputNodeBase::convertEnterKeyType(rawReturnKeyType);
  if (rawReturnKeyType == facebook::react::ReturnKeyType::Default) {
    returnKeyType = TextInputNodeBase::convertEnterKeyLabel(rawReturnKeyLabel);
  }
  ArkUI_NumberValue value = {.i32 = returnKeyType};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_ENTER_KEY_TYPE, &item);
}
void TextInputNode::setEnterKeyType(ArkUI_EnterKeyType returnKeyType) {
  ArkUI_NumberValue value = {.i32 = returnKeyType};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_ENTER_KEY_TYPE, &item);
}

void TextInputNode::setCancelButtonMode(
    facebook::react::TextInputAccessoryVisibilityMode mode) {
  int32_t cancelButtonStyle =
      static_cast<int32_t>(ARKUI_CANCELBUTTON_STYLE_INVISIBLE);
  if (mode == facebook::react::TextInputAccessoryVisibilityMode::Always) {
    cancelButtonStyle = static_cast<int32_t>(ARKUI_CANCELBUTTON_STYLE_CONSTANT);
  } else if (
      mode == facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
    cancelButtonStyle = static_cast<int32_t>(ARKUI_CANCELBUTTON_STYLE_INPUT);
  }

  ArkUI_NumberValue value[] = {{.i32 = cancelButtonStyle}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};

  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CANCEL_BUTTON, &item);
}

void TextInputNode::setFont(
    facebook::react::TextAttributes const& textAttributes) {
  TextInputNodeBase::setCommonFontAttributes(textAttributes);

  std::string fontFamily = "HarmonyOS Sans";
  if (!textAttributes.fontFamily.empty()) {
    fontFamily = textAttributes.fontFamily;
  }

  auto fontWeight = ARKUI_FONT_WEIGHT_NORMAL;
  if (textAttributes.fontWeight.has_value()) {
    fontWeight = rnoh::convertFontWeight(textAttributes.fontWeight.value());
  }

  auto fontStyle = ARKUI_FONT_STYLE_NORMAL;
  if (textAttributes.fontStyle.has_value() &&
      textAttributes.fontStyle.value() == facebook::react::FontStyle::Italic) {
    fontStyle = ARKUI_FONT_STYLE_ITALIC;
  }

  float fontSize = 16;
  if (!std::isnan(textAttributes.fontSize)) {
    fontSize = static_cast<float>(textAttributes.fontSize);
  }
  bool allowFontScaling = true;
  if (textAttributes.allowFontScaling.has_value()) {
    allowFontScaling = textAttributes.allowFontScaling.value();
  }

  std::array<ArkUI_NumberValue, 3> value = {
      {{.f32 = fontSize},
       {.i32 = static_cast<int32_t>(fontStyle)},
       {.i32 = static_cast<int32_t>(fontWeight)}}};
  ArkUI_AttributeItem item = {value.data(), value.size(), fontFamily.c_str()};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER_FONT, &item);
}

void TextInputNode::setLineHeight(float lineHeight) {
  ArkUI_NumberValue value[] = {{.f32 = lineHeight}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_LINE_HEIGHT, &item);
}

void TextInputNode::setCaretColor(facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  m_caretColorValue = *color;
  ArkUI_NumberValue value = {.u32 = colorValue};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CARET_COLOR, &item);
}

void TextInputNode::setUnderlineColor(
    facebook::react::SharedColor const& underlineColor) {
  if (*underlineColor >> 24 == 0) {
    return;
  }
  ArkUI_NumberValue showValue = {.i32 = 1};
  ArkUI_AttributeItem showItem = {&showValue, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SHOW_UNDERLINE, &showItem);
  ArkUI_NumberValue value[] = {
      {.u32 = TextInputNodeBase::convertColorToTranslucentUnderline(
           underlineColor)},
      {.u32 = TextInputNodeBase::convertColorToTranslucentUnderline(
           underlineColor)},
      {.u32 = TextInputNodeBase::convertColorToTranslucentUnderline(
           underlineColor)},
      {.u32 = TextInputNodeBase::convertColorToTranslucentUnderline(
           underlineColor)}};

  ArkUI_AttributeItem item = {
      .value = value, .size = sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_UNDERLINE_COLOR, &item);
}

void TextInputNode::setMaxLength(int32_t maxLength) {
  ArkUI_NumberValue value = {.i32 = maxLength};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_MAX_LENGTH, &item);
}

void TextInputNode::setPlaceholder(std::string const& placeholder) {
  ArkUI_AttributeItem item = {.string = placeholder.c_str()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER, &item);
}

void TextInputNode::setPlaceholderColor(
    facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  ArkUI_NumberValue value = {.u32 = colorValue};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_PLACEHOLDER_COLOR, &item);
}

void TextInputNode::resetSelectedBackgroundColor() {
  m_nodeApi->resetAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SELECTED_BACKGROUND_COLOR);
}

void TextInputNode::setTextContentType(std::string const& textContentType) {
  if (textContentType == "oneTimeCode") {
    ArkUI_NumberValue value = {.i32 = TEXT_INPUT_TYPE_ONE_TIME_CODE};
    ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_TYPE, &item);
    return;
  }
  ArkUI_NumberValue type =
      TextInputNodeBase::convertContentType(textContentType);
  if (type.i32 == -1) {
    this->setAutoFill(false); // The purpose is to fix the issue where an
                              // autofill bubble still pops up when
                              // textContentType is dynamically changed to none.
  } else {
    this->setAutoFill(true);
  }
  std::array<ArkUI_NumberValue, 1> value = {type};
  ArkUI_AttributeItem item = {value.data(), value.size()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CONTENT_TYPE, &item);
}

void TextInputNode::setAutoFill(bool autoFill) {
  uint32_t isAutoFill = static_cast<uint32_t>(autoFill);
  ArkUI_NumberValue value = {.u32 = isAutoFill};
  ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_ENABLE_AUTO_FILL, &item);
}

void TextInputNode::setAutoFill(std::string const& autoFill) {
  bool enableAutoFill = true;
  if (autoFill == "no" || autoFill == "noExcludeDescendants" ||
      autoFill == "auto") {
    enableAutoFill = false;
  } else if (autoFill == "yes" || autoFill == "yesExcludeDescendants") {
    enableAutoFill = true;
  } else {
    LOG(WARNING) << "Invalid ImportantForAutofill";
    enableAutoFill = true;
  }
  this->setAutoFill(enableAutoFill);
}

void TextInputNode::setBlurOnSubmit(bool blurOnSubmit) {
  ArkUI_NumberValue value = {.i32 = int32_t(blurOnSubmit)};
  ArkUI_AttributeItem item = {&value, 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_BLUR_ON_SUBMIT, &item);
}

void TextInputNode::setShowKeyboardOnFocus(bool enable) {
  ArkUI_NumberValue value = {.i32 = int32_t(enable)};
  ArkUI_AttributeItem item = {&value, 1};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SHOW_KEYBOARD_ON_FOCUS, &item);
}

void TextInputNode::setInputFilter(const std::string& inputFilter) {
  ArkUI_AttributeItem item = {.string = inputFilter.c_str()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_INPUT_FILTER, &item);
}

std::string TextInputNode::getTextContent() {
  auto item = NativeNodeApi::getInstance()->getAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_TEXT);
  return item->string;
}

void TextInputNode::setContextMenuHidden(bool hidden) {
  std::array<ArkUI_NumberValue, 1> value = {
      {{.i32 = static_cast<int32_t>(hidden)}}};
  ArkUI_AttributeItem item = {value.data(), value.size()};
  m_nodeApi->setAttribute(
      m_nodeHandle, NODE_TEXT_INPUT_SELECTION_MENU_HIDDEN, &item);
}

void TextInputNode::setPasswordRules(const std::string rules) {
  ArkUI_AttributeItem item = {.string = rules.c_str()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_PASSWORD_RULES, &item);
}

void TextInputNode::setAutoFocus(bool autoFocus) {
  m_autoFocus = autoFocus;
}
} // namespace rnoh
