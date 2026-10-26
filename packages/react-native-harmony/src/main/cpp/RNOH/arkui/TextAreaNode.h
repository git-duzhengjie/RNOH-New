/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "ArkUINode.h"
#include "TextInputNodeBase.h"
#include "react/renderer/components/textinput/TextInputEventEmitter.h"
#include "react/renderer/components/textinput/TextInputProps.h"
#include "react/renderer/graphics/Color.h"

namespace rnoh {
/**
 * @actor RNOH_LIBRARY
 */
class TextAreaNodeDelegate {
 public:
  /**
   * @brief Destructor
   */
  virtual ~TextAreaNodeDelegate() = default;

  /**
   * @brief Callback that is called when the text area's text changes. You only
   * need to overwrite one of these onChange methods, it is recommended to use
   * onChange(std::string text, std::string extendStr), which supports the input
   * method pinyin preview screen function.
   * @param node The text area's node.
   * @param text The text area's text.
   */
  virtual void onChange(ArkUINode* node, const std::string& text) {}
  /**
   * @brief Callback that is called when the text area's text changes.
   * @param node The text area's node.
   * @param text The text area's text.
   * @param extendStr The preview text area's text.
   */
  virtual void onChange(
      ArkUINode* node,
      const std::string& text,
      std::string extendStr = "") {}

  /**
   * @brief Callback that is called when the text area is blurred.
   * @param node The text area's node.
   */
  virtual void onBlur(ArkUINode* node) {}

  /**
   * @brief Callback that is called when the text area is focused.
   * @param node The text area's node.
   */
  virtual void onFocus(ArkUINode* node) {}

  /**
   * @brief Callback that is called when the text area's submit
   * button is pressed.
   * @param node The text area's node.
   */
  virtual void onSubmit(ArkUINode* node) {}

  /**
   * @brief Callback that is called when paste or cut the text.
   * @param node The text area's node.
   */
  virtual void onPasteOrCut(ArkUINode* node) {}

  /**
   * @brief Callback that is called when the text selection changes.
   * @param node The text area's node.
   * @param location The text area's caret location.
   * @param length The length of the selected text.
   */
  virtual void
  onTextSelectionChange(ArkUINode* node, int32_t location, int32_t length) {}

  /**
   * @brief Callback that is called when the text area's content scrolls.
   * @param node The text area's node.
   */
  virtual void onContentScroll(ArkUINode* node) {}

  /**
   * @brief Callback that is called when the text area's content size changes.
   * @param node The text area's node.
   * @param width The Text area content width.
   * @param height The Text area content height.
   */
  virtual void onContentSizeChange(ArkUINode* node, float width, float height) {
  }
  /**
   * @brief Callback that is called when the NODE_TEXT_AREA_ON_WILL_DELETE event
   * is fired, eg. when text is about to be deleted.
   * @param node The text area's node.
   * @param position Index of the text to be deleted.
   * @param direction The direction for deleting the text, with 0 indicating
   * backward-delete and 1 indicating forward-delete.
   */
  virtual void onWillDelete(ArkUINode* node, int position, int direction) {}

  /**
   * @Deprecated: use onChange(ArkUINode*, const std::string&).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onChange(std::string text) {}
  virtual void onChange(std::string text, std::string extendStr = "") {}
  /**
   * @Deprecated: use onBlur(ArkUINode*).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onBlur() {}
  /**
   * @Deprecated: use onFocus(ArkUINode*).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onFocus() {}
  /**
   * @Deprecated: use onSubmit(ArkUINode*).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onSubmit() {}
  /**
   * @Deprecated: use onPasteOrCut(ArkUINode*).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onPasteOrCut() {}
  /**
   * @Deprecated: use onTextSelectionChange(ArkUINode*, int32_t,
   * int32_t). Deprecated when preparing 0.77 for release.
   */
  virtual void onTextSelectionChange(int32_t location, int32_t length) {}
  /**
   * @Deprecated: use onContentScroll(ArkUINode*).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onContentScroll() {}
  /**
   * @Deprecated: use onContentSizeChange(ArkUINode*, float, float).
   * Deprecated when preparing 0.77 for release.
   */
  virtual void onContentSizeChange(float width, float height, bool multiline) {}
};

/**
 * @actor RNOH_LIBRARY
 */
class TextAreaNode : public TextInputNodeBase {
 private:
  bool m_hasRNSetTextContext{false};
  std::string m_textContent;
  bool m_autoFocus{false};
  void onChange(const std::string& text, const std::string& extendStr = "");

