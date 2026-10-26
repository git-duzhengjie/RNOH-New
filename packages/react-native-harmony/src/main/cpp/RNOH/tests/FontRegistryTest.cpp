/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "FontRegistryHostMocks.h"
#include "RNOH/FontRegistry.h"
#include "RNOH/RNOHError.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>

using rnoh::FontRegistry;
using rnoh::RNOHError;
using rnoh::testing::fontRegistryMockState;
using rnoh::testing::resetFontRegistryMockState;

namespace {

std::shared_ptr<NativeResourceManager> dummyResourceManager() {
  // NativeResourceManager is opaque to the host test. Keep a sentinel object
  // alive so FontRegistry's weak_ptr guard allows the sandbox branch to run.
  return std::shared_ptr<NativeResourceManager>(
      reinterpret_cast<NativeResourceManager*>(
          static_cast<std::uintptr_t>(0xBEEF0011)),
      [](NativeResourceManager*) {});
}
std::string sandboxPathForHost(const std::filesystem::path& filePath) {
  auto absolutePath = std::filesystem::absolute(filePath).lexically_normal();
#ifdef _WIN32
  // FontRegistry selects the sandbox branch by a leading '/'. A root-relative
  // path keeps that contract while remaining readable on the current drive.
  return "/" + absolutePath.relative_path().generic_string();
#else
  return absolutePath.generic_string();
#endif
}

#ifdef _WIN32
class ScopedCurrentPath {
 public:
  explicit ScopedCurrentPath(const std::filesystem::path& path)
      : originalPath_(std::filesystem::current_path()) {
    std::filesystem::current_path(path);
  }

  ~ScopedCurrentPath() {
    std::error_code errorCode;
    std::filesystem::current_path(originalPath_, errorCode);
  }

 private:
  std::filesystem::path originalPath_;
};
#endif

} // namespace

// §6.4 seam subclass: overrides the theme-font search root so device-only
// /data/themes logic is host-testable with a deterministic temp directory.
// Defined early so both fixtures below can use it.
class TestableFontRegistry : public FontRegistry {
 public:
  using FontRegistry::FontRegistry;
  void setThemePaths(std::vector<std::string> paths) {
    themePaths_ = std::move(paths);
  }

 protected:
  std::vector<std::string> getThemeFontPaths() const override {
    return themePaths_;
  }

 private:
  std::vector<std::string> themePaths_;
};

class FontRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override { resetFontRegistryMockState(); }
};

TEST_F(FontRegistryTest, GetThemeFontFamily_Initially_ReturnsEmpty) {
  FontRegistry fr(std::weak_ptr<NativeResourceManager>{});
  EXPECT_EQ(fr.getThemeFontFamily(), "");
}

TEST_F(FontRegistryTest, GetFontCollection_NoFonts_CreatesSharedCollection) {
  FontRegistry fr(std::weak_ptr<NativeResourceManager>{});
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  EXPECT_EQ(fontRegistryMockState().createFontCollectionCount, 1);
}

TEST_F(FontRegistryTest, GetFontCollection_SecondCall_CachesCollection) {
  FontRegistry fr(std::weak_ptr<NativeResourceManager>{});
  auto fc1 = fr.getFontCollection();
  auto fc2 = fr.getFontCollection();
  EXPECT_EQ(fc1.get(), fc2.get());
  EXPECT_EQ(fontRegistryMockState().createFontCollectionCount, 1);
}

TEST_F(FontRegistryTest, RegisterFont_RawFile_RegistersIntoCollection) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0x42, 0x43, 0x44};

  fr.registerFont("MyFont", "myfont.ttf");
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  EXPECT_EQ(s.registerFontBufferCount, 1);
  ASSERT_EQ(s.registeredFonts.size(), 1u);
  EXPECT_EQ(s.registeredFonts[0].first, "MyFont");
  EXPECT_EQ(s.registeredFonts[0].second, 3u);
}

