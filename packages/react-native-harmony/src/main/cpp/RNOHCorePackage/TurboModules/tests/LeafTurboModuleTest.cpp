/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <ReactCommon/CallInvoker.h>
#include "RNOH/ArkTSTurboModule.h"

#include "RNOHCorePackage/TurboModules/AppStateTurboModule.h"
#include "RNOHCorePackage/TurboModules/AccessibilityInfoTurboModule.h"
#include "RNOHCorePackage/TurboModules/AlertManagerTurboModule.h"
#include "RNOHCorePackage/TurboModules/ShareTurboModule.h"
#include "RNOHCorePackage/TurboModules/LinkingManagerTurboModule.h"

using namespace rnoh;

class FakeCallInvoker : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc&& func) noexcept override {
    func(*(facebook::jsi::Runtime*)nullptr);
  }
  void invokeSync(facebook::react::CallFunc&& func) override {
    func(*(facebook::jsi::Runtime*)nullptr);
  }
};

template <typename T>
class MethodMapAccessor : public T {
 public:
  using T::T;
  size_t getMethodCount() const { return this->methodMap_.size(); }
  bool hasMethod(const std::string& name) const {
    return this->methodMap_.find(name) != this->methodMap_.end();
  }
  size_t getMethodArgCount(const std::string& name) const {
    auto it = this->methodMap_.find(name);
    if (it == this->methodMap_.end()) return static_cast<size_t>(-1);
    return it->second.argCount;
  }
  const std::string& getName() const { return this->name_; }
};

struct MethodEntry { std::string name; size_t argCount; };

class LeafTurboModuleTest : public ::testing::Test {
 protected:
  std::shared_ptr<FakeCallInvoker> jsInvoker =
      std::make_shared<FakeCallInvoker>();

  ArkTSTurboModule::Context makeCtx() {
    return ArkTSTurboModule::Context{
        {.jsInvoker = jsInvoker, .instance = {}, .safeInstance = {},
         .arkTSMessageHub = nullptr},
        .env = nullptr, .arkTSTurboModuleInstanceRef = NapiRef{},
        .turboModuleThread = TaskThread::JS, .taskExecutor = nullptr,
        .eventDispatcher = nullptr, .jsQueue = nullptr, .scheduler = nullptr,
        .displayMetricsManager = nullptr, .featureFlagRegistry = nullptr};
  }

#define VERIFY_METHODS(mod_ptr, ...) \
  do { \
    auto expected = std::vector<MethodEntry>{__VA_ARGS__}; \
    EXPECT_EQ((mod_ptr)->getMethodCount(), expected.size()); \
    for (const auto& e : expected) { \
      EXPECT_TRUE((mod_ptr)->hasMethod(e.name)) << "Missing method: " << e.name; \
      if ((mod_ptr)->hasMethod(e.name)) { \
        EXPECT_EQ((mod_ptr)->getMethodArgCount(e.name), e.argCount) \
            << "Wrong argCount for: " << e.name; \
      } \
    } \
  } while (0)
};

// ===== AppStateTurboModule =====

TEST_F(LeafTurboModuleTest, AppState_RegistersTwoMethods) {
  auto mod = std::make_unique<MethodMapAccessor<AppStateTurboModule>>(makeCtx(), "AppState");
  VERIFY_METHODS(mod.get(),
    {"getCurrentAppState", 0},
    {"getConstants", 0});
}

TEST_F(LeafTurboModuleTest, AppState_GetCurrentAppState_HasZeroArgs) {
  auto mod = std::make_unique<MethodMapAccessor<AppStateTurboModule>>(makeCtx(), "AppState");
  EXPECT_EQ(mod->getMethodArgCount("getCurrentAppState"), 0u);
}

TEST_F(LeafTurboModuleTest, AppState_GetConstants_HasZeroArgs) {
  auto mod = std::make_unique<MethodMapAccessor<AppStateTurboModule>>(makeCtx(), "AppState");
  EXPECT_EQ(mod->getMethodArgCount("getConstants"), 0u);
}

TEST_F(LeafTurboModuleTest, AppState_ModuleName_IsCorrect) {
  auto mod = std::make_unique<MethodMapAccessor<AppStateTurboModule>>(makeCtx(), "AppState");
  EXPECT_EQ(mod->getName(), "AppState");
}

// ===== AccessibilityInfoTurboModule =====

