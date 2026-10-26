/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "RNOH/arkui/conversions.h"

using namespace rnoh;

// ===========================================================================
// convertInputType (KeyboardType → ArkUI_TextInputType)
// ===========================================================================

TEST(ConvertInputType, Numeric_ReturnsNumberType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::Numeric),
      ARKUI_TEXTINPUT_TYPE_NUMBER);
}

TEST(ConvertInputType, NumberPad_ReturnsNumberType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::NumberPad),
      ARKUI_TEXTINPUT_TYPE_NUMBER);
}

TEST(ConvertInputType, DecimalPad_ReturnsNumberDecimalType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::DecimalPad),
      ARKUI_TEXTINPUT_TYPE_NUMBER_DECIMAL);
}

TEST(ConvertInputType, PhonePad_ReturnsPhoneNumberType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::PhonePad),
      ARKUI_TEXTINPUT_TYPE_PHONE_NUMBER);
}

TEST(ConvertInputType, EmailAddress_ReturnsEmailType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::EmailAddress),
      ARKUI_TEXTINPUT_TYPE_EMAIL);
}

TEST(ConvertInputType, Default_ReturnsNormalType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::Default),
      ARKUI_TEXTINPUT_TYPE_NORMAL);
}

// ===========================================================================
// convertEnterKeyType (ReturnKeyType → ArkUI_EnterKeyType)
// ===========================================================================

TEST(ConvertEnterKeyType, Done_ReturnsDoneType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Done),
      ARKUI_ENTER_KEY_TYPE_DONE);
}

TEST(ConvertEnterKeyType, Go_ReturnsGoType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Go),
      ARKUI_ENTER_KEY_TYPE_GO);
}

TEST(ConvertEnterKeyType, Next_ReturnsNextType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Next),
      ARKUI_ENTER_KEY_TYPE_NEXT);
}

TEST(ConvertEnterKeyType, Search_ReturnsSearchType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Search),
      ARKUI_ENTER_KEY_TYPE_SEARCH);
}

TEST(ConvertEnterKeyType, Send_ReturnsSendType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Send),
      ARKUI_ENTER_KEY_TYPE_SEND);
}

TEST(ConvertEnterKeyType, Previous_ReturnsPreviousType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Previous),
      ARKUI_ENTER_KEY_TYPE_PREVIOUS);
}

TEST(ConvertEnterKeyType, Default_ReturnsDoneType) {
  EXPECT_EQ(
      convertEnterKeyType(facebook::react::ReturnKeyType::Default),
      ARKUI_ENTER_KEY_TYPE_DONE);
}

// ===========================================================================
// convertEnterKeyLabel (string → ArkUI_EnterKeyType)
// ===========================================================================

TEST(ConvertEnterKeyLabel, DoneString_ReturnsDoneType) {
  EXPECT_EQ(convertEnterKeyLabel("done"), ARKUI_ENTER_KEY_TYPE_DONE);
}

TEST(ConvertEnterKeyLabel, GoString_ReturnsGoType) {
  EXPECT_EQ(convertEnterKeyLabel("go"), ARKUI_ENTER_KEY_TYPE_GO);
}

TEST(ConvertEnterKeyLabel, NextString_ReturnsNextType) {
  EXPECT_EQ(convertEnterKeyLabel("next"), ARKUI_ENTER_KEY_TYPE_NEXT);
}

TEST(ConvertEnterKeyLabel, SearchString_ReturnsSearchType) {
  EXPECT_EQ(convertEnterKeyLabel("search"), ARKUI_ENTER_KEY_TYPE_SEARCH);
}

TEST(ConvertEnterKeyLabel, SendString_ReturnsSendType) {
  EXPECT_EQ(convertEnterKeyLabel("send"), ARKUI_ENTER_KEY_TYPE_SEND);
}