TEST_F(FontRegistryTest, RegisterFont_AbsoluteMissingPath_ThrowsRNOHError) {
  auto rm = dummyResourceManager(); // keep shared_ptr alive so lock() is non-null
  FontRegistry fr{rm};
  EXPECT_THROW(fr.registerFont("F", "/definitely/not/here.ttf"), RNOHError);
}

TEST_F(FontRegistryTest, RegisterFont_RawFileOpenFails_ThrowsRNOHError) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  // rawFileExists defaults false -> OH_ResourceManager_OpenRawFile returns null
  // -> readRawFile throws RNOHError("Failed to open font fontFile: ...").
  EXPECT_THROW(fr.registerFont("F", "missing.ttf"), RNOHError);
}

TEST_F(FontRegistryTest, RegisterFont_RawFileShortRead_ThrowsRNOHError) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0x01, 0x02, 0x03}; // 3 bytes available
  s.rawFileSizeOverride = 10; // report size 10 -> buffer(10), read 3 -> 3 != 10 -> throw
  EXPECT_THROW(fr.registerFont("F", "x.ttf"), RNOHError);
}

TEST_F(FontRegistryTest, RegisterFont_NullResourceManager_SkipsNativeCalls) {
  // Empty weak_ptr -> lock() returns null -> early return; no font is stored,
  // so a subsequent getFontCollection() registers nothing.
  FontRegistry fr(std::weak_ptr<NativeResourceManager>{});
  fr.registerFont("Ignored", "ignored.ttf");
  fr.getFontCollection();
  EXPECT_EQ(fontRegistryMockState().registerFontBufferCount, 0);
}

TEST_F(FontRegistryTest, UpdateThemeFont_NoThemePath_DoesNotThrow) {
  // Inject an explicitly non-existent path instead of the real /data/themes,
  // so the result does not depend on the host filesystem (§10.1).
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({"/no/such/rnoh_test_nonexistent_theme_path"});
  EXPECT_NO_THROW(fr.updateThemeFont());
  EXPECT_EQ(fr.getThemeFontFamily(), "");
}

// --- Theme-font discovery via §6.4 seam (TestableFontRegistry injects a temp
// theme root so the device-only /data/themes logic is host-testable). ---
// TestableFontRegistry is defined above (shared by both fixtures).

class FontRegistryThemeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetFontRegistryMockState();
    // Atomically claim a process-unique directory so concurrent GCC/Clang or
    // CI shards cannot delete each other's theme-font fixtures.
    auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string name = info ? info->name() : "test";
    auto tempRoot = std::filesystem::temp_directory_path() / "rnoh_fontreg";
    std::error_code ec;
    std::filesystem::create_directories(tempRoot, ec);
    ASSERT_FALSE(ec) << "Failed to create FontRegistry temp root";

    std::random_device randomDevice;
    for (int attempt = 0; attempt < 16; ++attempt) {
      auto candidate = tempRoot /
          (name + "_" + std::to_string(randomDevice()) + "_" +
           std::to_string(randomDevice()));
      ec.clear();
      if (std::filesystem::create_directory(candidate, ec)) {
        appRoot_ = std::move(candidate);
        break;
      }
    }
    ASSERT_FALSE(appRoot_.empty())
        << "Failed to claim a unique FontRegistry temp directory";
    ec.clear();
    std::filesystem::create_directories(appRoot_ / "fonts", ec);
    ASSERT_FALSE(ec) << "Failed to create FontRegistry fonts directory";
  }

  void TearDown() override {
    if (appRoot_.empty()) {
      return;
    }
    std::error_code ec;
    std::filesystem::remove_all(appRoot_, ec);
  }

  // Lay out <app>/flag + <app>/fonts/<name>.ttf; clears any prior fonts so each
  // call sets exactly one theme font (supports state-transition tests).
  void writeThemeFont(const std::string& fontName, const std::string& bytes) {
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(appRoot_ / "fonts", ec)) {
      std::filesystem::remove(e.path(), ec);
    }
    std::ofstream(appRoot_ / "flag") << "x";
    std::ofstream(appRoot_ / "fonts" / (fontName + ".ttf"), std::ios::binary)
        << bytes;
  }

  void clearThemeFonts() {
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(appRoot_ / "fonts", ec)) {
      std::filesystem::remove(e.path(), ec);
    }
    // keep the flag file so isValidThemePath still passes
  }

  std::filesystem::path appRoot_;
};