TEST_F(LeafTurboModuleTest, AccessibilityInfo_RegistersFiveMethods) {
  auto mod = std::make_unique<MethodMapAccessor<AccessibilityInfoTurboModule>>(makeCtx(), "AccessibilityInfo");
  VERIFY_METHODS(mod.get(),
    {"isScreenReaderEnabled", 0},
    {"isBoldTextEnabled", 0},
    {"announceForAccessibility", 1},
    {"setAccessibilityFocus", 1},
    {"isAccessibilityServiceEnabled", 0});
}

TEST_F(LeafTurboModuleTest, AccessibilityInfo_AnnounceForAccessibility_HasOneArg) {
  auto mod = std::make_unique<MethodMapAccessor<AccessibilityInfoTurboModule>>(makeCtx(), "AccessibilityInfo");
  EXPECT_EQ(mod->getMethodArgCount("announceForAccessibility"), 1u);
}

TEST_F(LeafTurboModuleTest, AccessibilityInfo_SetAccessibilityFocus_HasOneArg) {
  auto mod = std::make_unique<MethodMapAccessor<AccessibilityInfoTurboModule>>(makeCtx(), "AccessibilityInfo");
  EXPECT_EQ(mod->getMethodArgCount("setAccessibilityFocus"), 1u);
}

TEST_F(LeafTurboModuleTest, AccessibilityInfo_ModuleName_IsCorrect) {
  auto mod = std::make_unique<MethodMapAccessor<AccessibilityInfoTurboModule>>(makeCtx(), "AccessibilityInfo");
  EXPECT_EQ(mod->getName(), "AccessibilityInfo");
}

// ===== AlertManagerTurboModule =====

TEST_F(LeafTurboModuleTest, Alert_RegistersTwoMethods) {
  auto mod = std::make_unique<MethodMapAccessor<AlertManagerTurboModule>>(makeCtx(), "Alert");
  VERIFY_METHODS(mod.get(),
    {"getConstants", 0},
    {"alert", 3});
}

TEST_F(LeafTurboModuleTest, Alert_AlertMethod_HasThreeArgs) {
  auto mod = std::make_unique<MethodMapAccessor<AlertManagerTurboModule>>(makeCtx(), "Alert");
  EXPECT_EQ(mod->getMethodArgCount("alert"), 3u);
}

TEST_F(LeafTurboModuleTest, Alert_ModuleName_IsCorrect) {
  auto mod = std::make_unique<MethodMapAccessor<AlertManagerTurboModule>>(makeCtx(), "Alert");
  EXPECT_EQ(mod->getName(), "Alert");
}

// ===== ShareTurboModule =====

TEST_F(LeafTurboModuleTest, Share_RegistersOneMethod) {
  auto mod = std::make_unique<MethodMapAccessor<ShareTurboModule>>(makeCtx(), "Share");
  VERIFY_METHODS(mod.get(), {"share", 2});
}

TEST_F(LeafTurboModuleTest, Share_ShareMethod_HasTwoArgs) {
  auto mod = std::make_unique<MethodMapAccessor<ShareTurboModule>>(makeCtx(), "Share");
  EXPECT_EQ(mod->getMethodArgCount("share"), 2u);
}

TEST_F(LeafTurboModuleTest, Share_ModuleName_IsCorrect) {
  auto mod = std::make_unique<MethodMapAccessor<ShareTurboModule>>(makeCtx(), "Share");
  EXPECT_EQ(mod->getName(), "Share");
}

// ===== LinkingManagerTurboModule =====

TEST_F(LeafTurboModuleTest, Linking_RegistersFourMethods) {
  auto mod = std::make_unique<MethodMapAccessor<LinkingManagerTurboModule>>(makeCtx(), "Linking");
  VERIFY_METHODS(mod.get(),
    {"getInitialURL", 0},
    {"canOpenURL", 1},
    {"openURL", 1},
    {"openSettings", 0});
}

TEST_F(LeafTurboModuleTest, Linking_CanOpenURL_HasOneArg) {
  auto mod = std::make_unique<MethodMapAccessor<LinkingManagerTurboModule>>(makeCtx(), "Linking");
  EXPECT_EQ(mod->getMethodArgCount("canOpenURL"), 1u);
}

TEST_F(LeafTurboModuleTest, Linking_OpenURL_HasOneArg) {
  auto mod = std::make_unique<MethodMapAccessor<LinkingManagerTurboModule>>(makeCtx(), "Linking");
  EXPECT_EQ(mod->getMethodArgCount("openURL"), 1u);
}

TEST_F(LeafTurboModuleTest, Linking_ModuleName_IsCorrect) {
  auto mod = std::make_unique<MethodMapAccessor<LinkingManagerTurboModule>>(makeCtx(), "Linking");
  EXPECT_EQ(mod->getName(), "Linking");
}
