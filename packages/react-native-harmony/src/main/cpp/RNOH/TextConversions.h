/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

//
// Created on 2024/3/19.
//
// Node APIs are not fully supported. To solve the compilation error of the
// interface cannot be found, please include "napi/native_api.h".

#ifndef HARMONY_TEXTCONVERSIONS_H
#define HARMONY_TEXTCONVERSIONS_H

#include <arkui/native_type.h>
#include "react/renderer/attributedstring/AttributedString.h"
#include "react/renderer/attributedstring/primitives.h"
#include "react/renderer/components/text/ParagraphProps.h"
#include "react/renderer/core/ShadowNode.h"

namespace rnoh {
constexpr static float DEFAULT_FONT_SIZE = 14.0f;
constexpr static float EPSILON_THRESHOLD = 1e-6;

struct TextPaddingInfo {
  float top = 0.0;
  float bottom = 0.0;
  float left = 0.0;
  float right = 0.0;
};

// attachments' layout metrics are not ready yet
// terminate pre-measuring if attachment exists
class PreMeasureTerminated : public std::exception {};

class TextConversions {
 public:
  static int32_t getArkUIFontWeight(int32_t fontWeight);
  static int32_t getArkUITextCase(
      const facebook::react::TextTransform& textTransform);
  static int32_t getArkUIEllipsizeMode(
      const facebook::react::EllipsizeMode& ellipsizeMode);
  static int32_t getArkUITextAlignment(
      const facebook::react::TextAlignment& textAlign);
  static TextPaddingInfo getArkUIPadding(
      std::shared_ptr<const facebook::react::ParagraphProps> props);
  static std::optional<float> getMinFontSize(
      std::shared_ptr<const facebook::react::ParagraphProps> props);
  static std::optional<float> getMaxFontSize(
      std::shared_ptr<const facebook::react::ParagraphProps> props);
  static ArkUI_Alignment getAlignment(
      const facebook::react::TextAlignment& alignHorizon,
      const std::string& alignVertical);
  static ArkUI_WordBreak getArkUIWordBreakStrategy(
      facebook::react::TextBreakStrategy textBreakStrategy);
  static bool getArkUITextDataDetectorEnable(folly::dynamic dataDetectorType);
  static std::optional<ArkUI_TextDataDetectorType>
  getArkUITextDataDetectorTypes(folly::dynamic dataDetectorType);
  static void getAttributedStringFragmentsWithoutFrames(
      const facebook::react::ShadowNode& shadowNode,
      const facebook::react::TextAttributes& baseTextAttributes,
      const facebook::react::ShadowNode& parentShadowNode,
      facebook::react::AttributedString::Fragments& outFragments);
};
} // namespace rnoh
#endif // HARMONY_TEXTCONVERSIONS_H