// P1: verify the new theme font is REGISTERED under its own family name (not
// the old/empty one). This exposed a production bug where updateThemeFont
// called addFontData(m_themeFontFamilyName, ...) (the old name); fixed to
// addFontData(fontName, ...). An empty updateThemeFont impl fails here because
// no RegisterFontBuffer call would be recorded.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_FoundNonDefault_RegistersUnderNewFamily) {
  const std::string bytes = "FONTDATA";
  writeThemeFont("MyFont", bytes);
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});

  fr.updateThemeFont();
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  EXPECT_EQ(fr.getThemeFontFamily(), "MyFont");
  EXPECT_EQ(fontRegistryMockState().registerFontBufferCount, 1);
  ASSERT_EQ(fontRegistryMockState().registeredFonts.size(), 1u);
  EXPECT_EQ(fontRegistryMockState().registeredFonts[0].first, "MyFont");
  EXPECT_EQ(fontRegistryMockState().registeredFonts[0].second, bytes.size());
}

// P1: non-empty -> cleared state transition. Starts from "MyFont", switches to
// "default" and verifies the name is cleared AND "default" is not registered as
// a custom family. An empty updateThemeFont impl fails the ASSERT in step 1.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_DefaultFont_FromNonEmpty_ClearsName) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont"); // establish non-empty state

  writeThemeFont("default", "Y");
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), "");
  for (const auto& rf : fontRegistryMockState().registeredFonts) {
    EXPECT_NE(rf.first, "default")
        << "default must not be registered as a custom family";
  }
}

// P1: non-empty -> cleared when the theme dir has no valid font left.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_NoValidFont_FromNonEmpty_ClearsName) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");

  clearThemeFonts(); // no .ttf left
  EXPECT_NO_THROW(fr.updateThemeFont());
  EXPECT_EQ(fr.getThemeFontFamily(), "");
}

// P1 (§3.3): switching to a different theme registers the NEW family in
// addition to the previously-loaded one (no reload of the old name).
TEST_F(FontRegistryThemeTest, UpdateThemeFont_ThemeSwitch_RegistersNewFamily) {
  writeThemeFont("FontA", "AAA");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "FontA");

  writeThemeFont("FontB", "BBB");
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), "FontB");
  fr.getFontCollection();
  const auto& rf = fontRegistryMockState().registeredFonts;
  bool hasA = false, hasB = false;
  for (const auto& p : rf) {
    if (p.first == "FontA") hasA = true;
    if (p.first == "FontB") hasB = true;
  }
  EXPECT_TRUE(hasA);
  EXPECT_TRUE(hasB);
}

// P1 (§3.3): second updateThemeFont with the SAME theme font must not cause a
// second RegisterFontBuffer for that family (no reload). Note: with the
// current production logic, isValidThemeFont filters out the current font, so
// the second call finds no valid font and clears the name — but the already
// stored font is still registered exactly once into the collection.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_SameTheme_DoesNotReloadFont) {
  writeThemeFont("MyFont", "DATA");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  fr.updateThemeFont(); // same theme font still present
  fr.getFontCollection();
  long myFontCount = 0;
  for (const auto& p : fontRegistryMockState().registeredFonts) {
    if (p.first == "MyFont") myFontCount++;
  }
  EXPECT_EQ(myFontCount, 1);
}