TEST(ConvertEnterKeyLabel, PreviousString_ReturnsPreviousType) {
  EXPECT_EQ(convertEnterKeyLabel("previous"), ARKUI_ENTER_KEY_TYPE_PREVIOUS);
}

TEST(ConvertEnterKeyLabel, UnknownString_ReturnsDoneType) {
  EXPECT_EQ(convertEnterKeyLabel("unknown"), ARKUI_ENTER_KEY_TYPE_DONE);
}

TEST(ConvertEnterKeyLabel, EmptyString_ReturnsDoneType) {
  EXPECT_EQ(convertEnterKeyLabel(""), ARKUI_ENTER_KEY_TYPE_DONE);
}

// ===========================================================================
// convertContentType (string → ArkUI_NumberValue)
// ===========================================================================

TEST(ConvertContentType, Username_ReturnsUserNameEnum) {
  auto r = convertContentType("username");
  EXPECT_EQ(r.i32, TextInput_TextContentType::USER_NAME);
}

TEST(ConvertContentType, Password_ReturnsPasswordEnum) {
  auto r = convertContentType("password");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PASSWORD);
}

TEST(ConvertContentType, NewPassword_ReturnsNewPasswordEnum) {
  auto r = convertContentType("newPassword");
  EXPECT_EQ(r.i32, TextInput_TextContentType::NEW_PASSWORD);
}

TEST(ConvertContentType, FullName_ReturnsPersonFullNameEnum) {
  auto r = convertContentType("personFullName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PERSON_FULL_NAME);
}

TEST(ConvertContentType, NameAlias_ReturnsPersonFullNameEnum) {
  auto r = convertContentType("name");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PERSON_FULL_NAME);
}

TEST(ConvertContentType, FamilyName_ReturnsPersonLastNameEnum) {
  auto r = convertContentType("familyName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PERSON_LAST_NAME);
}

TEST(ConvertContentType, GivenName_ReturnsPersonFirstNameEnum) {
  auto r = convertContentType("givenName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PERSON_FIRST_NAME);
}

TEST(ConvertContentType, EmailAddress_ReturnsEmailAddressEnum) {
  auto r = convertContentType("emailAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::EMAIL_ADDRESS);
}

TEST(ConvertContentType, PhoneNumber_ReturnsPhoneNumberEnum) {
  auto r = convertContentType("phoneNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PHONE_NUMBER);
}

TEST(ConvertContentType, TelephoneNumberAlias_ReturnsPhoneNumberEnum) {
  auto r = convertContentType("telephoneNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PHONE_NUMBER);
}

TEST(ConvertContentType, FullStreetAddress_ReturnsFullStreetAddressEnum) {
  auto r = convertContentType("fullStreetAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::FULL_STREET_ADDRESS);
}

TEST(ConvertContentType, BankCardNumber_ReturnsBankCardNumberEnum) {
  auto r = convertContentType("bankCardNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::BANK_CARD_NUMBER);
}

TEST(ConvertContentType, CreditCardNumberAlias_ReturnsBankCardNumberEnum) {
  auto r = convertContentType("creditCardNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::BANK_CARD_NUMBER);
}

TEST(ConvertContentType, IdCardNumber_ReturnsIdCardNumberEnum) {
  auto r = convertContentType("idCardNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::ID_CARD_NUMBER);
}

TEST(ConvertContentType, NickName_ReturnsNicknameEnum) {
  auto r = convertContentType("nickName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::NICKNAME);
}

TEST(ConvertContentType, AddressCityAndState_ReturnsCorrectEnum) {
  auto r = convertContentType("addressCityAndState");
  EXPECT_EQ(r.i32, TextInput_TextContentType::ADDRESS_CITY_AND_STATE);
}

TEST(ConvertContentType, LicensePlate_ReturnsLicensePlateEnum) {
  auto r = convertContentType("licensePlate");
  EXPECT_EQ(r.i32, TextInput_TextContentType::LICENSE_PLATE);
}

TEST(ConvertContentType, UnknownString_ReturnsMinusOne) {
  auto r = convertContentType("nonExistentType");
  EXPECT_EQ(r.i32, -1);
}

TEST(ConvertContentType, EmptyString_ReturnsMinusOne) {
  auto r = convertContentType("");
  EXPECT_EQ(r.i32, -1);
}

// ===========================================================================
// convertColorToTranslucentUnderline (SharedColor → uint32 ARGB)
// ===========================================================================

TEST(ConvertColorToTranslucentUnderline, Black_ReturnsFullAlphaBlack) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(0, 0, 0, 255));
  auto result = convertColorToTranslucentUnderline(color);
  EXPECT_EQ(result, 0xFF000000u);
}

TEST(ConvertColorToTranslucentUnderline, White_ReturnsFullAlphaWhite) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 255, 255, 255));
  auto result = convertColorToTranslucentUnderline(color);
  EXPECT_EQ(result, 0xFFFFFFFFu);
}

TEST(ConvertColorToTranslucentUnderline, Red_ReturnsFullAlphaRed) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 0, 0, 255));
  auto result = convertColorToTranslucentUnderline(color);
  EXPECT_EQ(result, 0xFFFF0000u);
}

TEST(ConvertColorToTranslucentUnderline, SemiTransparent_PreservesAlpha) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 0, 0, 128));
  auto result = convertColorToTranslucentUnderline(color);
  uint32_t expectedAlpha = static_cast<uint32_t>(
      static_cast<float>(128) / 255.0f * 255.0f);
  EXPECT_EQ((result >> 24) & 0xFF, expectedAlpha);
  EXPECT_EQ((result >> 16) & 0xFF, 255u);
}

