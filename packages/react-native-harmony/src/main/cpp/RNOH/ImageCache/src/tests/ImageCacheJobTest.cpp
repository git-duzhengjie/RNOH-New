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
#include <ImageCacheJobQueue.h>
#include <ImageCacheRequest.h>

#include <memory>
#include <set>
#include <vector>

using namespace ImageCachePro;

class ImageCacheJobTest : public ::testing::Test {
 protected:
  ImageCacheJob job;

  std::shared_ptr<ImageCacheNormalRequest> makeRequest() {
    auto handle = ImageCacheRequest::AllocateHandle();
    auto opt = ImageCacheOption::Create(
        "http://example.com/img.png", "", "", 100, 200);
    return std::make_shared<ImageCacheNormalRequest>(handle, opt, nullptr);
  }
};

// === AddRequest / RemoveRequest ===

TEST_F(ImageCacheJobTest, AddRequest_ThenRemove_ReturnsTrueWhenEmpty) {
  auto req = makeRequest();
  job.AddRequest(req);
  bool empty = job.RemoveRequest(req->GetRequestHandle());
  EXPECT_TRUE(empty);
}

TEST_F(ImageCacheJobTest, AddRequest_TwoRequests_RemoveOne_NotEmpty) {
  auto req1 = makeRequest();
  auto req2 = makeRequest();
  job.AddRequest(req1);
  job.AddRequest(req2);
  bool empty = job.RemoveRequest(req1->GetRequestHandle());
  EXPECT_FALSE(empty);
}

TEST_F(ImageCacheJobTest, RemoveRequest_NonExistentId_ReturnsEmpty) {
  bool empty = job.RemoveRequest(99999);
  EXPECT_TRUE(empty);
  EXPECT_EQ(job.GetRequestKey(), "");
}

TEST_F(ImageCacheJobTest, AddRequest_DuplicateHandle_RemovesAll) {
  auto req = makeRequest();
  job.AddRequest(req);
  job.AddRequest(req);
  bool empty = job.RemoveRequest(req->GetRequestHandle());
  EXPECT_TRUE(empty);
}

TEST_F(ImageCacheJobTest, RemoveRequest_AfterNotify_AlreadyCleared) {
  auto req = makeRequest();
  job.AddRequest(req);
  job.NotifyRequests([](ImageCacheRequestHandle) {});
  bool empty = job.RemoveRequest(req->GetRequestHandle());
  EXPECT_TRUE(empty);
}

// === NotifyRequests ===

TEST_F(ImageCacheJobTest, NotifyRequests_CallbackCalledForEachRequest) {
  auto req1 = makeRequest();
  auto req2 = makeRequest();
  auto req3 = makeRequest();
  job.AddRequest(req1);
  job.AddRequest(req2);
  job.AddRequest(req3);
  std::set<ImageCacheRequestHandle> handles;
  job.NotifyRequests(
      [&handles](ImageCacheRequestHandle h) { handles.insert(h); });
  EXPECT_EQ(handles.size(), 3u);
  EXPECT_NE(handles.count(req1->GetRequestHandle()), 0u);
  EXPECT_NE(handles.count(req2->GetRequestHandle()), 0u);
}

TEST_F(ImageCacheJobTest, NotifyRequests_EmptyJob_NoCallback) {
  std::vector<ImageCacheRequestHandle> handles;
  job.NotifyRequests(
      [&handles](ImageCacheRequestHandle h) { handles.push_back(h); });
  EXPECT_EQ(handles.size(), 0u);
}

TEST_F(ImageCacheJobTest, NotifyRequests_AfterNotify_ListIsCleared) {
  auto req = makeRequest();
  job.AddRequest(req);
  job.NotifyRequests([](ImageCacheRequestHandle) {});
  std::vector<ImageCacheRequestHandle> handles;
  job.NotifyRequests(
      [&handles](ImageCacheRequestHandle h) { handles.push_back(h); });
  EXPECT_EQ(handles.size(), 0u);
  EXPECT_EQ(job.GetRequestKey(), "");
}

TEST_F(ImageCacheJobTest, NotifyRequests_CallbackReceivesCorrectHandles) {
  auto req1 = makeRequest();
  auto req2 = makeRequest();
  job.AddRequest(req1);
  job.AddRequest(req2);
  std::set<ImageCacheRequestHandle> received;
  job.NotifyRequests(
      [&received](ImageCacheRequestHandle h) { received.insert(h); });
  EXPECT_EQ(received.count(req1->GetRequestHandle()), 1u);
  EXPECT_EQ(received.count(req2->GetRequestHandle()), 1u);
  EXPECT_EQ(received.size(), 2u);
}

