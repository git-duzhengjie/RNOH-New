/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TextInputNodeBase.h"
#include <cmath>
#include <arkui/native_node.h>
#include "NativeNodeApi.h"
#include "RNOH/arkui/conversions.h"

namespace rnoh {

std::size_t charToByteIndex(const std::string& content, std::size_t position) {
  std::size_t byteIndex = 0;
  std::size_t chars = 0;
  std::size_t size = content.size();
  const unsigned char* currentByte =
      reinterpret_cast<const unsigned char*>(content.data());
  const unsigned char* endOfBytes = currentByte + size;

  while (currentByte < endOfBytes && chars < position) {
    std::size_t charLen;
    if (*currentByte < 0x80) {
      charLen = 1;
    } else if ((*currentByte >> 5) == 0x06) {
      charLen = 2;
    } else if ((*currentByte >> 4) == 0x0E) {
      charLen = 3;
    } else if ((*currentByte >> 3) == 0x1E) {
      charLen = 4;
    } else {
      charLen = 1;
    }
    byteIndex += charLen;
    currentByte += charLen;
    ++chars;
  }

  return byteIndex;
}

TextInputNodeBase::TextInputNodeBase(
    const ArkUINode::Context::Shared& context,
    ArkUI_NodeType nodeType)
    : ArkUINode(context, nodeType) {}

ArkUI_TextInputType TextInputNodeBase::convertInputType(
    facebook::react::KeyboardType keyboardType) {
  switch (keyboardType) {
    case facebook::react::KeyboardType::Numeric:
    case facebook::react::KeyboardType::NumberPad:
      return ARKUI_TEXTINPUT_TYPE_NUMBER;
    case facebook::react::KeyboardType::DecimalPad:
      return ARKUI_TEXTINPUT_TYPE_NUMBER_DECIMAL;
    case facebook::react::KeyboardType::PhonePad:
      return ARKUI_TEXTINPUT_TYPE_PHONE_NUMBER;
    case facebook::react::KeyboardType::EmailAddress:
      return ARKUI_TEXTINPUT_TYPE_EMAIL;
    default:
      return ARKUI_TEXTINPUT_TYPE_NORMAL;
  }
}

ArkUI_EnterKeyType TextInputNodeBase::convertEnterKeyType(
    facebook::react::ReturnKeyType returnKeyType) {
  switch (returnKeyType) {
    case facebook::react::ReturnKeyType::Done:
      return ARKUI_ENTER_KEY_TYPE_DONE;
    case facebook::react::ReturnKeyType::Go:
      return ARKUI_ENTER_KEY_TYPE_GO;
    case facebook::react::ReturnKeyType::Next:
      return ARKUI_ENTER_KEY_TYPE_NEXT;
    case facebook::react::ReturnKeyType::Search:
      return ARKUI_ENTER_KEY_TYPE_SEARCH;
    case facebook::react::ReturnKeyType::Send:
      return ARKUI_ENTER_KEY_TYPE_SEND;
    case facebook::react::ReturnKeyType::Previous:
      return ARKUI_ENTER_KEY_TYPE_PREVIOUS;
    default:
      return ARKUI_ENTER_KEY_TYPE_DONE;
  }
}

ArkUI_EnterKeyType TextInputNodeBase::convertEnterKeyLabel(
    std::string returnKeyLabel) {
  if (!returnKeyLabel.compare("done")) {
    return ARKUI_ENTER_KEY_TYPE_DONE;
  } else if (!returnKeyLabel.compare("go")) {
    return ARKUI_ENTER_KEY_TYPE_GO;
  } else if (!returnKeyLabel.compare("next")) {
    return ARKUI_ENTER_KEY_TYPE_NEXT;
  } else if (!returnKeyLabel.compare("search")) {
    return ARKUI_ENTER_KEY_TYPE_SEARCH;
  } else if (!returnKeyLabel.compare("send")) {
    return ARKUI_ENTER_KEY_TYPE_SEND;
  } else if (!returnKeyLabel.compare("previous")) {
    return ARKUI_ENTER_KEY_TYPE_PREVIOUS;
  } else {
    return ARKUI_ENTER_KEY_TYPE_DONE;
  }
}

ArkUI_NumberValue TextInputNodeBase::convertContentType(
    std::string const& textContentType) {
  if (textContentType == "addressCity") {
    return {.i32 = TextInput_TextContentType::CITY_ADDRESS};
  } else if (textContentType == "addressState") {
    return {.i32 = TextInput_TextContentType::PROVINCE_ADDRESS};
  } else if (textContentType == "countryName") {
    return {.i32 = TextInput_TextContentType::COUNTRY_ADDRESS};
  } else if (textContentType == "creditCardNumber") {
    return {.i32 = TextInput_TextContentType::BANK_CARD_NUMBER};
  } else if (textContentType == "fullStreetAddress") {
    return {.i32 = TextInput_TextContentType::FULL_STREET_ADDRESS};
  } else if (textContentType == "sublocality") {
    return {.i32 = TextInput_TextContentType::DISTRICT_ADDRESS};
  } else if (textContentType == "telephoneNumber") {
    return {.i32 = TextInput_TextContentType::PHONE_NUMBER};
  } else if (textContentType == "username") {
    return {.i32 = TextInput_TextContentType::USER_NAME};
  } else if (textContentType == "password") {
    return {.i32 = TextInput_TextContentType::PASSWORD};
  } else if (textContentType == "newPassword") {
    return {.i32 = TextInput_TextContentType::NEW_PASSWORD};
  } else if (textContentType == "houseNumber") {
    return {.i32 = TextInput_TextContentType::HOUSE_NUMBER};
  } else if (textContentType == "districtAddress") {
    return {.i32 = TextInput_TextContentType::DISTRICT_ADDRESS};
  } else if (textContentType == "cityAddress") {
    return {.i32 = TextInput_TextContentType::CITY_ADDRESS};
  } else if (textContentType == "provinceAddress") {
    return {.i32 = TextInput_TextContentType::PROVINCE_ADDRESS};
  } else if (textContentType == "countryAddress") {
    return {.i32 = TextInput_TextContentType::COUNTRY_ADDRESS};
  } else if (textContentType == "personFullName") {
    return {.i32 = TextInput_TextContentType::PERSON_FULL_NAME};
  } else if (textContentType == "personLastName") {
    return {.i32 = TextInput_TextContentType::PERSON_LAST_NAME};
  } else if (textContentType == "personFirstName") {
    return {.i32 = TextInput_TextContentType::PERSON_FIRST_NAME};
  } else if (textContentType == "phoneNumber") {
    return {.i32 = TextInput_TextContentType::PHONE_NUMBER};
  } else if (textContentType == "phoneCountryCode") {
    return {.i32 = TextInput_TextContentType::PHONE_COUNTRY_CODE};
  } else if (textContentType == "fullPhoneNumber") {
    return {.i32 = TextInput_TextContentType::FULL_PHONE_NUMBER};
  } else if (textContentType == "emailAddress") {
    return {.i32 = TextInput_TextContentType::EMAIL_ADDRESS};
  } else if (textContentType == "bankCardNumber") {
    return {.i32 = TextInput_TextContentType::BANK_CARD_NUMBER};
  } else if (textContentType == "idCardNumber") {
    return {.i32 = TextInput_TextContentType::ID_CARD_NUMBER};
  } else if (textContentType == "nickName") {
    return {.i32 = TextInput_TextContentType::NICKNAME};
  } else if (textContentType == "name") {
    return {.i32 = TextInput_TextContentType::PERSON_FULL_NAME};
  } else if (textContentType == "familyName") {
    return {.i32 = TextInput_TextContentType::PERSON_LAST_NAME};
  } else if (textContentType == "givenName") {
    return {.i32 = TextInput_TextContentType::PERSON_FIRST_NAME};
  } else if (textContentType == "detailInfoWithoutStreet") {
    return {.i32 = TextInput_TextContentType::DETAIL_INFO_WITHOUT_STREET};
  } else if (textContentType == "formatAddress") {
    return {.i32 = TextInput_TextContentType::FORMAT_ADDRESS};
  } else if (textContentType == "passportNumber") {
    return {.i32 = TextInput_TextContentType::PASSPORT_NUMBER};
  } else if (textContentType == "validity") {
    return {.i32 = TextInput_TextContentType::VALIDITY};
  } else if (textContentType == "issueAt") {
    return {.i32 = TextInput_TextContentType::ISSUE_AT};
  } else if (textContentType == "organization") {
    return {.i32 = TextInput_TextContentType::ORGANIZATION};
  } else if (textContentType == "taxId") {
    return {.i32 = TextInput_TextContentType::TAX_ID};
  } else if (textContentType == "addressCityAndState") {
    return {.i32 = TextInput_TextContentType::ADDRESS_CITY_AND_STATE};
  } else if (textContentType == "flightNumber") {
    return {.i32 = TextInput_TextContentType::FLIGHT_NUMBER};
  } else if (textContentType == "licenseNumber") {
    return {.i32 = TextInput_TextContentType::LICENSE_NUMBER};
  } else if (textContentType == "licenseFileNumber") {
    return {.i32 = TextInput_TextContentType::LICENSE_FILE_NUMBER};
  } else if (textContentType == "engineNumber") {
    return {.i32 = TextInput_TextContentType::ENGINE_NUMBER};
  } else if (textContentType == "licenseChassisNumber") {
    return {.i32 = TextInput_TextContentType::LICENSE_CHASSIS_NUMBER};
  } else if (textContentType == "licensePlate") {
    return {.i32 = TextInput_TextContentType::LICENSE_PLATE};
  } else {
    return {.i32 = -1};
  }
}

uint32_t TextInputNodeBase::convertColorToTranslucentUnderline(
    facebook::react::SharedColor const& color) {
  auto colorComponents = colorComponentsFromColor(color);
  uint32_t red = static_cast<uint32_t>(colorComponents.red * 255) & 0xFF;
  uint32_t green = static_cast<uint32_t>(colorComponents.green * 255) & 0xFF;
  uint32_t blue = static_cast<uint32_t>(colorComponents.blue * 255) & 0xFF;
  uint32_t alpha = static_cast<uint32_t>(colorComponents.alpha * 255) & 0xFF;
  auto colorValue = (alpha << 24) | (red << 16) | (green << 8) | blue;
  return colorValue;
}

uint32_t TextInputNodeBase::convertColorToTranslucentSelection(
    facebook::react::SharedColor const& color) {
  auto colorComponents = colorComponentsFromColor(color);
  uint32_t red = static_cast<uint32_t>(colorComponents.red * 255) & 0xFF;
  uint32_t green = static_cast<uint32_t>(colorComponents.green * 255) & 0xFF;
  uint32_t blue = static_cast<uint32_t>(colorComponents.blue * 255) & 0xFF;
  uint32_t alpha = 0x55;
  auto colorValue = (alpha << 24) | (red << 16) | (green << 8) | blue;
  return colorValue;
}

void TextInputNodeBase::setPadding(
    facebook::react::RectangleEdges<facebook::react::Float> padding) {
  std::array<ArkUI_NumberValue, 4> value = {
      static_cast<float>(padding.top),
      static_cast<float>(padding.right),
      static_cast<float>(padding.bottom),
      static_cast<float>(padding.left)};
  ArkUI_AttributeItem item = {value.data(), value.size()};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_PADDING, &item);
}