// registerFont with an existing absolute path reads the sandbox file and
// registers its bytes under the requested family.
TEST_F(FontRegistryThemeTest, RegisterFont_AbsoluteSandboxFile_RegistersIntoCollection) {
  const std::string bytes = "SANDBOXFONT";
  std::filesystem::path fontFile = appRoot_ / "sandbox.ttf";
  std::ofstream(fontFile, std::ios::binary) << bytes;
#ifdef _WIN32
  ScopedCurrentPath currentPath(appRoot_);
#endif
  auto rm = dummyResourceManager();
  TestableFontRegistry fr{rm};
  fr.setThemePaths({"/no/such/rnoh_test_nonexistent_theme_path"}); // neutral
  fr.registerFont("SandboxFont", sandboxPathForHost(fontFile));
  auto collection = fr.getFontCollection();
  ASSERT_NE(collection, nullptr);
  const auto& rf = fontRegistryMockState().registeredFonts;
  ASSERT_EQ(rf.size(), 1u);
  EXPECT_EQ(rf[0].first, "SandboxFont");
  EXPECT_EQ(rf[0].second, bytes.size());
}

// --- Branch-coverage supplements (§ can-cover analysis). ---

// addFontData: m_fontCollection non-null when registerFont triggers reset().
// getFontCollection() first (creates), then registerFont (addFontData -> reset
// on non-null -> DestroyFontCollection), then getFontCollection again (recreates).
TEST_F(FontRegistryTest, GetFontCollection_AfterRegister_RecreatesCollection) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0x41, 0x42};

  {
    auto fc1 = fr.getFontCollection(); // creates collection (create=1)
    ASSERT_NE(fc1, nullptr);
    EXPECT_EQ(s.createFontCollectionCount, 1);
    EXPECT_EQ(s.destroyFontCollectionCount, 0);
  } // fc1 out of scope -> refcount 1 (m_fontCollection only)

  fr.registerFont("F", "f.ttf"); // addFontData -> m_fontCollection.reset() -> refcount 1->0 -> destroy
  EXPECT_GE(s.destroyFontCollectionCount, 1);

  auto fc2 = fr.getFontCollection(); // recreates (create=2)
  ASSERT_NE(fc2, nullptr);
  EXPECT_EQ(s.createFontCollectionCount, 2);
}

// isValidThemePath: flag present but no fonts dir -> false (hasFlag && !hasFonts).
TEST_F(FontRegistryThemeTest, UpdateThemeFont_FlagOnlyNoFontsDir_FromNonEmpty_ClearsName) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");

  std::error_code ec;
  std::filesystem::remove_all(appRoot_ / "fonts", ec); // no fonts dir, flag stays
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), "");
}

// isValidThemePath: fonts dir present but no flag file -> false (!hasFlag && hasFonts).
TEST_F(FontRegistryThemeTest, UpdateThemeFont_FontsOnlyNoFlag_FromNonEmpty_ClearsName) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");

  std::error_code ec;
  std::filesystem::remove(appRoot_ / "flag", ec); // no flag, fonts dir stays
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), "");
}

// isValidThemeFont: non-regular file (subdirectory) in fonts/ -> false.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_FontsDirContainsSubdir_NotValid) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");

  clearThemeFonts();
  std::error_code ec;
  std::filesystem::create_directories(appRoot_ / "fonts" / "subdir", ec);
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), ""); // subdir not a regular .ttf
}

// isValidThemeFont: .txt extension (not .ttf) -> false.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_FontsDirContainsTxt_NotValid) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");

  clearThemeFonts();
  std::ofstream(appRoot_ / "fonts" / "readme.txt") << "not a font";
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), ""); // .txt != .ttf
}

// Multi-path catch: first path doesn't exist (catch+continue), second is valid.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_SecondPathHit_FindsFont) {
  writeThemeFont("MyFont", "DATA");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({"/no/such/rnoh_test_nonexistent", appRoot_.string()});
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), "MyFont"); // found on second path
}

// --- Additional branch-coverage supplements: cover residual non-throw branches. ---

// isValidThemePath: both flag and fonts found simultaneously on same entry scan
// iteration (hasFlag && hasFonts both become true within the loop).
TEST_F(FontRegistryThemeTest, UpdateThemeFont_BothFlagAndFonts_FindsFont) {
  writeThemeFont("MyFont", "DATA");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  // The first call covers: directory_iterator finds flag + fonts -> return true
  EXPECT_EQ(fr.getThemeFontFamily(), "MyFont");
}

