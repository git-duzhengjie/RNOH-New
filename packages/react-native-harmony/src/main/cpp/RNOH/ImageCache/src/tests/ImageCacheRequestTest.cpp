/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <ImageCacheOption.h>

#include <ImageCacheTaskWorker.h>
#define IMAGE_CACHE_TASK_WORKER_H
#include <ImageCacheRequest.h>

#include <memory>

using namespace ImageCachePro;

class ImageCacheNormalRequestTest : public ::testing::Test {
 protected:
  void SetUp() override {
    handle = ImageCacheRequest::AllocateHandle();
    option = ImageCacheOption::Create(
        "http://example.com/img.png", "", "", 100, 200);
    callbackCalled = false;
    callback = [this](ImageCacheLoadData, ImageCacheLoadResult) {
      callbackCalled = true;
    };
    request =
        std::make_unique<ImageCacheNormalRequest>(handle, option, callback);
  }
  ImageCacheRequestHandle handle;
  std::shared_ptr<ImageCacheOption> option;
  ImageCacheGetCacheCompleteCallback callback;
  bool callbackCalled;
  std::unique_ptr<ImageCacheNormalRequest> request;
};

// === AllocateHandle (static) ===

TEST(
    ImageCacheRequest,
    AllocateHandle_ConsecutiveCalls_ReturnsIncrementingValues) {
  auto h1 = ImageCacheRequest::AllocateHandle();
  auto h2 = ImageCacheRequest::AllocateHandle();
  EXPECT_GT(h2, h1);
  auto h3 = ImageCacheRequest::AllocateHandle();
  EXPECT_GT(h3, h2);
}

TEST(ImageCacheRequest, AllocateHandle_AllCallsReturnUniqueValues) {
  auto h1 = ImageCacheRequest::AllocateHandle();
  auto h2 = ImageCacheRequest::AllocateHandle();
  auto h3 = ImageCacheRequest::AllocateHandle();
  EXPECT_NE(h1, h2);
  EXPECT_NE(h2, h3);
  EXPECT_NE(h1, h3);
}

TEST(ImageCacheRequest, AllocateHandle_ReturnsNonZero) {
  auto h = ImageCacheRequest::AllocateHandle();
  EXPECT_GT(h, 0);
}

TEST(ImageCacheRequest, AllocateHandle_ManyCalls_AllUnique) {
  std::set<ImageCacheRequestHandle> handles;
  for (int i = 0; i < 100; ++i) {
    handles.insert(ImageCacheRequest::AllocateHandle());
  }
  EXPECT_EQ(handles.size(), 100u);
}

// === Getters ===

TEST_F(ImageCacheNormalRequestTest, GetRequestHandle_ReturnsAllocatedHandle) {
  EXPECT_EQ(request->GetRequestHandle(), handle);
}

TEST_F(ImageCacheNormalRequestTest, GetOption_ReturnsOption) {
  EXPECT_EQ(request->GetOption(), option);
}

TEST_F(ImageCacheNormalRequestTest, GetRequestType_ReturnsRequest) {
  EXPECT_EQ(request->GetRequestType(), ImageCacheHandleType::REQUEST);
}

TEST_F(ImageCacheNormalRequestTest, GetRequestType_NotAnimation) {
  EXPECT_NE(request->GetRequestType(), ImageCacheHandleType::ANIMATION);
}

// === InvokeCallback ===

TEST_F(
    ImageCacheNormalRequestTest,
    InvokeCallback_NullTaskExecutor_CallbackNotCalled) {
  request->InvokeCallback(
      nullptr, ImageCacheLoadData{}, ImageCacheLoadResult{});
  EXPECT_FALSE(callbackCalled);
}

TEST_F(
    ImageCacheNormalRequestTest,
    InvokeCallback_Twice_SecondIsNoOpAndStillFalse) {
  request->InvokeCallback(
      nullptr, ImageCacheLoadData{}, ImageCacheLoadResult{});
  EXPECT_FALSE(callbackCalled);
  request->InvokeCallback(
      nullptr, ImageCacheLoadData{}, ImageCacheLoadResult{});
  EXPECT_FALSE(callbackCalled);
  EXPECT_EQ(request->GetRequestHandle(), handle);
}

TEST_F(
    ImageCacheNormalRequestTest,
    InvokeCallback_NullOnComplete_NoCrashNoCall) {
  auto nullRequest =
      std::make_unique<ImageCacheNormalRequest>(handle, option, nullptr);
  EXPECT_NO_THROW(nullRequest->InvokeCallback(
      nullptr, ImageCacheLoadData{}, ImageCacheLoadResult{}));
  EXPECT_FALSE(callbackCalled);
}

// === Different option values ===

TEST_F(ImageCacheNormalRequestTest, DifferentOption_ProducesDifferentRequest) {
  auto opt2 =
      ImageCacheOption::Create("http://other.com/img.png", "", "", 50, 50);
  auto req2 = std::make_unique<ImageCacheNormalRequest>(handle, opt2, callback);
  EXPECT_NE(request->GetOption(), req2->GetOption());
}
