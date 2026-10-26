/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ImageCacheJobQueue.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheTaskInternal.h"

namespace ImageCachePro {

std::shared_ptr<ImageCacheJob> ImageCacheJob::CreateJob(
    const std::string& requestKey,
    std::shared_ptr<ImageCacheOption> option,
    std::shared_ptr<ImageCacheDispatcher> dispatcher,
    std::shared_ptr<ImageCacheRequest> request,
    FrameDecodeRequest decodeRequest) {
  if (dispatcher == nullptr || option == nullptr) {
    return nullptr;
  }
  auto job = std::make_shared<ImageCacheJob>();
  if (job == nullptr) {
    return nullptr;
  }
  job->requestKey = requestKey;
  job->type = ImageCacheRequestType::MAIN_SRC;
  job->imageSrc = option->loadSrc;
  job->option = std::move(option);

  job->task = std::make_shared<ImageCacheTaskInternal>(
      job, dispatcher);
  if (job->task == nullptr) {
    job.reset();
    return nullptr;
  }
  if (decodeRequest.frameContext != nullptr) {
    job->priority = ImageCachePriority::HIGH;
    Image_Size desiredSize = decodeRequest.frameContext->desiredImageSize_;
    job->decodeRequest_ = decodeRequest;
    job->task->SetDecodeMode(ImageCacheTaskInternal::DecodeMode::FRAME);
    job->task->SetDesiredImageSize(desiredSize);
    job->task->SetImageSourceNativeWrapper(
        decodeRequest.frameContext->imageSourceNative_);
    job->task->SetFileTypeInfo(
        decodeRequest.frameContext->fileTypeInfo_);
  }
  return job;
}

void ImageCacheJob::UpdateDecodeRequest(
    const FrameDecodeRequest& decodeRequest) {
  if (task == nullptr) {
    return;
  }
  if (decodeRequest.frameContext != nullptr) {
    priority = ImageCachePriority::HIGH;
    decodeRequest_ = decodeRequest;
    Image_Size desiredSize = decodeRequest.frameContext->desiredImageSize_;
    task->SetDecodeMode(ImageCacheTaskInternal::DecodeMode::FRAME);
    task->SetDesiredImageSize(desiredSize);
    task->SetImageSourceNativeWrapper(
        decodeRequest.frameContext->imageSourceNative_);
    task->SetFileTypeInfo(decodeRequest.frameContext->fileTypeInfo_);
    task->product.imageDataList.clear();
    task->product.imageBuffer.reset();
    task->product.imageLength = 0;
    task->ResetForReuse();
  }
}

} // namespace ImageCachePro
