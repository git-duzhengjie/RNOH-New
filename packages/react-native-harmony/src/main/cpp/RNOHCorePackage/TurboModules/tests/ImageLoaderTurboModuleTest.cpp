/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ImageLoaderTurboModule.h>
#include <gtest/gtest.h>
#include <set>
#include <string>

using namespace rnoh;

// ============================================================================
// Tier 4b — NAPI Mock path (stub ArkTSTurboModule)
// Module: RNOHCorePackage/TurboModules/ImageLoaderTurboModule
//
// Tested class: ImageLoaderTurboModule (constructor populates methodMap_)
// Verifies that the codegen-generated method metadata is correctly
// registered: method name + arg count for each TurboModule method.
// ============================================================================

class ImageLoaderTurboModuleTest : public ::testing::Test {
 protected:
  void SetUp() override {
    module = std::make_unique<ImageLoaderTurboModule>(
        ArkTSTurboModule::Context{}, "ImageLoader");
  }
  std::unique_ptr<ImageLoaderTurboModule> module;
};

// ---------------------------------------------------------------------------
// methodMap_ size
// ---------------------------------------------------------------------------

TEST_F(ImageLoaderTurboModuleTest, Constructor_RegistersSevenMethods) {
  EXPECT_EQ(module->methodMap_.size(), 7u);
}

// ---------------------------------------------------------------------------
// Method names + arg counts
// ---------------------------------------------------------------------------

TEST_F(ImageLoaderTurboModuleTest, Constructor_AbortRequest_HasOneArg) {
  auto it = module->methodMap_.find("abortRequest");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 1u);
  EXPECT_EQ(it->first, std::string("abortRequest"));
}

TEST_F(ImageLoaderTurboModuleTest, Constructor_GetConstants_HasZeroArgs) {
  auto it = module->methodMap_.find("getConstants");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 0u);
  EXPECT_EQ(it->first, std::string("getConstants"));
}

TEST_F(ImageLoaderTurboModuleTest, Constructor_GetSize_HasOneArg) {
  auto it = module->methodMap_.find("getSize");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 1u);
  EXPECT_EQ(it->first, std::string("getSize"));
}

TEST_F(ImageLoaderTurboModuleTest, Constructor_GetSizeWithHeaders_HasTwoArgs) {
  auto it = module->methodMap_.find("getSizeWithHeaders");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 2u);
  EXPECT_EQ(it->first, std::string("getSizeWithHeaders"));
}

TEST_F(ImageLoaderTurboModuleTest, Constructor_PrefetchImage_HasTwoArgs) {
  auto it = module->methodMap_.find("prefetchImage");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 2u);
  EXPECT_EQ(it->first, std::string("prefetchImage"));
}

TEST_F(
    ImageLoaderTurboModuleTest,
    Constructor_PrefetchImageWithMetadata_HasThreeArgs) {
  auto it = module->methodMap_.find("prefetchImageWithMetadata");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 3u);
  EXPECT_EQ(it->first, std::string("prefetchImageWithMetadata"));
}

TEST_F(ImageLoaderTurboModuleTest, Constructor_QueryCache_HasOneArg) {
  auto it = module->methodMap_.find("queryCache");
  ASSERT_NE(it, module->methodMap_.end());
  EXPECT_EQ(it->second.argc, 1u);
  EXPECT_EQ(it->first, std::string("queryCache"));
}

// ---------------------------------------------------------------------------
// Non-existent methods
// ---------------------------------------------------------------------------

TEST_F(
    ImageLoaderTurboModuleTest,
    Constructor_DoesNotRegisterNonExistentMethod) {
  EXPECT_EQ(
      module->methodMap_.find("nonExistentMethod"), module->methodMap_.end());
}

TEST_F(
    ImageLoaderTurboModuleTest,
    Constructor_AllRegisteredMethodNames_MatchExpectedSet) {
  std::set<std::string> expectedNames = {
      "abortRequest",       "getConstants",
      "getSize",            "getSizeWithHeaders",
      "prefetchImage",      "prefetchImageWithMetadata",
      "queryCache"};
  std::set<std::string> actualNames;
  for (const auto& entry : module->methodMap_) {
    actualNames.insert(entry.first);
  }
  EXPECT_EQ(actualNames, expectedNames);
}
