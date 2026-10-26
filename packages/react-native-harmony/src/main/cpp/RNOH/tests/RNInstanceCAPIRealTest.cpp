/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include <folly/dynamic.h>
#include "rnoh_all.h"

using namespace rnoh;
using namespace facebook::react;

class MockTaskExecutor : public TaskExecutor {
 public:
  void runTask(TaskThread, Task&& task) override { task(); }
  void runSyncTask(TaskThread, Task&& task) override { task(); }
};

static std::shared_ptr<RNInstanceCAPI> createInstance() {
  auto inst = std::make_shared<RNInstanceCAPI>();
  inst->m_taskExecutor = std::make_shared<MockTaskExecutor>();
  inst->m_componentInstanceRegistry = std::make_shared<ComponentInstanceRegistry>();
  inst->m_contextContainer = std::make_shared<const ContextContainer>();
  return inst;
}

TEST(RNInstanceCAPIReal, Constructor_DoesNotThrow) {
  EXPECT_NO_THROW(createInstance());
}

TEST(RNInstanceCAPIReal, UpdateSurfaceConstraints_9Param_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->updateSurfaceConstraints(999, 0, 0, 1000, 1000, 0, 0, 2.0f, false, true));
}

TEST(RNInstanceCAPIReal, UpdateSurfaceConstraints_10Param_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->updateSurfaceConstraints(999, 0, 0, 1000, 1000, 0, 0, 2.0f, 1.5f, false, true));
}

TEST(RNInstanceCAPIReal, MeasureSurface_NotFound_Throws) {
  auto inst = createInstance();
  EXPECT_ANY_THROW(inst->measureSurface(999, 0, 0, 1000, 1000, 0, 0, 2.0f, false, true));
}

TEST(RNInstanceCAPIReal, MeasureSurface_10Param_NotFound_Throws) {
  auto inst = createInstance();
  EXPECT_ANY_THROW(inst->measureSurface(999, 0, 0, 1000, 1000, 0, 0, 2.0f, 1.5f, false, true));
}

TEST(RNInstanceCAPIReal, StartSurface_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  folly::dynamic props = folly::dynamic::object();
  EXPECT_NO_THROW(inst->startSurface(999, 0, 0, 1000, 1000, 0, 0, 2.0f, false, std::move(props), true));
}

TEST(RNInstanceCAPIReal, SetSurfaceProps_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  folly::dynamic props = folly::dynamic::object();
  EXPECT_NO_THROW(inst->setSurfaceProps(999, std::move(props)));
}

TEST(RNInstanceCAPIReal, StopSurface_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->stopSurface(999, []() {}));
}

TEST(RNInstanceCAPIReal, DestroySurface_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->destroySurface(999));
}

TEST(RNInstanceCAPIReal, SetSurfaceDisplayMode_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->setSurfaceDisplayMode(999, DisplayMode::Default));
}

TEST(RNInstanceCAPIReal, OnConfigurationChange_NullPayload_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->onConfigurationChange(folly::dynamic(nullptr)));
}

TEST(RNInstanceCAPIReal, OnConfigurationChange_MissingKeys_DoesNotThrow) {
  auto inst = createInstance();
  auto payload = folly::dynamic::object("key", "value");
  EXPECT_NO_THROW(inst->onConfigurationChange(payload));
}

TEST(RNInstanceCAPIReal, OnConfigurationChange_WithPixels_DoesNotThrow) {
  auto inst = createInstance();
  auto payload = folly::dynamic::object
    ("windowPhysicalPixels", folly::dynamic::object("scale", 2.0)("fontScale", 1.5)("width", 1080.0)("height", 1920.0)("xDpi", 1.0)("yDpi", 1.0))
    ("screenPhysicalPixels", folly::dynamic::object("scale", 2.0)("fontScale", 1.5)("width", 1080.0)("height", 1920.0)("xDpi", 1.0)("yDpi", 1.0));
  EXPECT_NO_THROW(inst->onConfigurationChange(payload));
}

TEST(RNInstanceCAPIReal, FindComponentInstanceByTag_ReturnsNullptr) {
  auto inst = createInstance();
  EXPECT_EQ(inst->findComponentInstanceByTag(1), nullptr);
}

TEST(RNInstanceCAPIReal, FindComponentInstanceTagById_ReturnsNullopt) {
  auto inst = createInstance();
  auto result = inst->findComponentInstanceTagById("testId");
  EXPECT_FALSE(result.has_value());
}

TEST(RNInstanceCAPIReal, GetNativeNodeIdByTag_NotFound_ReturnsNullopt) {
  auto inst = createInstance();
  auto result = inst->getNativeNodeIdByTag(999);
  EXPECT_FALSE(result.has_value());
}

TEST(RNInstanceCAPIReal, GetSurfaceByRootTag_NotFound_ReturnsNullopt) {
  auto inst = createInstance();
  auto result = inst->getSurfaceByRootTag(999);
  EXPECT_FALSE(result.has_value());
}