// === Getters ===

TEST_F(ImageCacheJobTest, GetType_Default_ReturnsMainSrc) {
  EXPECT_EQ(job.GetType(), ImageCacheRequestType::MAIN_SRC);
}

TEST_F(ImageCacheJobTest, GetRequestKey_Default_ReturnsEmpty) {
  EXPECT_TRUE(job.GetRequestKey().empty());
  EXPECT_EQ(job.GetRequestKey(), "");
}

TEST_F(ImageCacheJobTest, IsFrameDecode_Default_ReturnsFalse) {
  EXPECT_FALSE(job.IsFrameDecode());
}

TEST_F(ImageCacheJobTest, GetFrameIndex_Default_ReturnsZero) {
  EXPECT_EQ(job.GetFrameIndex(), 0);
}

TEST_F(ImageCacheJobTest, GetImageSource_Default_IsEmpty) {
  EXPECT_TRUE(job.GetImageSource().IsEmpty());
}

TEST_F(ImageCacheJobTest, GetImageSource_Default_GetStringReturnsFalse) {
  std::string url;
  EXPECT_FALSE(job.GetImageSource().GetString(url));
  EXPECT_TRUE(url.empty());
}

TEST_F(ImageCacheJobTest, GetOption_Default_IsNullptr) {
  EXPECT_EQ(job.GetOption(), nullptr);
}

// === FrameDecodeRequest ===

TEST_F(ImageCacheJobTest, GetDecodeRequest_Default_StartFrameZero) {
  EXPECT_EQ(job.GetDecodeRequest().startFrame, 0);
}

TEST_F(ImageCacheJobTest, GetDecodeRequest_Default_FetchCountOne) {
  EXPECT_EQ(job.GetDecodeRequest().fetchCount, 1);
}

TEST_F(ImageCacheJobTest, GetDecodeRequest_Default_IsNotBatch) {
  EXPECT_FALSE(job.GetDecodeRequest().IsBatch());
}

TEST_F(ImageCacheJobTest, IsBatch_TrueWhenFetchCountGreaterThanOne) {
  FrameDecodeRequest req1{0, 2, nullptr};
  EXPECT_TRUE(req1.IsBatch());
  FrameDecodeRequest req2{0, 100, nullptr};
  EXPECT_TRUE(req2.IsBatch());
}

TEST_F(ImageCacheJobTest, IsBatch_FalseWhenFetchCountOne) {
  FrameDecodeRequest req{0, 1, nullptr};
  EXPECT_FALSE(req.IsBatch());
}

TEST_F(ImageCacheJobTest, IsBatch_FalseWhenFetchCountZero) {
  FrameDecodeRequest req{0, 0, nullptr};
  EXPECT_FALSE(req.IsBatch());
}

TEST_F(ImageCacheJobTest, IsBatch_FetchCount2Vs1_DifferentResults) {
  FrameDecodeRequest single{0, 1, nullptr};
  FrameDecodeRequest batch{0, 2, nullptr};
  EXPECT_NE(single.IsBatch(), batch.IsBatch());
}

// === Multiple operations ===

TEST_F(ImageCacheJobTest, AddRemoveAdd_CycleWorksCorrectly) {
  auto req1 = makeRequest();
  job.AddRequest(req1);
  job.RemoveRequest(req1->GetRequestHandle());
  auto req2 = makeRequest();
  job.AddRequest(req2);
  std::set<ImageCacheRequestHandle> handles;
  job.NotifyRequests(
      [&handles](ImageCacheRequestHandle h) { handles.insert(h); });
  EXPECT_EQ(handles.size(), 1u);
  EXPECT_NE(handles.count(req2->GetRequestHandle()), 0u);
}

TEST_F(ImageCacheJobTest, FiveRequests_NotifyAll_CountMatches) {
  std::set<ImageCacheRequestHandle> allHandles;
  for (int i = 0; i < 5; ++i) {
    auto req = makeRequest();
    allHandles.insert(req->GetRequestHandle());
    job.AddRequest(req);
  }
  std::set<ImageCacheRequestHandle> received;
  job.NotifyRequests(
      [&received](ImageCacheRequestHandle h) { received.insert(h); });
  EXPECT_EQ(received.size(), 5u);
  EXPECT_EQ(received, allHandles);
}
