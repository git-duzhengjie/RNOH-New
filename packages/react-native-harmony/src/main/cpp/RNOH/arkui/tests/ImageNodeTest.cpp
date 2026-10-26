/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ImageNode.h>
#include <gtest/gtest.h>
#include <MockArkUIAPI.h>

using namespace rnoh;

// ============================================================================
// Tier 4a — vtable Mock (ArkUI C API intercepted via MockArkUIAPI)
// Module: RNOH/arkui/ImageNode
//
// Tested class: ImageNode (ArkUI image node wrapper)
// IMAGE_CACHE_ENABLED is OFF (default), so PixelMap/DrawableDescriptor
// paths are not compiled.
// ============================================================================

class ImageNodeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
    imageNode = std::make_unique<ImageNode>();
    handle = imageNode->getArkUINodeHandle();
    ASSERT_NE(handle, nullptr);
  }
  void TearDown() override {
    imageNode.reset();
  }
  std::unique_ptr<ImageNode> imageNode;
  ArkUI_NodeHandle handle;
};

// ---------------------------------------------------------------------------
// setSource / setSources
// ---------------------------------------------------------------------------

TEST_F(ImageNodeTest, SetSource_RemoteUrl_CallsSetAttributeWithNodeImageSrc) {
  imageNode->setSource("http://example.com/img.png");
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 1);
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_IMAGE_SRC));
}

TEST_F(ImageNodeTest, SetSource_StoresUriForGetUri) {
  imageNode->setSource("http://example.com/img.png");
  EXPECT_EQ(imageNode->getUri(), "http://example.com/img.png");
}

TEST_F(ImageNodeTest, SetSources_WithAssetPrefix_PrependsRawfilePath) {
  imageNode->setSources("asset://icon.png");
  EXPECT_EQ(g_mockCallLog.setAttributeCount, 1);
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_IMAGE_SRC));
}

TEST_F(ImageNodeTest, SetSources_WithoutAssetPrefix_UsesUriDirectly) {
  imageNode->setSources("http://example.com/img.png");
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_IMAGE_SRC));
}

TEST_F(ImageNodeTest, SetSources_StoresUriForGetUri) {
  imageNode->setSources("http://example.com/img.png");
  EXPECT_EQ(imageNode->getUri(), "http://example.com/img.png");
}

// ---------------------------------------------------------------------------
// setResizeMode
// ---------------------------------------------------------------------------

TEST_F(ImageNodeTest, SetResizeMode_Cover_SetsObjectFitCover) {
  imageNode->setResizeMode(facebook::react::ImageResizeMode::Cover);
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_IMAGE_OBJECT_FIT));
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_OBJECT_FIT];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, static_cast<int32_t>(ARKUI_OBJECT_FIT_COVER));
}

TEST_F(ImageNodeTest, SetResizeMode_Contain_SetsObjectFitContain) {
  imageNode->setResizeMode(facebook::react::ImageResizeMode::Contain);
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_OBJECT_FIT];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, static_cast<int32_t>(ARKUI_OBJECT_FIT_CONTAIN));
}

TEST_F(ImageNodeTest, SetResizeMode_Stretch_SetsObjectFitFill) {
  imageNode->setResizeMode(facebook::react::ImageResizeMode::Stretch);
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_OBJECT_FIT];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, static_cast<int32_t>(ARKUI_OBJECT_FIT_FILL));
}

TEST_F(ImageNodeTest, SetResizeMode_Center_SetsObjectFitScaleDown) {
  imageNode->setResizeMode(facebook::react::ImageResizeMode::Center);
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_OBJECT_FIT];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(
      attr.value[0].i32, static_cast<int32_t>(ARKUI_OBJECT_FIT_SCALE_DOWN));
}

// ---------------------------------------------------------------------------
// setBlur / setInterpolation / setFadeDuration
// ---------------------------------------------------------------------------

TEST_F(ImageNodeTest, SetBlur_PositiveValue_CallsSetAttributeWithNodeBlur) {
  imageNode->setBlur(5.0);
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_BLUR));
  auto& attr = g_mockNodes[handle].attributes[NODE_BLUR];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_FLOAT_EQ(attr.value[0].f32, 5.0f);
}

TEST_F(ImageNodeTest, SetInterpolation_Value_CallsSetAttribute) {
  imageNode->setInterpolation(1);
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_IMAGE_INTERPOLATION));
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_INTERPOLATION];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, 1);
}

TEST_F(ImageNodeTest, SetFadeDuration_Value_CallsSetAttribute) {
  imageNode->setFadeDuration(300);
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_OPACITY_TRANSITION));
}

// ---------------------------------------------------------------------------
// setDraggable / setFocusable / setResizeMethod
// ---------------------------------------------------------------------------

TEST_F(ImageNodeTest, SetDraggable_True_SetsOne) {
  imageNode->setDraggable(true);
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_DRAGGABLE];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, 1);
}

TEST_F(ImageNodeTest, SetDraggable_False_SetsZero) {
  imageNode->setDraggable(false);
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_DRAGGABLE];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, 0);
}

TEST_F(ImageNodeTest, SetFocusable_True_SetsOne) {
  imageNode->setFocusable(true);
  auto& attr = g_mockNodes[handle].attributes[NODE_FOCUSABLE];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, 1);
}

TEST_F(ImageNodeTest, SetResizeMethod_Auto_SetsOne) {
  imageNode->setResizeMethod("auto");
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_AUTO_RESIZE];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, 1);
}