void TextInputNodeBase::setFocusable(bool const& focusable) {
  int32_t focusableValue = 1;
  if (!focusable) {
    focusableValue = 0;
  }
  ArkUI_NumberValue value[] = {{.i32 = focusableValue}};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_FOCUSABLE, &item);
}

void TextInputNodeBase::setSelectAll(bool selectAll) {
  ArkUI_NumberValue value = {.i32 = int32_t(selectAll)};
  ArkUI_AttributeItem item = {&value, 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_SELECT_ALL, &item);
}

void TextInputNodeBase::setResponseRegion(
    facebook::react::Point const& position,
    facebook::react::Size const& size) {
  ArkUI_NumberValue value[] = {
      0.0f, 0.0f, (float)size.width, (float)size.height};
  ArkUI_AttributeItem item = {value, sizeof(value) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_RESPONSE_REGION, &item);
}

void TextInputNodeBase::setFontColor(
    facebook::react::SharedColor const& color) {
  uint32_t colorValue = *color;
  ArkUI_NumberValue preparedColorValue[] = {{.u32 = colorValue}};
  ArkUI_AttributeItem colorItem = {
      preparedColorValue,
      sizeof(preparedColorValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_FONT_COLOR, &colorItem);
}

void TextInputNodeBase::setTextInputLineHeight(
    facebook::react::TextAttributes const& textAttributes) {
  bool allowFontScaling = true;
  if (textAttributes.allowFontScaling.has_value()) {
    allowFontScaling = textAttributes.allowFontScaling.value();
  }
  float lineHeight = static_cast<float>(textAttributes.lineHeight);
  float fontSizeScale = static_cast<float>(textAttributes.fontSizeMultiplier);
  if (std::isnan(lineHeight) || lineHeight < 0) {
    lineHeight = 0;
  }
  if (!allowFontScaling) {
    lineHeight /= fontSizeScale;
  } else {
    float clampedFontSizeScale = fontSizeScale;
    if (textAttributes.maxFontSizeMultiplier) { // if it's 0, we should ignore
      clampedFontSizeScale =
          fminf(fontSizeScale, textAttributes.maxFontSizeMultiplier);
    }
    lineHeight = lineHeight / fontSizeScale *
        clampedFontSizeScale; // ArkUI will scale the font, we divide it by
                              // the scale to cancel that out
  }
  ArkUI_NumberValue value[] = {{.f32 = lineHeight}};
  ArkUI_AttributeItem item = {.value = value, .size = 1};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_LINE_HEIGHT, &item);
}