TEST(ConvertColorToTranslucentUnderline, FullyTransparent_AlphaIsZero) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(0, 0, 0, 0));
  auto result = convertColorToTranslucentUnderline(color);
  EXPECT_EQ(result, 0x00000000u);
}

// ===========================================================================
// convertColorToTranslucentSelection (SharedColor → uint32 with alpha=0x55)
// ===========================================================================

TEST(ConvertColorToTranslucentSelection, Black_ReturnsFixedAlphaBlack) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(0, 0, 0, 255));
  auto result = convertColorToTranslucentSelection(color);
  EXPECT_EQ((result >> 24) & 0xFF, 0x55u);
  EXPECT_EQ(result, 0x55000000u);
}

TEST(ConvertColorToTranslucentSelection, Red_ReturnsFixedAlphaRed) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 0, 0, 255));
  auto result = convertColorToTranslucentSelection(color);
  EXPECT_EQ(result, 0x55FF0000u);
}

TEST(ConvertColorToTranslucentSelection, White_ReturnsFixedAlphaWhite) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 255, 255, 255));
  auto result = convertColorToTranslucentSelection(color);
  EXPECT_EQ(result, 0x55FFFFFFu);
}

TEST(ConvertColorToTranslucentSelection, SemiTransparentAlpha_Ignored) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 0, 0, 230));
  auto result = convertColorToTranslucentSelection(color);
  EXPECT_EQ((result >> 24) & 0xFF, 0x55u);
  EXPECT_EQ((result >> 16) & 0xFF, 255u);
}

// ===========================================================================
// convertTextAlign (TextAlignment → ArkUI_TextAlignment)
// ===========================================================================

TEST(ConvertTextAlign, Natural_ReturnsStart) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Natural),
      ARKUI_TEXT_ALIGNMENT_START);
}

TEST(ConvertTextAlign, Left_ReturnsStart) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Left),
      ARKUI_TEXT_ALIGNMENT_START);
}

TEST(ConvertTextAlign, Right_ReturnsEnd) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Right),
      ARKUI_TEXT_ALIGNMENT_END);
}

TEST(ConvertTextAlign, Center_ReturnsCenter) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Center),
      ARKUI_TEXT_ALIGNMENT_CENTER);
}

