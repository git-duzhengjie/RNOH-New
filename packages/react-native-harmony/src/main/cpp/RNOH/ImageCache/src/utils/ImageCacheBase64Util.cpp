/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ImageCacheBase64Util.h"

namespace ImageCachePro {

namespace {

int DecodeBase64Char(char ch) {
  if (ch >= 'A' && ch <= 'Z') {
    return ch - 'A';
  }
  if (ch >= 'a' && ch <= 'z') {
    return ch - 'a' + 26;
  }
  if (ch >= '0' && ch <= '9') {
    return ch - '0' + 52;
  }
  if (ch == '+') {
    return 62;
  }
  if (ch == '/') {
    return 63;
  }
  return -1;
}

} // namespace

std::vector<uint8_t> ImageCacheBase64Util::Decode(const std::string& encoded) {
  std::vector<uint8_t> decoded;
  if (encoded.empty()) {
    return decoded;
  }

  decoded.reserve(encoded.size() * 3 / 4);

  uint32_t accumulator = 0;
  int bits = 0;
  for (char ch : encoded) {
    if (ch == '=' || ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
      continue;
    }
    int value = DecodeBase64Char(ch);
    if (value < 0) {
      return {};
    }
    accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      decoded.push_back(static_cast<uint8_t>((accumulator >> bits) & 0xFF));
    }
  }

  return decoded;
}

} // namespace ImageCachePro
