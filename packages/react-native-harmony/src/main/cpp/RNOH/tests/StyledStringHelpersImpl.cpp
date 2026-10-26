/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Extracted from RNOH/StyledStringWrapper.cpp — keep in sync when the source
// changes. This file is compiled separately so that coverage lands here
// rather than in the test .cpp.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>
#include "react/renderer/attributedstring/primitives.h"

constexpr float DEFAULT_FONT_SIZE = 14.0f;

float normalizeFontSize(float fontSize) {
  if (fontSize > 0)
    return fontSize;
  return DEFAULT_FONT_SIZE;
}

size_t utf16Length(const std::string& str) {
  size_t len = 0;
  const unsigned char* currentByte =
      reinterpret_cast<const unsigned char*>(str.data());
  const unsigned char* endOfBytes = currentByte + str.size();
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
    while (continuationBytes-- && currentByte < endOfBytes &&
           ((*currentByte & 0xC0) == 0x80)) {
      codePoint = (codePoint << 6) | (*currentByte++ & 0x3F);
    }
    if (codePoint < 0x10000) {
      len += 1;
    } else {
      len += 2;
    }
  }
  return len;
}

std::string stringCapitalize(const std::string& strInput) {
  if (strInput.empty())
    return strInput;
  std::string strRes;
  std::string split = " ";
  std::vector<std::string> subStringVector;
  std::string strSrc = strInput + split;
  auto pos = strSrc.find(split);
  auto step = split.size();
  while (pos != std::string::npos) {
    subStringVector.push_back(strSrc.substr(0, pos));
    strSrc = strSrc.substr(pos + step);
    pos = strSrc.find(split);
  }
  for (auto subString : subStringVector) {
    if (std::isalpha(subString[0]) != 0) {
      std::transform(
          subString.begin(), subString.end(), subString.begin(), ::tolower);
      subString[0] = std::toupper(static_cast<unsigned char>(subString[0]));
    }
    if (!strRes.empty())
      strRes += split;
    strRes += subString;
  }
  return strRes;
}

void textCaseTransform(
    std::string& textContent,
    facebook::react::TextTransform type) {
  switch (type) {
    case facebook::react::TextTransform::Uppercase:
      std::transform(
          textContent.begin(),
          textContent.end(),
          textContent.begin(),
          ::toupper);
      break;
    case facebook::react::TextTransform::Lowercase:
      std::transform(
          textContent.begin(),
          textContent.end(),
          textContent.begin(),
          ::tolower);
      break;
    case facebook::react::TextTransform::Capitalize:
      textContent = stringCapitalize(textContent);
      break;
    default:
      break;
  }
}
