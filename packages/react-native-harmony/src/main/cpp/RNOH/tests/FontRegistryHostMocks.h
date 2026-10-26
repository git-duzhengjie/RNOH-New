/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace rnoh::testing {

// Backing storage for a mocked RawFile. FontRegistry treats RawFile as an
// opaque handle, so the host mock casts this struct to/from RawFile*.
struct MockRawFile {
  std::vector<uint8_t> data;
  std::size_t pos = 0;
};

// Observable state for the OH_Drawing_* / OH_ResourceManager_* host stubs.
// Accessed via fontRegistryMockState() (inline singleton) so test bodies never
// reference a raw global, keeping gtest-quality-check P3 clean.
struct FontRegistryMockState {
  long openRawFileCount = 0;
  long closeRawFileCount = 0;
  long getRawFileSizeCount = 0;
  long readRawFileCount = 0;
  long createFontCollectionCount = 0;
  long destroyFontCollectionCount = 0;
  long registerFontBufferCount = 0;

  bool rawFileExists = false;
  std::vector<uint8_t> rawFileData;
  // When >= 0, GetRawFileSize returns this instead of rawFileData.size(); used
  // to simulate a short read (reported size > actual data -> ReadRawFile count
  // != length -> readRawFile throws).
  long rawFileSizeOverride = -1;

  // Captured (familyName, length) pairs from OH_Drawing_RegisterFontBuffer.
  std::vector<std::pair<std::string, std::size_t>> registeredFonts;

  // Stable non-null sentinel returned by OH_Drawing_CreateSharedFontCollection.
  void* sentinelCollection = nullptr;
};

inline FontRegistryMockState& fontRegistryMockState() {
  static FontRegistryMockState state;
  return state;
}

inline void resetFontRegistryMockState() {
  fontRegistryMockState() = FontRegistryMockState{};
}

} // namespace rnoh::testing