TEST(ConvertTextAlign, Justified_ReturnsJustify) {
  EXPECT_EQ(
      convertTextAlign(facebook::react::TextAlignment::Justified),
      ARKUI_TEXT_ALIGNMENT_JUSTIFY);
}

// ===========================================================================
// convertFontWeight (FontWeight → ArkUI_FontWeight)
// ===========================================================================

TEST(ConvertFontWeight, Weight100_ReturnsW100) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight100),
      ARKUI_FONT_WEIGHT_W100);
}

TEST(ConvertFontWeight, Thin200_ReturnsW200) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Thin),
      ARKUI_FONT_WEIGHT_W200);
}

TEST(ConvertFontWeight, Weight400_ReturnsW400) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Regular),
      ARKUI_FONT_WEIGHT_W400);
}

TEST(ConvertFontWeight, Weight700_ReturnsW700) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Bold),
      ARKUI_FONT_WEIGHT_W700);
}

TEST(ConvertFontWeight, Weight900_ReturnsW900) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Black),
      ARKUI_FONT_WEIGHT_W900);
}

// ===========================================================================
// convertFontWeight — 补齐 W300/W500/W600/W800 未覆盖分支 (§4.4 边界覆盖)
// ===========================================================================

TEST(ConvertFontWeight, Weight300_ReturnsW300) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Weight300),
      ARKUI_FONT_WEIGHT_W300);
}

TEST(ConvertFontWeight, Medium500_ReturnsW500) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Medium),
      ARKUI_FONT_WEIGHT_W500);
}

TEST(ConvertFontWeight, Semibold600_ReturnsW600) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Semibold),
      ARKUI_FONT_WEIGHT_W600);
}

TEST(ConvertFontWeight, Heavy800_ReturnsW800) {
  EXPECT_EQ(
      convertFontWeight(facebook::react::FontWeight::Heavy),
      ARKUI_FONT_WEIGHT_W800);
}

// ===========================================================================
// convertContentType — 补齐 25 个未覆盖字符串分支 (§4.4 边界覆盖)
// 注: 多个字符串映射到同一枚举属于别名映射(覆盖各自独立 if 分支), 非冗余
// ===========================================================================

TEST(ConvertContentType, AddressCity_ReturnsCityAddress) {
  auto r = convertContentType("addressCity");
  EXPECT_EQ(r.i32, TextInput_TextContentType::CITY_ADDRESS);
}

TEST(ConvertContentType, AddressState_ReturnsProvinceAddress) {
  auto r = convertContentType("addressState");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PROVINCE_ADDRESS);
}

