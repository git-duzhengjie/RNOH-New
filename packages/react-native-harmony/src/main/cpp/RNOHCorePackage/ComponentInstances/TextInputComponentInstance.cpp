/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextInputComponentInstance.h"
#include <boost/locale.hpp>
#include <folly/dynamic.h>
#include <glog/logging.h>
#include <react/renderer/components/textinput/TextInputProps.h>
#include <react/renderer/components/textinput/TextInputState.h>
#include <react/renderer/core/ConcreteState.h>
#include <algorithm>
#include <sstream>
#include <utility>
#include "ScrollViewComponentInstance.h"
#include "conversions.h"
#include "react/renderer/components/textinput/primitives.h"

using RNTextVerticalAlignment = facebook::react::TextAlignmentVertical;

std::unordered_map<RNTextVerticalAlignment, ArkUI_Alignment>
    ARKUI_TEXT_ALIGNMENT_BY_RN_VERTICAL_TEXT_ALIGNMENT = {
        {RNTextVerticalAlignment::Auto, ARKUI_ALIGNMENT_CENTER},
        {RNTextVerticalAlignment::Top, ARKUI_ALIGNMENT_TOP},
        {RNTextVerticalAlignment::Bottom, ARKUI_ALIGNMENT_BOTTOM},
        {RNTextVerticalAlignment::Center, ARKUI_ALIGNMENT_CENTER}};

