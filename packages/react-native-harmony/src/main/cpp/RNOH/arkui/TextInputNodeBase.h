/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "ArkUINode.h"
#include "react/renderer/components/textinput/TextInputProps.h"
#include "react/renderer/graphics/Color.h"

/**
 * TextInput_TextContentType is defined to replace
 * ArkUI_TextInputContentType. Directly using enum from IDE SDK may raise
 * compiling error when developer use lower IDE SDK version that does not
 * contain new supported type.
 * @internal
 * @deprecated: This enum is no longer part of the native API. Deprecated when
 * preparing the 0.77 branch for release.
 */
typedef enum {
  USER_NAME,
  PASSWORD,
  NEW_PASSWORD,
  FULL_STREET_ADDRESS,
  HOUSE_NUMBER,
  DISTRICT_ADDRESS,
  CITY_ADDRESS,
  PROVINCE_ADDRESS,
  COUNTRY_ADDRESS,
  PERSON_FULL_NAME,
  PERSON_LAST_NAME,
  PERSON_FIRST_NAME,
  PHONE_NUMBER,
  PHONE_COUNTRY_CODE,
  FULL_PHONE_NUMBER,
  EMAIL_ADDRESS,
  BANK_CARD_NUMBER,
  ID_CARD_NUMBER,
  NICKNAME,
  DETAIL_INFO_WITHOUT_STREET,
  FORMAT_ADDRESS,
  PASSPORT_NUMBER,
  VALIDITY,
  ISSUE_AT,
  ORGANIZATION,
  TAX_ID,
  ADDRESS_CITY_AND_STATE,
  FLIGHT_NUMBER,
  LICENSE_NUMBER,
  LICENSE_FILE_NUMBER,
  LICENSE_PLATE,
  ENGINE_NUMBER,
  LICENSE_CHASSIS_NUMBER,

} TextInput_TextContentType;

namespace rnoh {

std::size_t charToByteIndex(const std::string& content, std::size_t position);

/**
 * @actor RNOH_LIBRARY
 */
class TextInputNodeBase : public ArkUINode {
 protected:
  TextInputNodeBase(
      const ArkUINode::Context::Shared& context,
      ArkUI_NodeType nodeType);
  void setCommonFontAttributes(
      facebook::react::TextAttributes const& textAttributes);

 public:
  /**
   * @brief Set padding for TextInput.
   * @param padding The value of padding.
   */
  void setPadding(
      facebook::react::RectangleEdges<facebook::react::Float> padding);

  /**
   * @brief Set the focusability of the text input.
   * @param focusable The focusability.
   */
  void setFocusable(bool const& focusable);

  /**
   * @brief Get the focus status of TextInput.
   * @return Focus status: true-focused, false-unfocused.
   * @Deprecated: use isFocused.
   * Deprecated since the name is not matching behavior.
   * Status should normally be enum instead of bool.
   */
  inline bool getTextFocusStatus() {
    return this->isFocused();
  };

  /**
   * @brief Set the selection to all text in the TextInput.
   * @param selectAll SelectAll flag.
   */
  void setSelectAll(bool selectAll);

  /**
   * @brief Set the touchable area.
   * @param position Touch area position.
   * @param size Touch area size.
   */
  void setResponseRegion(
      facebook::react::Point const& position,
      facebook::react::Size const& size);

  /**
   * @brief Set the content for text input.
   * @param textContent Text content.
   */
  void setTextContent(std::string const& textContent);

  /**
   * @brief Set the start and end of the text input's selection.
   * @param start The start of the text input's selection.
   * @param end The end of the text input's selection.
   */
  void setTextSelection(int32_t start, int32_t end);

  /**
   * @brief Set the font color.
   * @param color The font color.
   */
  void setFontColor(facebook::react::SharedColor const& color);

  /**
   * @brief Set the font style for the text input.
   * @param textAttributes TextAttributes object.
   */
  virtual void setFont(
      facebook::react::TextAttributes const& textAttributes) = 0;

  /**
   * @brief Set the caret color.
   * @param color The caret color.
   */
  virtual void setCaretColor(facebook::react::SharedColor const& color) = 0;

  /**
   * @brief Limits the maximum number of characters that can be entered.
   * @param  maxLength The max number of characters.
   */
  virtual void setMaxLength(int32_t maxLength) = 0;

  /**
   * @brief Set the string that will be rendered before text input has
   * been entered.
   * @param placeholder The placeholder string.
   */
  virtual void setPlaceholder(std::string const& placeholder) = 0;

  /**
   * @brief Set the text color of the placeholder string.
   * @param color The text color of the placeholder string.
   */
  virtual void setPlaceholderColor(
      facebook::react::SharedColor const& color) = 0;

  void setTextInputLineHeight(
      facebook::react::TextAttributes const& textAttributes);

  /**
   * @brief Set the text lineHeight for the text input.
   * @param textAlign TextAlignment object.
   */
  void setTextAlign(
      std::optional<facebook::react::TextAlignment> const& textAlign);

  /**
   * @brief Set the text alignment of the text input.
   * @param textAlignment
   */
  void setAlignment(ArkUI_Alignment const& textAlignment);

  /**
   * @brief Reset the maximum length of the text input.
   */
  void resetMaxLength();

  /**
   * @brief Get the content in the text input.
   * @return the content in the text input.
   */
  std::string virtual getTextContent() = 0;

  /**
   * @brief Get the position of the edited text input relative to the
   * component and its size.
   * @return The position of the edited text input relative to the
   * component and its size.
   */
  virtual facebook::react::Rect getTextContentRect() const = 0;
  /**
   * @internal
   */
  static uint32_t convertColorToTranslucentUnderline(
      facebook::react::SharedColor const& color);
  /**
   * @internal
   */
  static uint32_t convertColorToTranslucentSelection(
      facebook::react::SharedColor const& color);
  /**
   * @internal
   */
  static ArkUI_NumberValue convertContentType(
      std::string const& textContentType);
  /**
   * @internal
   */
  static ArkUI_EnterKeyType convertEnterKeyType(
      facebook::react::ReturnKeyType returnKeyType);
  /**
   * @internal
   */
  static ArkUI_EnterKeyType convertEnterKeyLabel(std::string returnKeyLabel);
  /**
   * @internal
   */
  static ArkUI_TextInputType convertInputType(
      facebook::react::KeyboardType keyboardType);
};

} // namespace rnoh