void TextInputNodeBase::setCommonFontAttributes(
    facebook::react::TextAttributes const& textAttributes) {
  if (textAttributes.fontFamily.empty()) {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_FONT_FAMILY);
  } else {
    ArkUI_AttributeItem item = {.string = textAttributes.fontFamily.c_str()};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_FONT_FAMILY, &item);
  }

  if (std::isnan(textAttributes.fontSize)) {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_FONT_SIZE);
  } else {
    bool allowFontScaling = true;
    if (textAttributes.allowFontScaling.has_value()) {
      allowFontScaling = textAttributes.allowFontScaling.value();
    }
    float fontSize = static_cast<float>(textAttributes.fontSize);
    if (!allowFontScaling) {
      maybeThrow(NativeNodeApi::getInstance()->setLengthMetricUnit(
          m_nodeHandle, ArkUI_LengthMetricUnit::ARKUI_LENGTH_METRIC_UNIT_VP));
    } else {
      maybeThrow(NativeNodeApi::getInstance()->setLengthMetricUnit(
          m_nodeHandle, ArkUI_LengthMetricUnit::ARKUI_LENGTH_METRIC_UNIT_FP));
    }
    std::array<ArkUI_NumberValue, 1> value = {{{.f32 = fontSize}}};
    ArkUI_AttributeItem item = {value.data(), value.size()};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_FONT_SIZE, &item);
  }

  if (textAttributes.fontWeight.has_value()) {
    std::array<ArkUI_NumberValue, 1> value = {
        {{.i32 = static_cast<int32_t>(
              rnoh::convertFontWeight(textAttributes.fontWeight.value()))}}};
    ArkUI_AttributeItem item = {value.data(), value.size()};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_FONT_WEIGHT, &item);
  } else {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_FONT_WEIGHT);
  }

  if (textAttributes.fontStyle.has_value() &&
      textAttributes.fontStyle.value() == facebook::react::FontStyle::Italic) {
    std::array<ArkUI_NumberValue, 1> value = {
        {{.i32 = static_cast<int32_t>(ARKUI_FONT_STYLE_ITALIC)}}};
    ArkUI_AttributeItem item = {value.data(), value.size()};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_FONT_STYLE, &item);
  } else {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_FONT_STYLE);
  }

  if (!std::isnan(textAttributes.letterSpacing)) {
    ArkUI_NumberValue value[] = {
        {.f32 = static_cast<float>(textAttributes.letterSpacing)}};
    ArkUI_AttributeItem item = {.value = value, .size = 1};
    maybeThrow(NativeNodeApi::getInstance()->setAttribute(
        m_nodeHandle, NODE_TEXT_INPUT_LETTER_SPACING, &item));
  } else {
    maybeThrow(NativeNodeApi::getInstance()->resetAttribute(
        m_nodeHandle, NODE_TEXT_INPUT_LETTER_SPACING));
  }
}