 protected:
  TextAreaNodeDelegate* m_textAreaNodeDelegate;

 public:
  explicit TextAreaNode(const ArkUINode::Context::Shared& context = nullptr);
  ~TextAreaNode() override;

  /**
   * @brief Obtains the offset of TextArea.
   * @return The offset of TextArea.
   */
  facebook::react::Point getTextAreaOffset() const;

  /**
   * @brief Obtains the position of the edited text area relative to the
   * component and its size.
   * @return The position of the edited text area relative to the component
   * and its size.
   */
  facebook::react::Rect getTextContentRect() const override;

  /**
   * @brief The callback function to receive component events.
   * @param eventType The event type of ArkUI.
   * @param eventArgs The parameter of the component callback event.
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, EventArgs& eventArgs)
      override;

  /**
   * @brief The callback function to receive component events.
   * @param eventType The event type of ArkUI.
   * @param eventString The string data in a component event.
   */
  void onNodeEvent(ArkUI_NodeEventType eventType, std::string_view eventString)
      override;

  void onNodeEvent(ArkUI_NodeEventType eventType, ArkUI_NodeEvent* event)
      override;

  /**
   * @brief Assigns a delegate to handle TextArea node events.
   * @param textAreaNodeDelegate The delegate object (must implement
   * TextAreaNodeDelegate).
   */
  void setTextAreaNodeDelegate(TextAreaNodeDelegate* textAreaNodeDelegate);

  /**
   * @brief Set the text content of TextArea.
   * @param textContent Text content.
   */
  void setTextContent(std::string const& textContent);

  /**
   * @brief Set the text area type.
   * @param keyboardType Determines which keyboard to open.
   */
  void setInputType(facebook::react::KeyboardType keyboardType);

  /**
   * @brief Set the text area type.
   * @param keyboardType Determines which keyboard to open.
   */
  void setInputType(ArkUI_TextAreaType keyboardType);

  /**
   * @brief Set the font style for the TextArea.
   * @param textAttributes TextAttributes object.
   */
  void setFont(facebook::react::TextAttributes const& textAttributes) override;

  /**
   * @brief Set the caret color.
   * @param color The caret color.
   */
  void setCaretColor(facebook::react::SharedColor const& color) override;

  /**
   * @brief Set the maximum number of characters in the text area.
   * @param maxLength The maximum number of characters in the text area.
   */
  void setMaxLength(int32_t maxLength) override;

  /**
   * @brief Set the placeholder text of the TextArea.
   * @param placeholder The placeholder text.
   */
  void setPlaceholder(std::string const& placeholder) override;

  /**
   * @brief Set the placeholder text color of the TextArea.
   * @param color The placeholder text color.
   */
  void setPlaceholderColor(facebook::react::SharedColor const& color) override;

  /**
   * @brief Set default Padding.
   */
  void defaultSetPadding();

  /**
   * @brief Sets the autofill type.
   * @param textContentType
   */
  void setTextContentType(std::string const& textContentType);

  /**
   * @brief Sets whether to hide the text selection menu when the text box is
   * long-pressed, double-click, or right-clicked.
   * @param hidden If true, context menu is hidden.
   */
  void setContextMenuHidden(bool hidden);

  /**
   * @brief Sets whether to hide the text selection menu when the text box is
   * @Deprecated: use setContextMenuHidden.
   * Deprecated when preparing 0.77 for release.
   */
  inline void SetContextMenuHidden(bool hidden) {
    this->setContextMenuHidden(hidden);
  }