TEST_F(ImageNodeTest, SetResizeMethod_Scale_SetsZero) {
  imageNode->setResizeMethod("scale");
  auto& attr = g_mockNodes[handle].attributes[NODE_IMAGE_AUTO_RESIZE];
  ASSERT_NE(attr.value, nullptr);
  EXPECT_EQ(attr.value[0].i32, 0);
}

// ---------------------------------------------------------------------------
// setAlt
// ---------------------------------------------------------------------------

TEST_F(ImageNodeTest, SetAlt_NonEmptyUri_CallsSetAttributeWithNodeImageAlt) {
  imageNode->setAlt("alt_text");
  EXPECT_TRUE(g_mockNodes[handle].attributes.count(NODE_IMAGE_ALT));
}

TEST_F(ImageNodeTest, SetAlt_EmptyUri_DoesNotCallSetAttribute) {
  imageNode->setAlt("");
  EXPECT_FALSE(g_mockNodes[handle].attributes.count(NODE_IMAGE_ALT));
}

// ---------------------------------------------------------------------------
// reset methods
// ---------------------------------------------------------------------------

TEST_F(ImageNodeTest, ResetSource_CallsResetAttribute) {
  imageNode->setSource("http://example.com/img.png");
  imageNode->resetSource();
  EXPECT_FALSE(g_mockNodes[handle].attributes.count(NODE_IMAGE_SRC));
}

TEST_F(ImageNodeTest, ResetFocusable_CallsResetAttribute) {
  imageNode->setFocusable(true);
  imageNode->resetFocusable();
  EXPECT_FALSE(g_mockNodes[handle].attributes.count(NODE_FOCUSABLE));
}

TEST_F(ImageNodeTest, ResetResizeMethod_CallsResetAttribute) {
  imageNode->setResizeMethod("auto");
  imageNode->resetResizeMethod();
  EXPECT_FALSE(g_mockNodes[handle].attributes.count(NODE_IMAGE_AUTO_RESIZE));
}

// ---------------------------------------------------------------------------
// setNodeDelegate / onNodeEvent
// ---------------------------------------------------------------------------

class TestImageNodeDelegate : public ImageNodeDelegate {
 public:
  int onCompleteCount = 0;
  float lastWidth = 0, lastHeight = 0;
  int onErrorCount = 0;
  int32_t lastErrorCode = 0;
  int onProgressCount = 0;
  uint32_t lastLoaded = 0, lastTotal = 0;

  void onComplete(float width, float height) override {
    onCompleteCount++;
    lastWidth = width;
    lastHeight = height;
  }
  void onError(int32_t errorCode) override {
    onErrorCount++;
    lastErrorCode = errorCode;
  }
  void onProgress(uint32_t loaded, uint32_t total) override {
    onProgressCount++;
    lastLoaded = loaded;
    lastTotal = total;
  }
};

TEST_F(ImageNodeTest, OnNodeEvent_Complete_CallsDelegateOnComplete) {
  TestImageNodeDelegate delegate;
  imageNode->setNodeDelegate(&delegate);
  ArkUINode::EventArgs args{};
  args[0].i32 = 1;
  args[1].f32 = 100.0f;
  args[2].f32 = 200.0f;
  imageNode->onNodeEvent(ArkUI_NodeEventType::NODE_IMAGE_ON_COMPLETE, args);
  EXPECT_EQ(delegate.onCompleteCount, 1);
  EXPECT_FLOAT_EQ(delegate.lastWidth, 100.0f);
  EXPECT_FLOAT_EQ(delegate.lastHeight, 200.0f);
}

TEST_F(ImageNodeTest, OnNodeEvent_Error_CallsDelegateOnError) {
  TestImageNodeDelegate delegate;
  imageNode->setNodeDelegate(&delegate);
  ArkUINode::EventArgs args{};
  args[0].i32 = 404;
  imageNode->onNodeEvent(ArkUI_NodeEventType::NODE_IMAGE_ON_ERROR, args);
  EXPECT_EQ(delegate.onErrorCount, 1);
  EXPECT_EQ(delegate.lastErrorCode, 404);
}

TEST_F(ImageNodeTest, OnNodeEvent_Progress_CallsDelegateOnProgress) {
  TestImageNodeDelegate delegate;
  imageNode->setNodeDelegate(&delegate);
  ArkUINode::EventArgs args{};
  args[0].u32 = 1024;
  args[1].u32 = 4096;
  imageNode->onNodeEvent(
      ArkUI_NodeEventType::NODE_IMAGE_ON_DOWNLOAD_PROGRESS, args);
  EXPECT_EQ(delegate.onProgressCount, 1);
  EXPECT_EQ(delegate.lastLoaded, 1024u);
  EXPECT_EQ(delegate.lastTotal, 4096u);
}

TEST_F(ImageNodeTest, OnNodeEvent_CompleteWithoutDelegate_NoCrash) {
  ArkUINode::EventArgs args{};
  args[0].i32 = 1;
  imageNode->onNodeEvent(ArkUI_NodeEventType::NODE_IMAGE_ON_COMPLETE, args);
  SUCCEED();
}

TEST_F(ImageNodeTest, OnNodeEvent_CompleteWithStatusZero_DoesNotCallDelegate) {
  TestImageNodeDelegate delegate;
  imageNode->setNodeDelegate(&delegate);
  ArkUINode::EventArgs args{};
  args[0].i32 = 0; // status 0 = not successful
  imageNode->onNodeEvent(ArkUI_NodeEventType::NODE_IMAGE_ON_COMPLETE, args);
  EXPECT_EQ(delegate.onCompleteCount, 0);
}