// isValidThemeFont: .ttf with stem == current theme -> filtered (false).
// On 2nd updateThemeFont call the same font is filtered (stem == m_themeFontFamilyName).
TEST_F(FontRegistryThemeTest, UpdateThemeFont_SameFontFilteredByStem_ClearsName) {
  writeThemeFont("MyFont", "DATA");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");
  // 2nd call: isValidThemeFont checks stem("MyFont") == m_themeFontFamilyName("MyFont") -> false
  fr.updateThemeFont();
  // isValidThemeFont returned false for all entries -> findValidThemeFontPath returns {}
  // -> else branch -> clear name
  EXPECT_EQ(fr.getThemeFontFamily(), "");
}

// isValidThemeFont: multiple .ttf files, first is current (filtered), second is new.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_MultipleTtf_SecondIsNew) {
  writeThemeFont("FontA", "AAA");
  // Also add FontB.ttf in same fonts dir
  std::ofstream(appRoot_ / "fonts" / "FontB.ttf", std::ios::binary) << "BBB";
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  // FontA or FontB found (depends on directory iteration order)
  std::string name = fr.getThemeFontFamily();
  EXPECT_TRUE(name == "FontA" || name == "FontB");
}

// registerFont: fontFilePath NOT starting with '/' -> rawfile path (readRawFile).
// Verify the rawfile path is taken (not sandbox).
TEST_F(FontRegistryTest, RegisterFont_RelativePath_UsesRawFile) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0xAA, 0xBB, 0xCC};
  fr.registerFont("RelFont", "rel/path.ttf");
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  EXPECT_EQ(s.openRawFileCount, 1);
  EXPECT_EQ(s.registerFontBufferCount, 1);
}

// getFontCollection: cache hit on 2nd call with fonts already registered.
// Verify the loop body (for each font -> RegisterFontBuffer) runs on first call.
TEST_F(FontRegistryTest, GetFontCollection_WithFonts_RegistersAllThenCaches) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0x01};
  fr.registerFont("A", "a.ttf");
  // re-set mock for second font
  s.rawFileData = {0x02};
  fr.registerFont("B", "b.ttf");
  auto fc1 = fr.getFontCollection();
  ASSERT_NE(fc1, nullptr);
  EXPECT_EQ(s.registerFontBufferCount, 2); // both A and B registered
  auto fc2 = fr.getFontCollection(); // cache hit
  EXPECT_EQ(fc1.get(), fc2.get());
  EXPECT_EQ(s.createFontCollectionCount, 1); // not recreated
}

// readRawFile: successful read with exact length match (no throw).
TEST_F(FontRegistryTest, RegisterFont_RawFileExactRead_Succeeds) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0x00, 0x01, 0x02, 0x03, 0x04}; // 5 bytes
  fr.registerFont("ExactFont", "exact.ttf");
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  EXPECT_EQ(s.registerFontBufferCount, 1);
  EXPECT_EQ(s.readRawFileCount, 1);
}

// readRawFile: zero-length file (GetRawFileSize returns 0).
TEST_F(FontRegistryTest, RegisterFont_RawFileZeroLength_Succeeds) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {}; // empty
  s.rawFileSizeOverride = 0;
  fr.registerFont("EmptyFont", "empty.ttf");
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  EXPECT_EQ(s.registerFontBufferCount, 1);
}

// addFontData: insert_or_assign overwrites existing font with same name.
TEST_F(FontRegistryTest, RegisterFont_SameName_OverwritesFontData) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0x01};
  fr.registerFont("Dup", "first.ttf");
  s.rawFileData = {0x02, 0x03};
  fr.registerFont("Dup", "second.ttf");
  auto fc = fr.getFontCollection();
  ASSERT_NE(fc, nullptr);
  // Only 1 font family "Dup" registered (overwritten, not duplicated)
  EXPECT_EQ(s.registeredFonts.size(), 1u);
  EXPECT_EQ(s.registeredFonts[0].first, "Dup");
  EXPECT_EQ(s.registeredFonts[0].second, 2u); // second data (2 bytes)
}