  /**
   * @brief Sets whether the TextArea loses focus after the Enter key is
   * pressed to submit information.
   * @param blurOnSubmit If true, the TextArea will loss focus.
   */
  void setBlurOnSubmit(bool blurOnSubmit);

  /**
   * @brief Set the text line height attribute.
   * @param lineHeight
   */
  void setLineHeight(float lineHeight);

  /**
   * @brief Set the type of the Enter key.
   * @param returnKeyType Determines how the return key should look.
   * @param returnKeyLabel Sets the return key to the label when `returnKeyType`
   * is set to `default`.
   */
  void setEnterKeyType(
      facebook::react::ReturnKeyType returnKeyType,
      std::string returnKeyLabel);

  /**
   * @brief Set the type of the Enter key.
   * @param returnKeyType Determines how the return key should look.
   */
  void setEnterKeyType(ArkUI_EnterKeyType returnKeyType);

  /**
   * @brief Sets the color of the text underline.
   * @param underlineColor
   */
  void setUnderlineColor(facebook::react::SharedColor const& underlineColor);

  /**
   * @brief Sets the color of the text underline.
   * @param underlineColorAndroid
   * @Deprecated: use setUnderlineColor.
   * Deprecated when preparing 0.77 for release.
   */
  inline void setUnderlineColorAndroid(
      facebook::react::SharedColor const& underlineColorAndroid) {
    this->setUnderlineColor(underlineColorAndroid);
  };

  /**
   * @brief Sets whether to enable autofill.
   * @param autoFill whether to enable autofill.
   */
  void setAutoFill(bool autoFill);

  /**
   * @brief Sets whether to enable autofill.
   * @param autoFill Tells the operating system whether the individual fields
   * in your app should be included in a view structure for autofill purposes.
   * Possible values are auto, no, noExcludeDescendants, yes, and
   * yesExcludeDescendants. The default value is auto.
   */
  void setAutoFill(std::string const& autoFill);

  /**
   * @brief Sets whether the keyboard pops up when the input box gains focus.
   * @param enable When false, it will prevent the soft keyboard from showing
   * when the field is focused.
   */
  void setShowKeyboardOnFocus(bool enable);

  /**
   * @brief Sets whether the keyboard pops up when the input box gains focus.
   * @Deprecated: use setShowKeyboardOnFocus
   * Deprecated when preparing 0.77 branch for release.
   */
  inline void setshowSoftInputOnFocus(bool enable) {
    this->setShowKeyboardOnFocus(enable);
  }

  /**
   * @brief Sets the regular expression for input filtering.
   * @param inputFilter The regular expression.
   */
  void setInputFilter(std::string const& inputFilter);

  /**
   * @brief Set whether the input box automatically gets focus.
   * @param autoFocus When autoFocus is true, the input box automatically
   * gets focus.
   */
  void setAutoFocus(bool autoFocus);

  /**
   * @brief Obtains text content of the TextArea.
   * @return Text content.
   */
  std::string getTextContent() override;

  /**
   * @brief Convert TextArea EnterKeyType
   * @param returnKeyType Enter key type enumeration
   * @return Type of enter key in ArkUI
   */
  static ArkUI_EnterKeyType convertTextAreaEnterKeyType(
      facebook::react::ReturnKeyType returnKeyType);

  /**
   * @brief Convert TextArea EnterKeyType
   * @param returnKeyType Enter key type string
   * @return Type of enter key in ArkUI
   */
  static ArkUI_EnterKeyType convertTextAreaEnterKeyLabel(
      std::string returnKeyLabel);

  /**
   * @brief Convert TextArea Keyboard type
   * @param keyboardType Keyboard type enumeration
   * @return TextArea keyboard type in ArkUI
   */
  static ArkUI_TextAreaType convertTextAreaInputType(
      facebook::react::KeyboardType keyboardType);
};

} // namespace rnoh