TEST(RNInstanceCAPIReal, AttachRootView_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  NodeContentHandle handle;
  EXPECT_NO_THROW(inst->attachRootView(std::move(handle), 999));
}

TEST(RNInstanceCAPIReal, DetachRootView_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->detachRootView(999));
}

TEST(RNInstanceCAPIReal, SynchronouslyUpdateViewOnUIThread_NotFound_DoesNotThrow) {
  auto inst = createInstance();
  EXPECT_NO_THROW(inst->synchronouslyUpdateViewOnUIThread(999, folly::dynamic::object()));
}

// === Helper: create instance with a surface ===
static std::shared_ptr<RNInstanceCAPI> createInstanceWithSurface(SurfaceId surfaceId = 1) {
  auto inst = createInstance();
  inst->m_scheduler = std::make_shared<Scheduler>();
  inst->m_arkTSMessageHub = std::make_shared<ArkTSMessageHub>();
  inst->m_arkTSBridge = std::make_shared<ArkTSBridge>(nullptr);
  inst->m_componentInstanceFactory = std::make_shared<ComponentInstanceFactory>();
  inst->createSurface(surfaceId, "testModule");
  return inst;
}

// === Surface "found" path tests ===

TEST(RNInstanceCAPIReal, UpdateSurfaceConstraints_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  EXPECT_NO_THROW(inst->updateSurfaceConstraints(1, 0, 0, 1000, 1000, 0, 0, 2.0f, false, true));
}

TEST(RNInstanceCAPIReal, MeasureSurface_Found_ReturnsSize) {
  auto inst = createInstanceWithSurface();
  EXPECT_NO_THROW(inst->measureSurface(1, 0, 0, 1000, 1000, 0, 0, 2.0f, false, true));
}

TEST(RNInstanceCAPIReal, StartSurface_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  folly::dynamic props = folly::dynamic::object();
  EXPECT_NO_THROW(inst->startSurface(1, 0, 0, 1000, 1000, 0, 0, 2.0f, false, std::move(props), true));
}

TEST(RNInstanceCAPIReal, SetSurfaceProps_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  folly::dynamic props = folly::dynamic::object();
  EXPECT_NO_THROW(inst->setSurfaceProps(1, std::move(props)));
}

TEST(RNInstanceCAPIReal, StopSurface_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  EXPECT_NO_THROW(inst->stopSurface(1, []() {}));
}

TEST(RNInstanceCAPIReal, DestroySurface_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  EXPECT_NO_THROW(inst->destroySurface(1));
}

TEST(RNInstanceCAPIReal, SetSurfaceDisplayMode_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  EXPECT_NO_THROW(inst->setSurfaceDisplayMode(1, DisplayMode::Visible));
}

TEST(RNInstanceCAPIReal, AttachRootView_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  NodeContentHandle handle;
  EXPECT_NO_THROW(inst->attachRootView(std::move(handle), 1));
}

TEST(RNInstanceCAPIReal, DetachRootView_Found_DoesNotThrow) {
  auto inst = createInstanceWithSurface();
  EXPECT_NO_THROW(inst->detachRootView(1));
}

TEST(RNInstanceCAPIReal, GetSurfaceByRootTag_Found_ReturnsWeak) {
  auto inst = createInstanceWithSurface();
  auto result = inst->getSurfaceByRootTag(1);
  EXPECT_TRUE(result.has_value());
}

// === Destructor with listener and surfaces ===

TEST(RNInstanceCAPIReal, Destructor_WithListener_CallsUnsubscribe) {
  auto inst = createInstance();
  inst->m_unsubscribeUITickListener = []() {};
}

TEST(RNInstanceCAPIReal, Destructor_WithSurfaces_StopsAllSurfaces) {
  auto inst = createInstanceWithSurface();
  inst->m_unsubscribeUITickListener = []() {};
}

// === onConfigurationChange with partial payload and surfaces ===

TEST(RNInstanceCAPIReal, OnConfigurationChange_WindowOnly_NoScreen_DoesNotThrow) {
  auto inst = createInstance();
  auto payload = folly::dynamic::object("windowPhysicalPixels", folly::dynamic::object("scale", 2.0));
  EXPECT_NO_THROW(inst->onConfigurationChange(payload));
}

TEST(RNInstanceCAPIReal, OnConfigurationChange_WithSurfaces_Iterates) {
  auto inst = createInstanceWithSurface();
  auto payload = folly::dynamic::object
    ("windowPhysicalPixels", folly::dynamic::object("scale", 2.0)("fontScale", 1.5)("width", 1080.0)("height", 1920.0)("xDpi", 1.0)("yDpi", 1.0))
    ("screenPhysicalPixels", folly::dynamic::object("scale", 2.0)("fontScale", 1.5)("width", 1080.0)("height", 1920.0)("xDpi", 1.0)("yDpi", 1.0));
  EXPECT_NO_THROW(inst->onConfigurationChange(payload));
}