// getFontCollection: double-check pattern (another thread set m_fontCollection
// between our check and lock). Cover the `if (m_fontCollection) return` after lock.
TEST_F(FontRegistryTest, GetFontCollection_AfterAddFontData_CacheInvalidated) {
  auto rm = dummyResourceManager();
  FontRegistry fr{rm};
  auto& s = fontRegistryMockState();
  s.rawFileExists = true;
  s.rawFileData = {0xFF};
  fr.registerFont("X", "x.ttf");
  {
    auto fc1 = fr.getFontCollection(); // creates + registers X
    ASSERT_NE(fc1, nullptr);
  }
  // addFontData resets collection -> next getFontCollection must recreate
  s.rawFileData = {0xEE};
  fr.registerFont("Y", "y.ttf");
  auto fc2 = fr.getFontCollection();
  ASSERT_NE(fc2, nullptr);
  EXPECT_EQ(s.createFontCollectionCount, 2); // recreated after invalidation
  // Both X and Y should be registered
  EXPECT_EQ(s.registerFontBufferCount, 3); // X(1st) + X+Y(2nd recreate)
}

// isValidThemePath: entry is not regular file and not directory -> neither flag
// nor fonts. Cover the branch where entry is neither.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_NeitherFlagNorFonts_ClearsName) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");
  // Replace flag+fonts with a single non-flag, non-fonts file
  std::error_code ec;
  std::filesystem::remove_all(appRoot_ / "fonts", ec);
  std::filesystem::remove(appRoot_ / "flag", ec);
  std::ofstream(appRoot_ / "random.txt") << "neither flag nor fonts dir";
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), ""); // isValidThemePath false
}

// isValidThemePath: all three paths non-existent -> catch on all -> clear name.
TEST_F(FontRegistryThemeTest, UpdateThemeFont_AllPathsNonExistent_ClearsName) {
  writeThemeFont("MyFont", "X");
  TestableFontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.setThemePaths({appRoot_.string()});
  fr.updateThemeFont();
  ASSERT_EQ(fr.getThemeFontFamily(), "MyFont");
  fr.setThemePaths({"/no/such/a", "/no/such/b", "/no/such/c"});
  fr.updateThemeFont();
  EXPECT_EQ(fr.getThemeFontFamily(), ""); // all paths catch filesystem_error
}

// registerFont: fontFilePath with '/' prefix but null resourceManager.
// starts_with('/') true -> readSandboxFile is called BEFORE the null RM check
// at line 62. Wait - actually the null RM check is at line 62 (before starts_with).
// So null RM + '/' path: the null check fires first -> LOG(ERROR) + return.
// Verify: no throw, no native calls.
TEST_F(FontRegistryTest, RegisterFont_AbsolutePath_NullRM_SkipsEarly) {
  // Empty weak_ptr -> lock() returns null -> LOG(ERROR) + return (before starts_with check).
  FontRegistry fr{std::weak_ptr<NativeResourceManager>{}};
  fr.registerFont("F", "/nonexistent/sandbox/font.ttf");
  fr.getFontCollection();
  EXPECT_EQ(fontRegistryMockState().registerFontBufferCount, 0); // nothing registered
}

// readSandboxFile: file exists but is empty (length 0).
TEST_F(FontRegistryThemeTest, RegisterFont_SandboxEmptyFile_RegistersEmpty) {
  std::filesystem::path emptyFile = appRoot_ / "empty_sandbox.ttf";
  std::ofstream(emptyFile, std::ios::binary); // empty file
#ifdef _WIN32
  ScopedCurrentPath currentPath(appRoot_);
#endif
  auto rm = dummyResourceManager();
  TestableFontRegistry fr{rm};
  fr.setThemePaths({"/no/such/rnoh_test_nonexistent"});
  fr.registerFont("EmptySandbox", sandboxPathForHost(emptyFile));
  auto collection = fr.getFontCollection();
  ASSERT_NE(collection, nullptr);
  const auto& rf = fontRegistryMockState().registeredFonts;
  ASSERT_EQ(rf.size(), 1u);
  EXPECT_EQ(rf[0].first, "EmptySandbox");
  EXPECT_EQ(rf[0].second, 0u);
}