TEST(ConvertContentType, CountryName_ReturnsCountryAddress) {
  auto r = convertContentType("countryName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::COUNTRY_ADDRESS);
}

TEST(ConvertContentType, Sublocality_ReturnsDistrictAddress) {
  auto r = convertContentType("sublocality");
  EXPECT_EQ(r.i32, TextInput_TextContentType::DISTRICT_ADDRESS);
}

TEST(ConvertContentType, HouseNumber_ReturnsHouseNumber) {
  auto r = convertContentType("houseNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::HOUSE_NUMBER);
}

TEST(ConvertContentType, DistrictAddress_ReturnsDistrictAddress) {
  auto r = convertContentType("districtAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::DISTRICT_ADDRESS);
}

TEST(ConvertContentType, CityAddress_ReturnsCityAddress) {
  auto r = convertContentType("cityAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::CITY_ADDRESS);
}

TEST(ConvertContentType, ProvinceAddress_ReturnsProvinceAddress) {
  auto r = convertContentType("provinceAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PROVINCE_ADDRESS);
}

TEST(ConvertContentType, CountryAddress_ReturnsCountryAddress) {
  auto r = convertContentType("countryAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::COUNTRY_ADDRESS);
}

TEST(ConvertContentType, PersonLastName_ReturnsPersonLastName) {
  auto r = convertContentType("personLastName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PERSON_LAST_NAME);
}

TEST(ConvertContentType, PersonFirstName_ReturnsPersonFirstName) {
  auto r = convertContentType("personFirstName");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PERSON_FIRST_NAME);
}

TEST(ConvertContentType, PhoneCountryCode_ReturnsPhoneCountryCode) {
  auto r = convertContentType("phoneCountryCode");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PHONE_COUNTRY_CODE);
}

TEST(ConvertContentType, FullPhoneNumber_ReturnsFullPhoneNumber) {
  auto r = convertContentType("fullPhoneNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::FULL_PHONE_NUMBER);
}

TEST(ConvertContentType, DetailInfoWithoutStreet_ReturnsCorrectEnum) {
  auto r = convertContentType("detailInfoWithoutStreet");
  EXPECT_EQ(r.i32, TextInput_TextContentType::DETAIL_INFO_WITHOUT_STREET);
}

TEST(ConvertContentType, FormatAddress_ReturnsFormatAddress) {
  auto r = convertContentType("formatAddress");
  EXPECT_EQ(r.i32, TextInput_TextContentType::FORMAT_ADDRESS);
}

TEST(ConvertContentType, PassportNumber_ReturnsPassportNumber) {
  auto r = convertContentType("passportNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::PASSPORT_NUMBER);
}

TEST(ConvertContentType, Validity_ReturnsValidity) {
  auto r = convertContentType("validity");
  EXPECT_EQ(r.i32, TextInput_TextContentType::VALIDITY);
}

TEST(ConvertContentType, IssueAt_ReturnsIssueAt) {
  auto r = convertContentType("issueAt");
  EXPECT_EQ(r.i32, TextInput_TextContentType::ISSUE_AT);
}

TEST(ConvertContentType, Organization_ReturnsOrganization) {
  auto r = convertContentType("organization");
  EXPECT_EQ(r.i32, TextInput_TextContentType::ORGANIZATION);
}

TEST(ConvertContentType, TaxId_ReturnsTaxId) {
  auto r = convertContentType("taxId");
  EXPECT_EQ(r.i32, TextInput_TextContentType::TAX_ID);
}

TEST(ConvertContentType, FlightNumber_ReturnsFlightNumber) {
  auto r = convertContentType("flightNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::FLIGHT_NUMBER);
}

TEST(ConvertContentType, LicenseNumber_ReturnsLicenseNumber) {
  auto r = convertContentType("licenseNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::LICENSE_NUMBER);
}

TEST(ConvertContentType, LicenseFileNumber_ReturnsLicenseFileNumber) {
  auto r = convertContentType("licenseFileNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::LICENSE_FILE_NUMBER);
}

TEST(ConvertContentType, EngineNumber_ReturnsEngineNumber) {
  auto r = convertContentType("engineNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::ENGINE_NUMBER);
}

TEST(ConvertContentType, LicenseChassisNumber_ReturnsLicenseChassisNumber) {
  auto r = convertContentType("licenseChassisNumber");
  EXPECT_EQ(r.i32, TextInput_TextContentType::LICENSE_CHASSIS_NUMBER);
}

// ===========================================================================
// convertReactBorderStyleToArk — 补齐零调用函数 (§4.4)
// ===========================================================================

TEST(ConvertReactBorderStyleToArk, Solid_ReturnsSolid) {
  EXPECT_EQ(
      convertReactBorderStyleToArk(facebook::react::BorderStyle::Solid),
      ARKUI_BORDER_STYLE_SOLID);
}

TEST(ConvertReactBorderStyleToArk, Dotted_ReturnsDotted) {
  EXPECT_EQ(
      convertReactBorderStyleToArk(facebook::react::BorderStyle::Dotted),
      ARKUI_BORDER_STYLE_DOTTED);
}

TEST(ConvertReactBorderStyleToArk, Dashed_ReturnsDashed) {
  EXPECT_EQ(
      convertReactBorderStyleToArk(facebook::react::BorderStyle::Dashed),
      ARKUI_BORDER_STYLE_DASHED);
}

// ===========================================================================
// convertColorToTranslucent — 补齐零调用函数 (deprecated, fixed alpha 0x55)
// ===========================================================================

TEST(ConvertColorToTranslucent, Red_ReturnsFixedAlphaRed) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 0, 0, 255));
  auto result = convertColorToTranslucent(color);
  EXPECT_EQ(result, 0x55FF0000u);
}

// ===========================================================================
// convertTextAreaEnterKeyType — 补齐零调用函数 (Default→NEW_LINE)
// ===========================================================================

TEST(ConvertTextAreaEnterKeyType, Default_ReturnsNewLine) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Default),
      ARKUI_ENTER_KEY_TYPE_NEW_LINE);
}

TEST(ConvertTextAreaEnterKeyType, Done_ReturnsDoneType) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Done),
      ARKUI_ENTER_KEY_TYPE_DONE);
}

TEST(ConvertTextAreaEnterKeyType, Go_ReturnsGoType) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Go),
      ARKUI_ENTER_KEY_TYPE_GO);
}

