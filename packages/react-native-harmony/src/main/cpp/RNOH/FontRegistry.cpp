/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "FontRegistry.h"
#include <glog/logging.h>
#include <native_drawing/drawing_register_font.h>
#include <rawfile/raw_file_manager.h>
#include <fstream>
#include <mutex>
#include <string_view>
#include "RNOHError.h"

using namespace rnoh;

std::vector<uint8_t> readRawFile(
    NativeResourceManager* resourceManager,
    std::string const& rawfileRelativeFontFilePath) {
  auto fontFile =
      std::unique_ptr<RawFile, decltype(&OH_ResourceManager_CloseRawFile)>(
          OH_ResourceManager_OpenRawFile(
              resourceManager, rawfileRelativeFontFilePath.c_str()),
          OH_ResourceManager_CloseRawFile);
  if (!fontFile) {
    throw RNOHError(
        "Failed to open font fontFile: " + rawfileRelativeFontFilePath);
  }
  auto length = OH_ResourceManager_GetRawFileSize(fontFile.get());
  std::vector<uint8_t> buffer(length);
  if (OH_ResourceManager_ReadRawFile(fontFile.get(), buffer.data(), length) !=
      length) {
    throw RNOHError("Failed to read font file: " + rawfileRelativeFontFilePath);
  }
  return buffer;
}

std::vector<uint8_t> readSandboxFile(std::string const& absoluteFontFilePath) {
  std::ifstream fontFile(absoluteFontFilePath, std::ifstream::binary);
  if (!fontFile.is_open()) {
    throw RNOHError("Failed to read font file: " + absoluteFontFilePath);
  }
  fontFile.seekg(0, std::ios::end);
  auto length = fontFile.tellg();
  fontFile.seekg(0, std::ios::beg);
  std::vector<uint8_t> buffer(length);
  fontFile.read(reinterpret_cast<char*>(buffer.data()), length);
  return buffer;
}

FontRegistry::FontRegistry(
    std::weak_ptr<NativeResourceManager> weakResourceManager)
    : m_weakResourceManager(std::move(weakResourceManager)) {}

void FontRegistry::registerFont(
    const std::string& name,
    const std::string& fontFilePath) {
  m_threadGuard.assertThread();
  auto resourceManager = m_weakResourceManager.lock();
  if (resourceManager == nullptr) {
    LOG(ERROR) << "Couldn't register font " << name
               << " - resourceManager is nullptr";
    return;
  };
  auto fontData = fontFilePath.starts_with('/')
      ? readSandboxFile(fontFilePath)
      : readRawFile(resourceManager.get(), fontFilePath);
  addFontData(name, std::move(fontData));
}

void FontRegistry::addFontData(
    const std::string& name,
    std::vector<uint8_t> fontData) {
  m_threadGuard.assertThread();
  std::scoped_lock lock(m_fontFileContentByFontFamilyMtx, m_fontCollectionMtx);
  m_fontFileContentByFontFamily.insert_or_assign(name, std::move(fontData));
  // NOTE: fonts cannot be added to an existing collection, so we need to
  // recreate it the next time `getFontCollection` is called
  m_fontCollection.reset();
}

SharedFontCollection FontRegistry::getFontCollection() const {
  {
    auto lockFontCollection = std::lock_guard(m_fontCollectionMtx);
    if (m_fontCollection) {
      return m_fontCollection;
    }
  }
  SharedFontCollection fontCollection(
      OH_Drawing_CreateSharedFontCollection(),
      OH_Drawing_DestroyFontCollection);
  std::scoped_lock lock(m_fontCollectionMtx, m_fontFileContentByFontFamilyMtx);
  if (m_fontCollection) {
    return m_fontCollection;
  }
  for (auto& [name, fileContent] : m_fontFileContentByFontFamily) {
    OH_Drawing_RegisterFontBuffer(
        fontCollection.get(),
        name.c_str(),
        (uint8_t*)fileContent.data(),
        fileContent.size());
  }
  m_fontCollection = fontCollection;
  return fontCollection;
}

std::string FontRegistry::getThemeFontFamily() const {
  std::lock_guard<std::mutex> lock(m_themeFontFamilyNameMtx);
  return m_themeFontFamilyName;
}

void FontRegistry::updateThemeFont() {
  std::lock_guard lock(m_themeFontFamilyNameMtx);

  if (auto fontPath = findValidThemeFontPath(); !fontPath.empty()) {
    std::filesystem::path path(fontPath);
    const auto fontName = path.stem().string();

    if (fontName == "default") {
      m_themeFontFamilyName.clear();
      return;
    }

    if (fontName != m_themeFontFamilyName) {
      addFontData(fontName, readSandboxFile(fontPath.string()));
    }
    m_themeFontFamilyName = fontName;
  } else {
    m_themeFontFamilyName.clear();
  }
}

std::filesystem::path FontRegistry::findValidThemeFontPath() const {
  for (const auto& basePath : getThemeFontPaths()) {
    try {
      std::filesystem::path themePath(basePath);
      if (!isValidThemePath(themePath))
        continue;

      auto fontDir = themePath / "fonts";
      for (const auto& entry : std::filesystem::directory_iterator(fontDir)) {
        if (isValidThemeFont(entry)) {
          return entry;
        }
      }
    } catch (const std::filesystem::filesystem_error& e) {
      LOG(WARNING) << "Theme scan error: " << e.what();
    }
  }
  return {};
}

bool FontRegistry::isValidThemePath(const std::filesystem::path& path) const {
  bool hasFlag = false;
  bool hasFonts = false;

  try {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      if (entry.is_regular_file() && entry.path().filename() == "flag") {
        hasFlag = true;
      } else if (entry.is_directory() && entry.path().filename() == "fonts") {
        hasFonts = true;
      }

      if (hasFlag && hasFonts)
        return true;
    }
  } catch (const std::filesystem::filesystem_error&) {
    return false;
  }
  return false;
}

bool FontRegistry::isValidThemeFont(
    const std::filesystem::directory_entry& entry) const {
  return entry.is_regular_file() && (entry.path().extension() == ".ttf") &&
      entry.path().stem().string() != m_themeFontFamilyName;
}
