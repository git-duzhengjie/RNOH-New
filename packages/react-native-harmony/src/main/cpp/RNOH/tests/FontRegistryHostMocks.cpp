/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "FontRegistryHostMocks.h"

#include <cstring>
#include <utility>

#ifndef __OHOS__
#define __availability__(...)
#endif
#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_register_font.h>

// Forward-declare the rawfile C API instead of including
// <rawfile/raw_file_manager.h>, which transitively pulls <napi/native_api.h>
// into the mock TU. The opaque struct tags and C-linkage symbol names match the
// SDK declarations that the production FontRegistry.cpp translation unit sees,
// so the host stubs satisfy the linker.
struct NativeResourceManager;
struct RawFile;

using rnoh::testing::fontRegistryMockState;
using rnoh::testing::MockRawFile;

extern "C" {

RawFile* OH_ResourceManager_OpenRawFile(
    const NativeResourceManager* mgr,
    const char* fileName) {
  auto& s = fontRegistryMockState();
  s.openRawFileCount++;
  if (mgr == nullptr || fileName == nullptr || !s.rawFileExists) {
    return nullptr;
  }
  auto* mf = new MockRawFile{};
  mf->data = s.rawFileData;
  return reinterpret_cast<RawFile*>(mf);
}

long OH_ResourceManager_GetRawFileSize(RawFile* rawFile) {
  auto& s = fontRegistryMockState();
  s.getRawFileSizeCount++;
  if (rawFile == nullptr) {
    return 0;
  }
  if (s.rawFileSizeOverride >= 0) {
    return s.rawFileSizeOverride;
  }
  return static_cast<long>(reinterpret_cast<MockRawFile*>(rawFile)->data.size());
}

int OH_ResourceManager_ReadRawFile(
    const RawFile* rawFile,
    void* buf,
    size_t length) {
  auto& s = fontRegistryMockState();
  s.readRawFileCount++;
  if (rawFile == nullptr || buf == nullptr) {
    return 0;
  }
  auto* mf = reinterpret_cast<MockRawFile*>(const_cast<RawFile*>(rawFile));
  std::size_t avail = mf->data.size() - mf->pos;
  std::size_t n = length < avail ? length : avail;
  std::memcpy(buf, mf->data.data() + mf->pos, n);
  mf->pos += n;
  return static_cast<int>(n);
}

void OH_ResourceManager_CloseRawFile(RawFile* rawFile) {
  auto& s = fontRegistryMockState();
  s.closeRawFileCount++;
  delete reinterpret_cast<MockRawFile*>(rawFile);
}

OH_Drawing_FontCollection* OH_Drawing_CreateSharedFontCollection(void) {
  auto& s = fontRegistryMockState();
  s.createFontCollectionCount++;
  if (s.sentinelCollection == nullptr) {
    s.sentinelCollection =
        reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEAD0001));
  }
  return reinterpret_cast<OH_Drawing_FontCollection*>(s.sentinelCollection);
}

void OH_Drawing_DestroyFontCollection(OH_Drawing_FontCollection* fontCollection) {
  fontRegistryMockState().destroyFontCollectionCount++;
}

uint32_t OH_Drawing_RegisterFontBuffer(
    OH_Drawing_FontCollection* fontCollection,
    const char* fontFamily,
    uint8_t* fontBuffer,
    size_t length) {
  auto& s = fontRegistryMockState();
  s.registerFontBufferCount++;
  if (fontFamily != nullptr) {
    s.registeredFonts.emplace_back(std::string(fontFamily), length);
  }
  return 0;
}

} // extern "C"