void TextInputNodeBase::setTextAlign(
    std::optional<facebook::react::TextAlignment> const& textAlign) {
  if (textAlign.has_value()) {
    std::array<ArkUI_NumberValue, 1> value = {
        {{.i32 = static_cast<int32_t>(
              rnoh::convertTextAlign(textAlign.value()))}}};
    ArkUI_AttributeItem item = {value.data(), value.size()};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_ALIGN, &item);
  } else {
    m_nodeApi->resetAttribute(m_nodeHandle, NODE_TEXT_ALIGN);
  }
}

void TextInputNodeBase::setTextSelection(int32_t start, int32_t end) {
  std::array<ArkUI_NumberValue, 2> value = {{{.i32 = start}, {.i32 = end}}};
  if (start == end) {
    ArkUI_NumberValue value = {.i32 = start};
    ArkUI_AttributeItem item = {&value, sizeof(ArkUI_NumberValue)};
    m_nodeApi->setAttribute(m_nodeHandle, NODE_TEXT_INPUT_CARET_OFFSET, &item);
  } else {
    ArkUI_AttributeItem item = {.value = value.data(), .size = 2};
    m_nodeApi->setAttribute(
        m_nodeHandle, NODE_TEXT_INPUT_TEXT_SELECTION, &item);
  }
}

void TextInputNodeBase::resetMaxLength() {
  m_nodeApi->resetAttribute(m_nodeHandle, NODE_TEXT_INPUT_MAX_LENGTH);
}

void TextInputNodeBase::setAlignment(ArkUI_Alignment const& textAlignment) {
  ArkUI_NumberValue alignmentValue[] = {
      {.i32 = static_cast<int32_t>(textAlignment)}};
  ArkUI_AttributeItem alignmentItem = {
      alignmentValue, sizeof(alignmentValue) / sizeof(ArkUI_NumberValue)};
  m_nodeApi->setAttribute(m_nodeHandle, NODE_ALIGNMENT, &alignmentItem);
}
} // namespace rnoh
