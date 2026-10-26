/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// =============================================================================
// Module: Text 组件 — StyledStringWrapper 纯 helper 函数
// Tier: 1 (纯 C++, 零平台依赖)
// Functions: normalizeFontSize, utf16Length, stringCapitalize,
// textCaseTransform Real source: linked via StyledStringHelpersImpl.cpp
// (extracted from StyledStringWrapper.cpp)
// =============================================================================

#include <gtest/gtest.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
#include "react/renderer/attributedstring/primitives.h"

// Declarations — implementations in StyledStringHelpersImpl.cpp
float normalizeFontSize(float fontSize);
size_t utf16Length(const std::string& str);
std::string stringCapitalize(const std::string& strInput);
void textCaseTransform(
    std::string& textContent,
    facebook::react::TextTransform type);

// Must match DEFAULT_FONT_SIZE in StyledStringHelpersImpl.cpp
constexpr float DEFAULT_FONT_SIZE = 14.0f;

// ===========================================================================
// Tests: normalizeFontSize
// ===========================================================================

TEST(StyledStringHelpers, NormalizeFontSize_PositiveValue_ReturnsValue) {
  EXPECT_FLOAT_EQ(normalizeFontSize(16.0f), 16.0f);
}
TEST(StyledStringHelpers, NormalizeFontSize_Zero_ReturnsDefault) {
  EXPECT_FLOAT_EQ(normalizeFontSize(0.0f), DEFAULT_FONT_SIZE);
}
TEST(StyledStringHelpers, NormalizeFontSize_Negative_ReturnsDefault) {
  EXPECT_FLOAT_EQ(normalizeFontSize(-5.0f), DEFAULT_FONT_SIZE);
}

// ===========================================================================
// Tests: utf16Length
// ===========================================================================

TEST(StyledStringHelpers, Utf16Length_EmptyString_ReturnsZero) {
  EXPECT_EQ(utf16Length(""), 0u);
}
TEST(StyledStringHelpers, Utf16Length_Ascii_ReturnsCharCount) {
  EXPECT_EQ(utf16Length("hello"), 5u);
}
TEST(StyledStringHelpers, Utf16Length_TwoByteChars_ReturnsCharCount) {
  EXPECT_EQ(utf16Length("\xE4\xBD\xA0\xE5\xA5\xBD"), 2u);
}
TEST(StyledStringHelpers, Utf16Length_FourByteEmoji_ReturnsTwoCodeUnits) {
  EXPECT_EQ(utf16Length("\xF0\x9F\x91\x8B"), 2u);
}
TEST(StyledStringHelpers, Utf16Length_MixedAsciiAndMultiByte_ReturnsCharCount) {
  EXPECT_EQ(utf16Length("ab\xE4\xBD\xA0\xE5\xA5\xBD"), 4u);
}

// ===========================================================================
// Tests: stringCapitalize
// ===========================================================================

TEST(StyledStringHelpers, StringCapitalize_EmptyString_ReturnsEmpty) {
  EXPECT_EQ(stringCapitalize(""), "");
}
TEST(StyledStringHelpers, StringCapitalize_SingleWord_ReturnsCapitalized) {
  EXPECT_EQ(stringCapitalize("hello"), "Hello");
}
TEST(
    StyledStringHelpers,
    StringCapitalize_MultipleWords_ReturnsAllCapitalized) {
  EXPECT_EQ(stringCapitalize("hello world"), "Hello World");
}
TEST(StyledStringHelpers, StringCapitalize_AllCaps_ReturnsCapitalized) {
  EXPECT_EQ(stringCapitalize("HELLO"), "Hello");
}
TEST(StyledStringHelpers, StringCapitalize_NumericFirstChar_PreservesNumbers) {
  EXPECT_EQ(stringCapitalize("123abc"), "123abc");
}

// ===========================================================================
// Tests: textCaseTransform
// ===========================================================================

TEST(StyledStringHelpers, TextCaseTransform_None_PreservesText) {
  std::string s = "Hello World";
  textCaseTransform(s, facebook::react::TextTransform::None);
  EXPECT_EQ(s, "Hello World");
}
TEST(StyledStringHelpers, TextCaseTransform_Uppercase_ConvertsToUppercase) {
  std::string s = "hello world";
  textCaseTransform(s, facebook::react::TextTransform::Uppercase);
  EXPECT_EQ(s, "HELLO WORLD");
}
TEST(StyledStringHelpers, TextCaseTransform_Lowercase_ConvertsToLowercase) {
  std::string s = "HELLO WORLD";
  textCaseTransform(s, facebook::react::TextTransform::Lowercase);
  EXPECT_EQ(s, "hello world");
}
TEST(StyledStringHelpers, TextCaseTransform_Capitalize_ConvertsToCapitalized) {
  std::string s = "hello world";
  textCaseTransform(s, facebook::react::TextTransform::Capitalize);
  EXPECT_EQ(s, "Hello World");
}
TEST(StyledStringHelpers, TextCaseTransform_EmptyString_NoCrash) {
  std::string s = "";
  textCaseTransform(s, facebook::react::TextTransform::Uppercase);
  EXPECT_EQ(s, "");
}
