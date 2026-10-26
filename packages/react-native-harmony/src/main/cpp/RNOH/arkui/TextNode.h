/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include "ArkUINode.h"
#include "RNOH/ArkUITypography.h"
#include "react/renderer/attributedstring/primitives.h"

namespace rnoh {

class TextNodeDelegate {
 public:
  virtual ~TextNodeDelegate() = default;
  virtual void onDisappear(){};
};

/**
 * @actor RNOH_LIBRARY
 *
 * TextNode is the wrapper of a ARKUI_NODE_TEXT.
 */
class TextNode : public ArkUINode {
 private:
  enum {
    FLAG_PADDING = 0,
    FLAG_MINFONTSIZE,
    FLAG_MAXFONTSIZE,
    FLAG_COPYOPTION,
    FLAG_ENABLE,
    FLAG_MAX
  };
  bool m_initFlag[FLAG_MAX] = {0};

  float m_minFontSize = 0.0;
  float m_maxFontSize = 0.0;
  int32_t m_textCopyOption = 0;
  bool m_enableFlag = false;
  float m_top = 0.0;
  float m_right = 0.0;
  float m_bottom = 0.0;
  float m_left = 0.0;

 public:
  explicit TextNode(const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief append a new child node, usually an attachment
   * @param child child node to insert
   * @param index index before which the child will be inserted
   * @throws std::runtime_error if operation failed
   */
  void insertChild(ArkUINode& child, std::size_t index);
  /**
   * @brief remove a child node, usually an attachment
   * @param child child node to remove
   * @throws std::runtime_error if operation failed
   */
  void removeChild(ArkUINode& child);

  /**
   * @brief set text content, necessary for correct text selection
   * @param text text content
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextContent(const std::string& text);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use OH_Drawing_SetTextStyleColor
   * instead.
   * @brief set font color
   * @param fontColor font color, in 0xAARRGGBB format.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setFontColor(uint32_t fontColor);
  /**
   * @brief reset font color
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& resetFontColor();
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleFontSize instead.
   * @brief set font size
   * @param fontSize font size
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setFontSize(float fontSize);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleFontStyle instead.
   * @brief set font style
   * @param fontStyle font style
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setFontStyle(int32_t fontStyle);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleFontWeight instead.
   * @brief set font weight
   * @param fontWeight font weight
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setFontWeight(int32_t fontWeight);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleFontHeight instead.
   * @brief set line height
   * @param textLineHeight line height, in fp.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextLineHeight(float textLineHeight);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleDecoration instead.
   * @brief set text decoration style and color
   * @param decorationStyle decoration style
   * @param decorationColor decoration color, in 0xAARRGGBB format, optional by
   * default value 0xFFFF0000
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextDecoration(
      int32_t decorationStyle,
      uint32_t decorationColor = 0xFFFF0000);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use textCaseTransform instead
   * @brief set text case
   * @param textCase text case
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextCase(int32_t textCase);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleLetterSpacing instead.
   * @brief set letter spacing
   * @param textLetterSpacing letter spacing, in fp.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextLetterSpacing(float textLetterSpacing);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTypographyTextMaxLines instead.
   * @brief set maximum number of lines
   * @param textMaxLines maximum number of lines
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextMaxLines(int32_t textMaxLines);
  /**
   * @brief reset the maximum number of lines
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& resetTextMaxLines();
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTypographyTextAlign instead.
   * @brief set horizontal alignment mode of the text
   * @param align horizontal alignment mode of the text
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextAlign(int32_t align);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTypographyTextEllipsisModal instead.
   * @brief set ellipsis position
   * @param ellipsisMode ellipsis mode
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextEllipsisMode(int32_t ellipsisMode);
  /**
   * @brief set display mode when the text is too long
   * @param textOverflow display mode when the text is too long
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextOverflow(int32_t textOverflow);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTextStyleFontFamilies instead.
   * @brief set fonts
   * @param fontFamily fonts, separated by commas
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setFontFamily(const std::string& fontFamily);
  /**
   * @brief set copy option
   * @param textCopyOption copy option
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextCopyOption(int32_t textCopyOption);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_TypographyHandlerAddPlaceholder instead.
   * @brief set baseLine offset
   * @param textBaselineOffset baseLine offset, in fp.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextBaselineOffset(float textBaselineOffset);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use OH_Drawing_TextStyleAddShadow
   * instead.
   * @brief set shadow
   * @param textShadowRadius shadow radius, in vp.
   * @param textShadowType shadow type
   * @param textShadowColor shadow color, in 0xAARRGGBB format.
   * @param textShadowOffsetX shadow offset x, in vp.
   * @param textShadowOffsetY shadow offset y, in vp.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextShadow(
      float textShadowRadius,
      int32_t textShadowType,
      uint32_t textShadowColor,
      float textShadowOffsetX,
      float textShadowOffsetY);
  /**
   * @brief set minimum font size
   * @param minFontSize minimum font size
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setMinFontSize(float minFontSize);
  /**
   * @brief set maximum font size
   * @param maxFontSize maximum font size
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setMaxFontSize(float maxFontSize);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use OH_Drawing APIs instead.
   * @brief set text font size, weight and style
   * @param fontSize font size
   * @param fontWeight font weight, optional by default value
   * ARKUI_FONT_WEIGHT_NORMAL
   * @param fontStyle font style, optional by default value
   * ARKUI_FONT_STYLE_NORMAL
   * @param fontFamily fonts, separated by commas, optional
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextFont(
      float fontSize,
      int32_t fontWeight = ARKUI_FONT_WEIGHT_NORMAL,
      int32_t fontStyle = ARKUI_FONT_STYLE_NORMAL,
      const std::string& fontFamily = std::string());
  /**
   * @brief set how the adaptive height is determined
   * @param policyType height adaptive policy type
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextHeightAdaptivePolicy(int32_t policyType);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_TypographySetIndents instead.
   * @brief set indentation of the first line
   * @param textIndent indentation of the first line
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextIndent(float textIndent);
  /**
   * @brief set ability to interact
   * @param enableFlag if text component can interact
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextEnable(bool enableFlag);
  /**
   * @brief set padding for all sides
   * @param left Left padding, in vp.
   * @param top Top padding, in vp.
   * @param right Right padding, in vp.
   * @param bottom Bottom padding, in vp.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setPadding(float top, float right, float bottom, float left);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTypographyTextWordBreakType instead.
   * @brief set line break rule
   * @param wordBreak line break rule
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setWordBreak(ArkUI_WordBreak wordBreak);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTypographyTextDirection instead.
   * @brief set direction of the main axis
   * @param direction direction of the main axis
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setWritingDirection(int32_t direction);
  /**
   * @note DO NOT USE THIS TOGETHER with
   * TextNode::setTextContentWithStyledString. Use
   * OH_Drawing_SetTypographyTextDirection instead.
   * @brief set background color of the selected text
   * @param color background color of the selected text, in 0xAARRGGBB format.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setSelectedBackgroundColor(uint32_t color);
  /**
   * @brief uses a formatted string object to set text content properties
   * @param styledString ArkUI_StyledString formatted string data
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextContentWithStyledString(ArkUI_StyledString* styledString);
  /**
   * @brief reset formatted string data
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& resetTextContentWithStyledString();
  /**
   * @brief set ability to do text recognition
   * @param enableFlag if text component do text recognition
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextDataDetectorEnable(int32_t enableFlag);
  /**
   * @brief set the text recognition configuration.
   * @param type text recognition configuration.
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextDataDetectorType(ArkUI_TextDataDetectorType type);
  /**
   * @deprecated use setTextDataDetectorEnable and setTextDataDetectorType
   * separately. Deprecated when preparing the 0.77 branch for release.
   * @brief set the text recognition.
   * @param enable if text component do text recognition
   * @param types text recognition configuration.
   * @param size number of enabled text recognition types
   * @return Reference to this node for method chaining
   * @throws std::runtime_error if operation failed
   */
  TextNode& setTextDataDetectorType(
      int32_t enable,
      const ArkUI_NumberValue types[],
      int size);
};

} // namespace rnoh