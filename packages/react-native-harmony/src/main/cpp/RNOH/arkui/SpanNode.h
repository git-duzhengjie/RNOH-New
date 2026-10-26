/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "ArkUINode.h"

namespace rnoh {
/**
 * @internal
 */
class ImageSpanNode : public ArkUINode {
 public:
  explicit ImageSpanNode(const ArkUINode::Context::Shared& context);
};

/**
 * @brief A node representing a styled text span in a text component.
 *
 * SpanNode allows rich styling attributes such as font size, color, shadow,
 * decoration, and case transformations to be applied to a segment of text.

 * @actor RNOH_LIBRARY
 */
class SpanNode : public ArkUINode {
 public:
  /**
   * @brief Constructs a SpanNode using ARKUI_NODE_SPAN type.
   *
   * @param context The Context of ArkUINode.
   */
  explicit SpanNode(const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief Sets the textual content of this span.
   * @param text The string content to display in the span.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setSpanContent(const std::string& text);

  /**
   * @brief Sets the font color of the span.
   * @param fontColor A 32-bit ARGB color value.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setFontColor(uint32_t fontColor);

  /**
   * @brief Sets the font size of the span.
   * @param fontSize The font size in pixels.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setFontSize(float fontSize);

  /**
   * @brief Sets the font style (e.g., italic) of the span.
   * @param fontStyle An integer representing the font style.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setFontStyle(int32_t fontStyle);

  /**
   * @brief Sets the font family of the span.
   * @param fontFamily The name of the font family.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setFontFamily(const std::string& fontFamily);

  /**
   * @brief Sets the font weight of the span.
   * @param fontWeight An integer indicating font weight (e.g., 400 for normal,
   * 700 for bold).
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setFontWeight(int32_t fontWeight);

  /**
   * @brief Sets text decoration such as underline or strikethrough.
   * @param decorationType The type of decoration (e.g., underline).
   * @param decorationColor The color of the decoration line (default is red:
   * 0xFFFF0000).
   * @param decorationStyle The style of decoration (e.g., solid, dashed).
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setTextDecoration(
      int32_t decorationType,
      uint32_t decorationColor = 0xFFFF0000,
      int32_t decorationStyle = 0);

  /**
   * @brief Sets the letter spacing for the span.
   * @param textLetterSpacing The spacing between letters in pixels.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setTextLetterSpacing(float textLetterSpacing);

  /**
   * @brief Sets shadow attributes for the text in the span.
   * @param textShadowRadius The blur radius of the shadow.
   * @param textShadowType The type of shadow (e.g., inner or outer).
   * @param textShadowColor The color of the shadow.
   * @param textShadowOffsetX The horizontal offset of the shadow.
   * @param textShadowOffsetY The vertical offset of the shadow.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setTextShadow(
      float textShadowRadius,
      int32_t textShadowType,
      uint32_t textShadowColor,
      float textShadowOffsetX,
      float textShadowOffsetY);

  /**
   * @brief Sets the line height for the span.
   * @param textLineHeight The height of each text line in pixels.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setTextLineHeight(float textLineHeight);

  /**
   * @brief Sets the case transformation of the text.
   * @param textCase An integer representing the desired case (e.g., uppercase,
   * lowercase).
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setTextCase(int32_t textCase);

  /**
   * @brief Sets the background color of the span.
   * @param color A 32-bit ARGB color value for the background.
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setBackgroundStyle(uint32_t color);

  /**
   * @brief Resets the background style to default (no background).
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& resetBackgroundStyle();

  /**
   * @brief Sets the length metric unit for size-related attributes.
   * @param unit The unit to be used (e.g., px, dp, vp).
   * @return Reference to the current SpanNode instance.
   */
  SpanNode& setLengthMetricUnit(ArkUI_LengthMetricUnit unit);
};

} // namespace rnoh