namespace rnoh {

TextInputComponentInstance::TextInputComponentInstance(Context context)
    : CppComponentInstance(std::move(context)),
      ArkTSMessageHub::Observer(m_deps->arkTSMessageHub),
      m_textInputNode(m_arkUINodeCtx),
      m_textAreaNode(m_arkUINodeCtx) {
  m_textInputNode.setInputFilter(".*");
  m_textAreaNode.setInputFilter(".*");

  m_textInputNode.setPadding(resolveEdges(
      [this](auto edge) { return m_props->yogaStyle.padding(edge); }));
  m_textAreaNode.setPadding(resolveEdges(
      [this](auto edge) { return m_props->yogaStyle.padding(edge); }));

  m_textAreaNode.setBackgroundColor(facebook::react::clearColor());
  m_textInputNode.setBackgroundColor(facebook::react::clearColor());
  m_textAreaNode.setCaretColor(facebook::react::blackColor());
  m_textInputNode.setCaretColor(facebook::react::blackColor());
}

void TextInputComponentInstance::onChange(
    ArkUINode* node,
    const std::string& text,
    std::string extendStr) {
  std::string previousContent = m_content;

  m_content = text;
  m_extendStr = extendStr;
  m_nativeEventCount++;
  int32_t contentLength = countUtf16Characters(m_content);
  if (contentLength == 0) {
    m_endOffset = 0;
  }
  int32_t location = std::max<int32_t>(contentLength - m_endOffset, 0);

  // Track IME preview state using a dedicated flag (m_inPreviewMode) rather
  // than m_extendStr, because m_extendStr gets cleared externally by
  // setTextAndSelection and onStateChanged between onChange calls.
  bool previewJustStarted = !m_inPreviewMode && !extendStr.empty();
  bool previewJustCommitted = m_inPreviewMode && extendStr.empty();

  if (previewJustStarted) {
    m_preEditContent = previousContent;
    m_preEditSelectionStart = m_selectionLocation;
    m_preEditSelectionLen = m_selectionLength;
    m_inPreviewMode = true;
  }
  if (previewJustCommitted) {
    m_inPreviewMode = false;
  }

  bool willDeleteHandled = m_willDeleteHandled;
  m_willDeleteHandled = false;

  if (willDeleteHandled) {
    // onWillDelete already emitted a Backspace for this deletion; skip to
    // avoid duplicate events.
  } else if (previewJustCommitted) {
    emitCommittedKeyPress(
        m_preEditContent,
        m_content,
        m_preEditSelectionStart,
        m_preEditSelectionLen);
  } else if (m_inPreviewMode && !extendStr.empty()) {
    if (location > m_selectionLocation) {
      onKeyPressChange(location, m_content);
    }
  } else {
    onKeyPressChange(location, m_content);
  }
  // Update cursor cache and content
  setTextSelection(location, location);
  if (m_eventEmitter) {
    m_eventEmitter->onChange(getTextInputMetrics());
    m_eventEmitter->onSelectionChange(getTextInputMetrics());
  }
  m_valueChanged = true;
}

void TextInputComponentInstance::onSubmit(ArkUINode* node) {
  if (m_eventEmitter) {
    m_eventEmitter->onSubmitEditing(getTextInputMetrics());
  }
}

void TextInputComponentInstance::onBlur(ArkUINode* node) {
  this->m_focused = false;
  if (m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
    m_textInputNode.setCancelButtonMode(
        facebook::react::TextInputAccessoryVisibilityMode::Never);
  } else if (
      m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
    m_textInputNode.setCancelButtonMode(
        facebook::react::TextInputAccessoryVisibilityMode::Always);
  }
  if (m_eventEmitter) {
    m_eventEmitter->onBlur(getTextInputMetrics());
    m_eventEmitter->onEndEditing(getTextInputMetrics());
  }
}

void TextInputComponentInstance::onFocus(ArkUINode* node) {
  this->m_focused = true;
  if (this->m_clearTextOnFocus) {
    m_endOffset = 0;
    setSelection(0, 0);
    setTextContent("");
  }
  if (m_props->traits.selectTextOnFocus) {
    m_textInputNode.setTextSelection(0, m_content.size());
    m_textAreaNode.setTextSelection(0, m_content.size());
  }
  if (m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
    m_textInputNode.setCancelButtonMode(m_props->traits.clearButtonMode);
  } else if (
      m_props->traits.clearButtonMode ==
      facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
    m_textInputNode.setCancelButtonMode(
        facebook::react::TextInputAccessoryVisibilityMode::Never);
  }
  if (m_eventEmitter) {
    m_eventEmitter->onFocus(getTextInputMetrics());
  }
}

void TextInputComponentInstance::onPasteOrCut(ArkUINode* node) {
  m_textWasPastedOrCut = true;
}

void TextInputComponentInstance::onWillDelete(
    ArkUINode* node,
    int position,
    int direction) {
  // onTextSelectionChange doesn't get triggered when backspace is pressed and
  // the TextInput is empty, so we cover that case here.
  if (m_eventEmitter && position == 0 && direction == 0) {
    auto keyPressMetrics =
        facebook::react::TextInputEventEmitter::KeyPressMetrics();
    m_nativeEventCount++;
    keyPressMetrics.text = "";
    keyPressMetrics.eventCount = m_nativeEventCount;
    m_eventEmitter->onKeyPress(keyPressMetrics);
    // Use the onChange event to synchronize eventCount to ensure normal
    // rendering during long-press deletion.
    m_eventEmitter->onChange(getTextInputMetrics());
    if (!m_content.empty()) {
      m_willDeleteHandled = true;
    }
  }
}

void TextInputComponentInstance::setSelection(
    int32_t location,
    int32_t length) {
  m_selectionLocation = location;
  m_selectionLength = length;
  m_selectionStart = static_cast<size_t>(location);
  m_selectionEnd = static_cast<size_t>(location + length);
}

void TextInputComponentInstance::onTextSelectionChange(
    ArkUINode* node,
    int32_t location,
    int32_t length) {
  if (m_textWasPastedOrCut) {
    m_textWasPastedOrCut = false;
  }
  // If the cursor controlled by onChange is not updated here,
  if (m_valueChanged) {
    m_valueChanged = false;
    return;
  }
  int32_t contentLength = countUtf16Characters(m_content);
  m_endOffset = std::max<int32_t>(contentLength - location - length, 0);
  setSelection(location, length);
  if (m_eventEmitter && m_focused) {
    m_eventEmitter->onSelectionChange(getTextInputMetrics());
  }
}

void TextInputComponentInstance::onContentSizeChange(
    ArkUINode* node,
    float width,
    float height) {
  bool calledByTextArea = (dynamic_cast<TextAreaNode*>(node) != nullptr);
  if (calledByTextArea == m_multiline) {
    m_contentSizeWidth = width;
    m_contentSizeHeight = height;
    if (m_eventEmitter) {
      m_eventEmitter->onContentSizeChange(getTextInputMetrics());
    }
  }
}

void TextInputComponentInstance::onContentScroll(ArkUINode* node) {
  if (m_eventEmitter) {
    m_eventEmitter->onScroll(getTextInputMetrics());
  }
}

std::string TextInputComponentInstance::getTextContentFromState(
    SharedConcreteState const& state) {
  std::ostringstream contentStream;
  for (auto const& fragment :
       state->getData().attributedStringBox.getValue().getFragments()) {
    contentStream << fragment.string;
  }
  return contentStream.str();
}

facebook::react::TextInputEventEmitter::Metrics
TextInputComponentInstance::getTextInputMetrics() {
  auto textInputMetrics = facebook::react::TextInputEventEmitter::Metrics();
  auto contentOffset = m_multiline
      ? m_textAreaNode.getTextContentRect().origin
      : m_textInputNode.getTextContentRect().origin;
  float pointScaleFactor = m_layoutMetrics.pointScaleFactor;
  auto padding = m_layoutMetrics.contentInsets - m_layoutMetrics.borderWidth;
  contentOffset.x = contentOffset.x / pointScaleFactor - padding.left;
  contentOffset.y = contentOffset.y / pointScaleFactor - padding.top;
  contentOffset.x = std::max<float>(-contentOffset.x, 0.0f);
  contentOffset.y = std::max<float>(-contentOffset.y, 0.0f);
  textInputMetrics.contentOffset = contentOffset;
  textInputMetrics.containerSize = m_layoutMetrics.frame.size;

  textInputMetrics.eventCount = this->m_nativeEventCount;
  textInputMetrics.selectionRange.location = this->m_selectionLocation;
  textInputMetrics.selectionRange.length = this->m_selectionLength;
  textInputMetrics.contentSize.width = this->m_contentSizeWidth;
  textInputMetrics.contentSize.height = this->m_contentSizeHeight;
  textInputMetrics.zoomScale = 1;
  textInputMetrics.text = m_multiline ? m_textAreaNode.getTextContent()
                                      : m_textInputNode.getTextContent();
  return textInputMetrics;
}

facebook::react::Size
TextInputComponentInstance::getOnContentSizeChangeMetrics() {
  auto OnContentSizeChangeMetrics = facebook::react::Size();
  OnContentSizeChangeMetrics.width = this->m_contentSizeWidth;
  OnContentSizeChangeMetrics.height = this->m_contentSizeHeight;
  return OnContentSizeChangeMetrics;
}

void TextInputComponentInstance::onPropsChanged(
    SharedConcreteProps const& props) {
  m_multiline = props->traits.multiline;
  if (m_multiline) {
    m_textInputNode.setTextInputNodeDelegate(nullptr);
    m_textAreaNode.setTextAreaNodeDelegate(this);
  } else {
    m_textInputNode.setTextInputNodeDelegate(this);
    m_textAreaNode.setTextAreaNodeDelegate(nullptr);
  }
  CppComponentInstance::onPropsChanged(props);
  m_clearTextOnFocus = props->traits.clearTextOnFocus;

  if (props->traits.contextMenuHidden != m_props->traits.contextMenuHidden) {
    m_textInputNode.setContextMenuHidden(props->traits.contextMenuHidden);
    m_textAreaNode.setContextMenuHidden(props->traits.contextMenuHidden);
  }
  if (props->traits.passwordRules != m_props->traits.passwordRules) {
    m_textInputNode.setPasswordRules(props->traits.passwordRules);
  }
  if (*(props->textAttributes.foregroundColor) !=
      *(m_props->textAttributes.foregroundColor)) {
    if (props->textAttributes.foregroundColor) {
      m_textAreaNode.setFontColor(props->textAttributes.foregroundColor);
      m_textInputNode.setFontColor(props->textAttributes.foregroundColor);
    } else {
      m_textAreaNode.setFontColor(facebook::react::blackColor());
      m_textInputNode.setFontColor(facebook::react::blackColor());
    }
  }
  if (props->textAttributes != m_props->textAttributes) {
    auto fontSizeScale =
        this->m_deps->displayMetricsManager->getDisplayMetrics()
            .windowPhysicalPixels.fontScale;
    m_textAreaNode.setFont(props->getEffectiveTextAttributes(fontSizeScale));
    m_textInputNode.setFont(props->getEffectiveTextAttributes(fontSizeScale));
  }
  if (!m_props ||
      props->textAttributes.lineHeight != m_props->textAttributes.lineHeight) {
    if (props->textAttributes.lineHeight) {
      auto fontSizeScale =
          this->m_deps->displayMetricsManager->getDisplayMetrics()
              .windowPhysicalPixels.fontScale;
      m_textAreaNode.setTextInputLineHeight(
          props->getEffectiveTextAttributes(fontSizeScale));
      m_textInputNode.setTextInputLineHeight(
          props->getEffectiveTextAttributes(fontSizeScale));
    }
  }
  if (*(props->backgroundColor) != *(m_props->backgroundColor)) {
    if (props->backgroundColor) {
      m_textAreaNode.setBackgroundColor(props->backgroundColor);
      m_textInputNode.setBackgroundColor(props->backgroundColor);
    } else {
      m_textAreaNode.setBackgroundColor(facebook::react::clearColor());
      m_textInputNode.setBackgroundColor(facebook::react::clearColor());
    }
  }
  if (props->textAttributes.alignment) {
    if (*(props->textAttributes.alignment) !=
        *(m_props->textAttributes.alignment)) {
      m_textAreaNode.setTextAlign(props->textAttributes.alignment);
      m_textInputNode.setTextAlign(props->textAttributes.alignment);
    }
  }
  if (props->paragraphAttributes.textAlignVertical !=
      m_props->paragraphAttributes.textAlignVertical) {
    ArkUI_Alignment alignmentVertical =
        ARKUI_TEXT_ALIGNMENT_BY_RN_VERTICAL_TEXT_ALIGNMENT
            [props->paragraphAttributes.textAlignVertical.value()];
    m_textInputNode.setAlignment(alignmentVertical);
    m_textAreaNode.setAlignment(alignmentVertical);
  }
  if (*(props->cursorColor) != *(m_props->cursorColor)) {
    if (props->cursorColor) {
      m_textAreaNode.setCaretColor(props->cursorColor);
      m_textInputNode.setCaretColor(props->cursorColor);
    } else {
      m_textAreaNode.setCaretColor(facebook::react::blackColor());
      m_textInputNode.setCaretColor(facebook::react::blackColor());
    }
  }
  if (props->traits.keyboardType != m_props->traits.keyboardType) {
    if (m_multiline) {
      m_textAreaNode.setInputType(props->traits.keyboardType);
    } else {
      m_textInputNode.setInputType(
          props->traits.keyboardType, props->traits.secureTextEntry);
      ;
    }
  }
  if (!m_props || props->maxLength != m_props->maxLength) {
    if (!props->maxLength) {
      m_textAreaNode.resetMaxLength();
      m_textInputNode.resetMaxLength();
    } else {
      m_textAreaNode.setMaxLength(props->maxLength);
      m_textInputNode.setMaxLength(props->maxLength);
    }
  }
  if (props->placeholder != m_props->placeholder) {
    m_textAreaNode.setPlaceholder(props->placeholder);
    m_textInputNode.setPlaceholder(props->placeholder);
  }
  if (props->placeholderTextColor) {
    if (*(props->placeholderTextColor) != *(m_props->placeholderTextColor)) {
      m_textAreaNode.setPlaceholderColor(props->placeholderTextColor);
      m_textInputNode.setPlaceholderColor(props->placeholderTextColor);
    }
  }
  if (props->rawProps.count("focusable") > 0) {
    if (m_props->rawProps.count("focusable") == 0 ||
        props->rawProps["focusable"].asBool() !=
            m_props->rawProps["focusable"].asBool()) {
      m_textAreaNode.setFocusable(props->rawProps["focusable"].asBool());
      m_textInputNode.setFocusable(props->rawProps["focusable"].asBool());
    }
  }
  m_textAreaNode.setId(getIdFromProps(props));
  m_textInputNode.setId(getIdFromProps(props));

  if (props->autoFocus != m_props->autoFocus) {
    if (m_multiline) {
      m_textAreaNode.setAutoFocus(props->autoFocus);
    } else {
      m_textInputNode.setAutoFocus(props->autoFocus);
    }
  }

  if (*(props->selectionColor) != *(m_props->selectionColor)) {
    if (props->selectionColor) {
      m_textInputNode.setSelectedBackgroundColor(props->selectionColor);
      if (!props->cursorColor) {
        m_textInputNode.setCaretColor(props->selectionColor);
        m_textAreaNode.setCaretColor(props->selectionColor);
      }
    } else {
      m_textInputNode.resetSelectedBackgroundColor();
    }
  }
  if (props->traits.secureTextEntry != m_props->traits.secureTextEntry ||
      props->traits.keyboardType != m_props->traits.keyboardType) {
    m_textInputNode.setInputType(
        props->traits.keyboardType, props->traits.secureTextEntry);
  }
  if (props->traits.caretHidden != m_props->traits.caretHidden) {
    m_textInputNode.setCaretHidden(props->traits.caretHidden);
  }
  if (!m_props ||
      props->traits.returnKeyType != m_props->traits.returnKeyType ||
      props->traits.returnKeyLabel != m_props->traits.returnKeyLabel) {
    m_textInputNode.setEnterKeyType(
        props->traits.returnKeyType, props->traits.returnKeyLabel);
    m_textAreaNode.setEnterKeyType(
        props->traits.returnKeyType, props->traits.returnKeyLabel);
  }
  if (props->traits.clearButtonMode != m_props->traits.clearButtonMode) {
    if (m_focused) {
      if (props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
        m_textInputNode.setCancelButtonMode(props->traits.clearButtonMode);
      } else if (
          props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
        m_textInputNode.setCancelButtonMode(
            facebook::react::TextInputAccessoryVisibilityMode::Never);
      }
    } else {
      if (props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::WhileEditing) {
        m_textInputNode.setCancelButtonMode(
            facebook::react::TextInputAccessoryVisibilityMode::Never);
      } else if (
          props->traits.clearButtonMode ==
          facebook::react::TextInputAccessoryVisibilityMode::UnlessEditing) {
        m_textInputNode.setCancelButtonMode(
            facebook::react::TextInputAccessoryVisibilityMode::Always);
      }
    }

    if (props->traits.clearButtonMode ==
            facebook::react::TextInputAccessoryVisibilityMode::Always ||
        props->traits.clearButtonMode ==
            facebook::react::TextInputAccessoryVisibilityMode::Never) {
      m_textInputNode.setCancelButtonMode(props->traits.clearButtonMode);
    }
  }

  if (!(props->yogaStyle == m_props->yogaStyle)) {
    m_textInputNode.setPadding(resolveEdges(
        [&props](auto edge) { return props->yogaStyle.padding(edge); }));
    m_textAreaNode.setPadding(resolveEdges(
        [&props](auto edge) { return props->yogaStyle.padding(edge); }));
  }

  if (!m_props ||
      props->traits.showSoftInputOnFocus !=
          m_props->traits.showSoftInputOnFocus) {
    m_textAreaNode.setShowKeyboardOnFocus(props->traits.showSoftInputOnFocus);
    m_textInputNode.setShowKeyboardOnFocus(props->traits.showSoftInputOnFocus);
  }
  if (!m_props ||
      props->importantForAutofill != m_props->importantForAutofill) {
    m_textAreaNode.setAutoFill(props->importantForAutofill);
    m_textInputNode.setAutoFill(props->importantForAutofill);
  }
  if (!m_props ||
      *(props->underlineColorAndroid) != *(m_props->underlineColorAndroid)) {
    m_textInputNode.setUnderlineColor(props->underlineColorAndroid);
    m_textAreaNode.setUnderlineColor(props->underlineColorAndroid);
  }

  if (props->traits.textContentType != m_props->traits.textContentType) {
    m_textInputNode.setTextContentType(props->traits.textContentType);
    m_textAreaNode.setTextContentType(props->traits.textContentType);
  }

  if (!m_props ||
      props->traits.submitBehavior != m_props->traits.submitBehavior) {
    m_textInputNode.setBlurOnSubmit(
        props->traits.submitBehavior ==
        facebook::react::SubmitBehavior::BlurAndSubmit);
    m_textAreaNode.setBlurOnSubmit(
        props->traits.submitBehavior ==
        facebook::react::SubmitBehavior::BlurAndSubmit);
  }

  if (!m_props || props->blurOnSubmit != m_props->blurOnSubmit) {
    m_textInputNode.setBlurOnSubmit(props->blurOnSubmit);
    m_textAreaNode.setBlurOnSubmit(props->blurOnSubmit);
  }

  if (!m_props || props->traits.editable != m_props->traits.editable) {
    m_textAreaNode.setEnabled(props->traits.editable);
    m_textInputNode.setEnabled(props->traits.editable);
  }
  if (!m_props ||
      props->traits.selectTextOnFocus != m_props->traits.selectTextOnFocus) {
    m_textInputNode.setSelectAll(props->traits.selectTextOnFocus);
    m_textAreaNode.setSelectAll(props->traits.selectTextOnFocus);
  }
}

void TextInputComponentInstance::onLayoutChanged(
    facebook::react::LayoutMetrics const& layoutMetrics) {
  CppComponentInstance::onLayoutChanged(layoutMetrics);
  m_textAreaNode.setLayoutRect(
      layoutMetrics.frame.origin,
      layoutMetrics.frame.size,
      layoutMetrics.pointScaleFactor);
  m_textInputNode.setLayoutRect(
      layoutMetrics.frame.origin,
      layoutMetrics.frame.size,
      layoutMetrics.pointScaleFactor);
}

int32_t TextInputComponentInstance::countUtf16Characters(
    std::string const& content) {
  int32_t len = 0;
  const unsigned char* currentByte =
      reinterpret_cast<const unsigned char*>(content.data());
  const unsigned char* endOfBytes = currentByte + content.size();
  // Judge the length of UTF-8 characters based on the number of bits higher
  // than the first byte, take out the significant bits, and record the number
  // of continuation bytes that need to be read
  while (currentByte < endOfBytes) {
    uint32_t codePoint = 0;
    int continuationBytes = 0;
    if (*currentByte < 0x80) {
      codePoint = *currentByte++;
    } else if ((*currentByte >> 5) == 0x6) {
      codePoint = *currentByte & 0x1F;
      continuationBytes = 1;
      ++currentByte;
    } else if ((*currentByte >> 4) == 0xE) {
      codePoint = *currentByte & 0x0F;
      continuationBytes = 2;
      ++currentByte;
    } else if ((*currentByte >> 3) == 0x1E) {
      codePoint = *currentByte & 0x07;
      continuationBytes = 3;
      ++currentByte;
    } else {
      ++currentByte;
      continue;
    }
    // The remaining bytes are taken and the length is calculated
    while (continuationBytes-- && currentByte < endOfBytes &&
           ((*currentByte & 0xC0) == 0x80)) {
      codePoint = (codePoint << 6) | (*currentByte++ & 0x3F);
    }
    // The number of code elements is calculated according to the hexadecimal
    // format
    if (codePoint < 0x10000) {
      len += 1;
    } else {
      len += 2;
    }
  }
  return len;
}
void TextInputComponentInstance::setTextContent(std::string const& content) {
  m_content = content;
  m_textInputNode.setTextContent(content);
  m_textAreaNode.setTextContent(content);
}

void TextInputComponentInstance::setTextSelection(
    int32_t selectionStart,
    int32_t selectionEnd) {
  if (selectionStart < 0 || selectionEnd < 0) {
    int32_t contentLength = countUtf16Characters(m_content);
    int32_t location = contentLength - m_endOffset;
    setSelection(location, 0);
    m_textInputNode.setTextSelection(location, location);
    m_textAreaNode.setTextSelection(location, location);
    return;
  }
  if (selectionStart > selectionEnd) {
    std::swap(selectionStart, selectionEnd);
  }
  if (m_selectionStart.has_value() && m_selectionEnd.has_value() &&
      m_selectionStart.value() == selectionStart &&
      m_selectionEnd.value() == selectionEnd) {
    return;
  }
  int32_t length = selectionEnd - selectionStart;
  setSelection(selectionStart, length);
  m_textInputNode.setTextSelection(selectionStart, selectionEnd);
  m_textAreaNode.setTextSelection(selectionStart, selectionEnd);
}

void TextInputComponentInstance::onKeyPressChange(
    int32_t location,
    std::string text) {
  int32_t newCursorPosition = location;
  int32_t previousCursorPosition = m_selectionLocation;
  std::u16string newCharUtf16;
  bool noPreviousSelection = previousCursorPosition == 0;
  bool cursorDidNotMove = newCursorPosition == previousCursorPosition;
  bool cursorMovedBackwardsOrAtBeginningOfInput =
      (newCursorPosition < previousCursorPosition) || newCursorPosition <= 0;
  if (!cursorMovedBackwardsOrAtBeginningOfInput &&
      (noPreviousSelection || !cursorDidNotMove)) {
    auto utfContent = boost::locale::conv::utf_to_utf<char16_t>(text);
    if (newCursorPosition > 0 && newCursorPosition <= utfContent.size()) {
      int onlyNewContentLength =
          std::max(newCursorPosition - previousCursorPosition, 1);
      onlyNewContentLength = std::min(onlyNewContentLength, newCursorPosition);
      newCharUtf16 = utfContent.substr(
          newCursorPosition - onlyNewContentLength, onlyNewContentLength);
    }
  }
  auto keyPressMetrics =
      facebook::react::TextInputEventEmitter::KeyPressMetrics();
  keyPressMetrics.text = boost::locale::conv::utf_to_utf<char>(newCharUtf16);
  keyPressMetrics.eventCount = m_nativeEventCount;
  if (m_eventEmitter) {
    m_eventEmitter->onKeyPress(keyPressMetrics);
  }
}

void TextInputComponentInstance::emitCommittedKeyPress(
    const std::string& previousContent,
    const std::string& text,
    int32_t selStart,
    int32_t selLen) {
  auto prevUtf16 = boost::locale::conv::utf_to_utf<char16_t>(previousContent);
  auto textUtf16 = boost::locale::conv::utf_to_utf<char16_t>(text);

  size_t prevLen = prevUtf16.size();
  size_t newLen = textUtf16.size();

  // Characters after the selection in the original text (tail that should be
  // preserved)
  size_t tailLen = 0;
  if (static_cast<size_t>(selStart + selLen) <= prevLen) {
    tailLen = prevLen - selStart - selLen;
  }
  // The committed content sits between selStart and (newLen - tailLen)
  size_t commitEnd = newLen >= tailLen ? newLen - tailLen : newLen;
  size_t commitStart = static_cast<size_t>(selStart);
  if (commitStart > commitEnd) {
    commitStart = commitEnd;
  }

  std::u16string newCharUtf16 =
      textUtf16.substr(commitStart, commitEnd - commitStart);

  auto keyPressMetrics =
      facebook::react::TextInputEventEmitter::KeyPressMetrics();
  keyPressMetrics.text = boost::locale::conv::utf_to_utf<char>(newCharUtf16);
  keyPressMetrics.eventCount = m_nativeEventCount;
  if (m_eventEmitter) {
    m_eventEmitter->onKeyPress(keyPressMetrics);
  }
}

void TextInputComponentInstance::onCommandReceived(
    std::string const& commandName,
    folly::dynamic const& args) {
  if (commandName == "focus") {
    // When showSoftInputOnFocus is set to false, the keyboard opened by the
    // previous TextInput will be dismissed.
    if (m_props->traits.showSoftInputOnFocus == false) {
      blur();
    }
    focus();
    if (m_selectionStart.has_value() && m_selectionEnd.has_value() &&
        !m_props->traits.selectTextOnFocus) {
      // Cursor must be forced to set in the focused scenario
      int32_t selectionStart = m_selectionStart.value();
      m_selectionStart = static_cast<size_t>(-1);
      setTextSelection(selectionStart, m_selectionEnd.value());
    }
  } else if (commandName == "blur") {
    blur();
  } else if (
      commandName == "setTextAndSelection" && args.isArray() &&
      args.size() == 4 && args[0].asInt() >= m_nativeEventCount) {
    auto textContent = args[1].asString();
    auto selectionStart = args[2].asInt();
    auto selectionEnd = args[3].asInt();
    // When adding a timer refresh parameter on the JavaScript side, returning
    // the last parameter causes an exception, and the current pre-screening
    // needs to be skipped.
    if (m_extendStr.empty()) {
      setTextContent(textContent);
    }
    m_extendStr = "";
    setTextSelection(selectionStart, selectionEnd);
  }
}

void TextInputComponentInstance::onStateChanged(
    SharedConcreteState const& state) {
  CppComponentInstance::onStateChanged(state);

  if (state->getData().mostRecentEventCount < this->m_nativeEventCount) {
    return;
  }

  m_extendStr = "";
  auto content = getTextContentFromState(state);
  if (content != m_content) {
    setTextContent(content);
    int32_t contentLength = countUtf16Characters(content);
    int32_t location = contentLength - m_endOffset;
    setTextSelection(location, location);
  }
}

ArkUINode& TextInputComponentInstance::getLocalRootArkUINode() {
  if (m_multiline) {
    return m_textAreaNode;
  }
  return m_textInputNode;
}

void TextInputComponentInstance::focus() {
  getLocalRootArkUINode().setFocusStatus(1);
}

void TextInputComponentInstance::blur() {
  getLocalRootArkUINode().setFocusStatus(0);
}

void TextInputComponentInstance::onMessageReceived(
    const ArkTSMessage& message) {
  if (message.name == "KEYBOARD_VISIBLE" && this->m_focused) {
    auto parent = this->m_parent.lock();
    std::shared_ptr<ScrollViewComponentInstance> scrollView = nullptr;
    while (parent != nullptr) {
      scrollView =
          std::dynamic_pointer_cast<ScrollViewComponentInstance>(parent);
      if (scrollView != nullptr) {
        scrollView->setKeyboardAvoider(shared_from_this());
        break;
      }
      parent = parent->getParent().lock();
    }
  }
}

// KeyboardAvoider
facebook::react::Float
TextInputComponentInstance::getBottomEdgeOffsetRelativeToScrollView(
    std::shared_ptr<ScrollViewComponentInstance> scrollView) {
  auto relativePos = m_layoutMetrics.frame.origin;
  auto parent = m_parent.lock();
  while (parent != nullptr && parent->getTag() != scrollView->getTag()) {
    relativePos += parent->getLayoutMetrics().frame.origin;
    parent = parent->getParent().lock();
  }
  /**
   * It looks like 24 is used by the platform when KeyboardAvoider hack isn't
   * needed.
   */
  auto GAP_BETWEEN_KEYBOARD_AND_TEXT_INPUT = 24;
  return relativePos.y + m_layoutMetrics.frame.size.height +
      GAP_BETWEEN_KEYBOARD_AND_TEXT_INPUT;
}

} // namespace rnoh