TEST(ConvertTextAreaEnterKeyType, Next_ReturnsNextType) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Next),
      ARKUI_ENTER_KEY_TYPE_NEXT);
}

TEST(ConvertTextAreaEnterKeyType, Search_ReturnsSearchType) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Search),
      ARKUI_ENTER_KEY_TYPE_SEARCH);
}

TEST(ConvertTextAreaEnterKeyType, Send_ReturnsSendType) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Send),
      ARKUI_ENTER_KEY_TYPE_SEND);
}

TEST(ConvertTextAreaEnterKeyType, Previous_ReturnsPreviousType) {
  EXPECT_EQ(
      convertTextAreaEnterKeyType(facebook::react::ReturnKeyType::Previous),
      ARKUI_ENTER_KEY_TYPE_PREVIOUS);
}

// ===========================================================================
// convertTextAreaEnterKeyLabel — 补齐零调用函数 (empty→NEW_LINE)
// ===========================================================================

TEST(ConvertTextAreaEnterKeyLabel, EmptyString_ReturnsNewLine) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel(""), ARKUI_ENTER_KEY_TYPE_NEW_LINE);
}

TEST(ConvertTextAreaEnterKeyLabel, DoneString_ReturnsDoneType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("done"), ARKUI_ENTER_KEY_TYPE_DONE);
}

TEST(ConvertTextAreaEnterKeyLabel, GoString_ReturnsGoType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("go"), ARKUI_ENTER_KEY_TYPE_GO);
}

TEST(ConvertTextAreaEnterKeyLabel, NextString_ReturnsNextType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("next"), ARKUI_ENTER_KEY_TYPE_NEXT);
}

TEST(ConvertTextAreaEnterKeyLabel, SearchString_ReturnsSearchType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("search"), ARKUI_ENTER_KEY_TYPE_SEARCH);
}

TEST(ConvertTextAreaEnterKeyLabel, SendString_ReturnsSendType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("send"), ARKUI_ENTER_KEY_TYPE_SEND);
}

TEST(ConvertTextAreaEnterKeyLabel, PreviousString_ReturnsPreviousType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("previous"), ARKUI_ENTER_KEY_TYPE_PREVIOUS);
}

TEST(ConvertTextAreaEnterKeyLabel, UnknownString_ReturnsDoneType) {
  EXPECT_EQ(convertTextAreaEnterKeyLabel("unknown"), ARKUI_ENTER_KEY_TYPE_DONE);
}

// ===========================================================================
// convertTextAreaInputType — 补齐零调用函数
// ===========================================================================

