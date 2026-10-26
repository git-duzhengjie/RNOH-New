/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <native_drawing/drawing_font_collection.h>
#include <unordered_map>
#include <vector>
#include <array>
#include <rawfile/raw_file_manager.h>
#include <filesystem>
#include <mutex>
#include <string>
#include "ThreadGuard.h"

namespace rnoh {

/**
 * @internal
 * OH_Drawing_FontCollection can't be shared by multiple Typographies because
 * the app crashes.
 */
using SharedFontCollection = std::shared_ptr<OH_Drawing_FontCollection>;

/**
 * @internal
 * @thread: MAIN
 */
class FontRegistry {
  static constexpr std::array<const char*, 2> THEME_PATHS{
      "/data/themes/a/app",
      "/data/themes/b/app"};
  void addFontData(const std::string& name, std::vector<uint8_t> fontData);
  std::filesystem::path findValidThemeFontPath() const;
  bool isValidThemePath(const std::filesystem::path& path) const;
  bool isValidThemeFont(const std::filesystem::directory_entry& entry) const;

  std::weak_ptr<NativeResourceManager> m_weakResourceManager;
  std::unordered_map<std::string, std::vector<uint8_t>>
      m_fontFileContentByFontFamily;
  mutable std::mutex m_fontFileContentByFontFamilyMtx;
  ThreadGuard m_threadGuard;
  mutable std::mutex m_fontCollectionMtx;
  mutable SharedFontCollection m_fontCollection;
  mutable std::mutex m_themeFontFamilyNameMtx;
  std::string m_themeFontFamilyName;

 protected:
  // §6.4 test seam: returns the theme-font search roots. Default returns the
  // production THEME_PATHS so runtime behavior is unchanged; tests override to
  // inject a deterministic temporary directory (no device /data/themes needed).
  virtual std::vector<std::string> getThemeFontPaths() const {
    return {THEME_PATHS[0], THEME_PATHS[1]};
  }

 public:
  using Shared = std::shared_ptr<FontRegistry>;

  // Polymorphic (has the protected virtual seam above): a virtual destructor
  // makes deletion via a base pointer safe.
  virtual ~FontRegistry() = default;

  FontRegistry(std::weak_ptr<NativeResourceManager> weakResourceManager);

  void registerFont(const std::string& name, const std::string& fontFilePath);

  /**
   * @threadSafe
   */
  SharedFontCollection getFontCollection() const;

  /**
   * @threadSafe
   */
  std::string getThemeFontFamily() const;

  /**
   * @threadSafe
   */
  void updateThemeFont();
};
} // namespace rnoh