TEST(ConvertTextAreaInputType, Numeric_ReturnsNumberType) {
  EXPECT_EQ(
      convertTextAreaInputType(facebook::react::KeyboardType::Numeric),
      ARKUI_TEXTAREA_TYPE_NUMBER);
}

TEST(ConvertTextAreaInputType, NumberPad_ReturnsNumberType) {
  EXPECT_EQ(
      convertTextAreaInputType(facebook::react::KeyboardType::NumberPad),
      ARKUI_TEXTAREA_TYPE_NUMBER);
}

TEST(ConvertTextAreaInputType, PhonePad_ReturnsPhoneNumberType) {
  EXPECT_EQ(
      convertTextAreaInputType(facebook::react::KeyboardType::PhonePad),
      ARKUI_TEXTAREA_TYPE_PHONE_NUMBER);
}

TEST(ConvertTextAreaInputType, EmailAddress_ReturnsEmailType) {
  EXPECT_EQ(
      convertTextAreaInputType(facebook::react::KeyboardType::EmailAddress),
      ARKUI_TEXTAREA_TYPE_EMAIL);
}

TEST(ConvertTextAreaInputType, Default_ReturnsNormalType) {
  EXPECT_EQ(
      convertTextAreaInputType(facebook::react::KeyboardType::Default),
      ARKUI_TEXTAREA_TYPE_NORMAL);
}

// ===========================================================================
// convertImportantForAutofill — 补齐零调用函数
// ===========================================================================

TEST(ConvertImportantForAutofill, Yes_ReturnsTrue) {
  EXPECT_TRUE(convertImportantForAutofill("yes"));
}

TEST(ConvertImportantForAutofill, No_ReturnsFalse) {
  EXPECT_FALSE(convertImportantForAutofill("no"));
}

TEST(ConvertImportantForAutofill, Auto_ReturnsFalse) {
  EXPECT_FALSE(convertImportantForAutofill("auto"));
}

TEST(ConvertImportantForAutofill, Unknown_ReturnsTrue) {
  EXPECT_TRUE(convertImportantForAutofill("unknown"));
}

TEST(ConvertImportantForAutofill, NoExcludeDescendants_ReturnsFalse) {
  EXPECT_FALSE(convertImportantForAutofill("noExcludeDescendants"));
}

TEST(ConvertImportantForAutofill, YesExcludeDescendants_ReturnsTrue) {
  EXPECT_TRUE(convertImportantForAutofill("yesExcludeDescendants"));
}

// ===========================================================================
// convertColorToTranslucent — 补齐 black/white/transparent 边界
// ===========================================================================

TEST(ConvertColorToTranslucent, Black_ReturnsFixedAlphaBlack) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(0, 0, 0, 255));
  EXPECT_EQ(convertColorToTranslucent(color), 0x55000000u);
}

TEST(ConvertColorToTranslucent, White_ReturnsFixedAlphaWhite) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(255, 255, 255, 255));
  EXPECT_EQ(convertColorToTranslucent(color), 0x55FFFFFFu);
}

TEST(ConvertColorToTranslucent, FullyTransparent_IgnoredAlpha) {
  auto color = facebook::react::SharedColor(
      facebook::react::hostPlatformColorFromRGBA(100, 50, 25, 0));
  auto result = convertColorToTranslucent(color);
  EXPECT_EQ((result >> 24) & 0xFF, 0x55u);
  EXPECT_EQ((result >> 16) & 0xFF, 100u);
}

// ===========================================================================
// convertInputType — 补齐 default 分支显式测试
// ===========================================================================

TEST(ConvertInputType, Url_ReturnsNormalType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::URL),
      ARKUI_TEXTINPUT_TYPE_NORMAL);
}

TEST(ConvertInputType, AsciiCapable_ReturnsNormalType) {
  EXPECT_EQ(
      convertInputType(facebook::react::KeyboardType::ASCIICapable),
      ARKUI_TEXTINPUT_TYPE_NORMAL);
}
