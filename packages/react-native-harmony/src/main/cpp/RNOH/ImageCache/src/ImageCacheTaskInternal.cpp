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

#include "ImageCacheTaskInternal.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include "ImageCacheDecodeUtil.h"
#include "ImageCacheDispatcher.h"
#include "ImageCacheDownSampler.h"
#include "ImageCacheFileTypeUtil.h"
#include "ImageCacheJobQueue.h"
#include "ImageCacheLoaderInternal.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheOption.h"
#include "ImageCachePixelmapUtils.h"
#include "MemoryCache/ImageCacheMemoryCache.h"
#include "utils/ImageCacheLog.h"
#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
#include "ImageCacheEGL.h"
#include "transformation/ImageCacheTransformation.h"
#endif

namespace ImageCachePro {

namespace {

static ImageCacheRequestType GetRequestTypeFromJob(
    const std::shared_ptr<ImageCacheJob>& job) {
  return job ? job->GetType() : ImageCacheRequestType::MAIN_SRC;
}

static const ImageCacheSource& GetImageSourceFromJob(
    const std::shared_ptr<ImageCacheJob>& job) {
  static const ImageCacheSource kEmpty;
  return job ? job->GetImageSource() : kEmpty;
}

static std::shared_ptr<ImageCacheOption> GetOptionFromJob(
    const std::shared_ptr<ImageCacheJob>& job) {
  return job ? job->GetOption() : nullptr;
}

} // namespace

bool ImageCacheTaskInternal::TransitionTo(
    ImageCacheTaskState expected,
    ImageCacheTaskState desired) {
  ImageCacheTaskState exp = expected;
  return state.compare_exchange_strong(exp, desired, std::memory_order_acq_rel);
}

void ImageCacheTaskInternal::ForceTransitionTo(ImageCacheTaskState desired) {
  ImageCacheTaskState exp = state.load(std::memory_order_acquire);
  while (
      !state.compare_exchange_weak(exp, desired, std::memory_order_acq_rel)) {
    // Already in a terminal state (DONE/CLOSED) - don't force-transition
    // out of it, as that would break state machine invariants.
    if (exp == ImageCacheTaskState::DONE ||
        exp == ImageCacheTaskState::CLOSED) {
      break;
    }
    // CAS failed because state changed; retry with updated expected value
  }
}

bool ImageCacheTaskInternal::ShouldAbort() {
  auto j = GetJob();
  if (j == nullptr) {
    TASK_SET_ERROR(this, ImageCacheErrorCode::JOB_ABANDONED);
    return true;
  }
  auto disp = dispatcher_.lock();
  if (disp == nullptr || disp->IsDestroyed()) {
    TASK_SET_ERROR(this, ImageCacheErrorCode::LOADING_CANCEL);
    return true;
  }
  return false;
}

void ImageCacheTaskInternal::Suspend() {
  suspendStartTime = std::chrono::steady_clock::now();
  auto disp = dispatcher_.lock();
  if (disp) {
    disp->SuspendTask(shared_from_this());
  }
}

void ImageCacheTaskInternal::Resume(bool result) {
  auto disp = dispatcher_.lock();
  if (disp) {
    disp->ResumeTask(shared_from_this(), result);
  }
}

bool ImageCacheTaskInternal::IsSuspended() const {
  return state.load(std::memory_order_acquire) == ImageCacheTaskState::SUSPEND;
}

bool ImageCacheTaskInternal::IsSuspendTimedOut() const {
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - suspendStartTime)
                     .count();
  return static_cast<uint64_t>(elapsed) > SUSPEND_TIMEOUT_MS;
}

void ImageCacheTaskInternal::MarkSuccess() {
  errorCode_.store(ImageCacheErrorCode::OK, std::memory_order_release);
}

bool ImageCacheTaskInternal::IsSuccess() const {
  return errorCode_.load(std::memory_order_acquire) == ImageCacheErrorCode::OK;
}

void ImageCacheTaskInternal::SetError(ImageCacheErrorCode code) {
  errorCode_.store(code, std::memory_order_release);
}

ImageCacheErrorCode ImageCacheTaskInternal::GetError() const {
  return errorCode_.load(std::memory_order_acquire);
}

const ImageCacheSource& ImageCacheTaskInternal::GetImageSource() const {
  return GetImageSourceFromJob(GetJob());
}

std::shared_ptr<ImageCacheOption> ImageCacheTaskInternal::GetOption() const {
  return GetOptionFromJob(GetJob());
}

bool ImageCacheTaskInternal::Execute(
    std::shared_ptr<ImageCacheLoaderInternal> loader) {
  if (ShouldAbort()) {
    return false;
  }
  if (loader == nullptr) {
    SetError(ImageCacheErrorCode::DEFAULT_LOADER_NULL);
    return false;
  }
  if (decodeMode_ == DecodeMode::FRAME) { // frame decode
    DecodeFrame(loader);
  } else {
    if (TryReadFromMemoryCache(loader)) {
      return true;
    }
    if (ShouldAbort()) {
      SetError(ImageCacheErrorCode::TASK_ABORTED);
      return false;
    }
    LoadImage(loader);
    // failed to load image, or suspend, or abort, or not success
    if (IsSuspended() || ShouldAbort() || !IsSuccess()) {
      return false;
    }
    // decode image
    DecodeImage(loader);
  }
  if (ShouldAbort() || !IsSuccess()) {
    return false;
  }

#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
  if (!TransformImage()) {
    SetError(ImageCacheErrorCode::TRANSFORMATION_FAILED);
    return false;
  }
#endif
  if (ShouldAbort() || !IsSuccess()) {
    return false;
  }

  if (writeMemoryCache) {
    TryWriteToMemoryCache(loader);
  }

  return true;
}

void ImageCacheTaskInternal::LoadImage(
    std::shared_ptr<ImageCacheLoaderInternal> loader) {
  if (product.imageBuffer != nullptr) {
    MarkSuccess();
    return;
  }

  if (!loader->DownloadImage(shared_from_this())) {
    if (GetError() == ImageCacheErrorCode::OK) {
      TASK_SET_ERROR(this, ImageCacheErrorCode::DOWNLOAD_FAILED);
    }
    return;
  }
}

void ImageCacheTaskInternal::FinishDecode() {
  if (decodeMode_ == DecodeMode::LAZY_DECODE) {
    return;
  }
  if (product.imageDataList.empty()) {
    TASK_SET_ERROR(this, ImageCacheErrorCode::EMPTY_IMAGE_DATA_AFTER_DECODE);
    return;
  }
  auto job = GetJob();
  if (GetRequestTypeFromJob(job) == ImageCacheRequestType::MAIN_SRC) {
    auto option = GetOptionFromJob(job);
    if (option != nullptr) {
      for (auto& data : product.imageDataList) {
        ImageCachePixelmapUtils::Orientate(data, option->orientation);
      }
    }
  }
  MarkSuccess();
}

void ImageCacheTaskInternal::DecodeImage(
    std::shared_ptr<ImageCacheLoaderInternal> loader) {
  auto err = ImageCacheFileTypeUtil::ParseFromBuffer(*this, fileTypeInfo_);
  if (err != ImageCacheErrorCode::OK) {
    SetError(err);
    return;
  }
  isAnimation = ImageCacheDecodeUtil::IsAnimatedFormat(fileTypeInfo_.format) ||
      fileTypeInfo_.frameCount > 1;
  CalculateDesiredImageSize();
  CheckDecodeMode();

  // -- Create frameDecodeContext for per-frame playback --
  if (decodeMode_ == ImageCacheTaskInternal::DecodeMode::BATCH_TO_FRAME ||
      decodeMode_ == ImageCacheTaskInternal::DecodeMode::LAZY_DECODE) {
    auto frameDecodeContext = std::make_shared<FrameDecodeContext>();
    product.frameDecodeContext = frameDecodeContext;
    frameDecodeContext->desiredImageSize_ = desiredImageSize_;
    frameDecodeContext->fileTypeInfo_ = fileTypeInfo_;
  }

  // -- Decode via interceptor chain --
  if (!loader->DecodeImage(shared_from_this())) {
    if (GetError() == ImageCacheErrorCode::OK) {
      TASK_SET_ERROR(this, ImageCacheErrorCode::DECODE_FAILED);
    }
    return;
  }
  FinishDecode();
}

void ImageCacheTaskInternal::DecodeFrame(
    std::shared_ptr<ImageCacheLoaderInternal> loader) {
  if (!loader->DecodeImage(shared_from_this()) ||
      GetError() != ImageCacheErrorCode::OK) {
    if (GetError() == ImageCacheErrorCode::OK) {
      TASK_SET_ERROR(this, ImageCacheErrorCode::DECODE_FAILED);
    }
    return;
  }
  this->writeMemoryCache = false;
  FinishDecode();
}

void ImageCacheTaskInternal::CheckDecodeMode() {
  if (fileTypeInfo_.format == ImageCacheFormat::UNKNOWN ||
      fileTypeInfo_.width == 0 || fileTypeInfo_.height == 0) {
    return;
  }

  auto option = GetOptionFromJob(GetJob());
  if (option == nullptr) {
    return;
  }

  uint64_t singleFramePixels =
      ImageCachePixelmapUtils::GetFrameSize(fileTypeInfo_);
  if (desiredImageSize_.width != 0 && desiredImageSize_.height != 0) {
    singleFramePixels = ImageCachePixelmapUtils::GetFrameSize(
        desiredImageSize_, fileTypeInfo_.pixelFormat);
  }
  auto decodeMode = singleFramePixels > SINGLE_FRAME_SAFE_LIMIT
      ? ImageCacheTaskInternal::DecodeMode::LAZY_DECODE
      : ImageCacheTaskInternal::DecodeMode::BATCH_TO_FRAME;
  const uint64_t totalFramePixels =
      singleFramePixels * fileTypeInfo_.frameCount;
  if (totalFramePixels > GetMemoryCacheMaxEntrySize()) {
    if (fileTypeInfo_.frameCount > 1) {
      this->writeMemoryCache = true;
      SetDecodeMode(decodeMode);
    } else {
      this->writeMemoryCache = false;
      SetDecodeMode(DecodeMode::NORMAL);
    }
    return;
  }
  if (fileTypeInfo_.frameCount <= 1) {
    this->writeMemoryCache = true;
    SetDecodeMode(DecodeMode::NORMAL);
    return;
  }

  // Too many frames for CreatePixelmapList batch decode -- switch to per-frame.
  if (fileTypeInfo_.frameCount > MAX_BATCH_FRAME_COUNT) {
    DLOG(WARNING) << "CheckDecodeMode: frameCount=" << fileTypeInfo_.frameCount
                  << " > " << MAX_BATCH_FRAME_COUNT
                  << ", switch to per-frame decode";
    this->writeMemoryCache = true;
    SetDecodeMode(decodeMode);
    return;
  }
  if (option->animationDecodeMode ==
          ImageCacheAnimationDecodeMode::FRAME_MODE ||
      option->animationDecodeMode ==
          ImageCacheAnimationDecodeMode::BATCH_MODE) {
    this->writeMemoryCache = true;
    SetDecodeMode(decodeMode);
    return;
  }
}

void ImageCacheTaskInternal::CalculateDesiredImageSize() {
  auto job = GetJob();
  if (fileTypeInfo_.format == ImageCacheFormat::UNKNOWN || job == nullptr) {
    return;
  }
  auto option = job->GetOption();
  if (option == nullptr) {
    return;
  }

  Image_Size requestSize = option->requestImageSize;
  if (job->GetType() == ImageCacheRequestType::MAIN_SRC) {
    desiredImageSize_ = ImageCacheDownSampler::CalculateSize(
        fileTypeInfo_.format,
        {.width = fileTypeInfo_.width, .height = fileTypeInfo_.height},
        requestSize,
        option->downSampling,
        option->resizeMultiplier);
  }

  if (fileTypeInfo_.format == ImageCacheFormat::SVG) {
    if (requestSize.width != 0 && requestSize.height != 0) {
      desiredImageSize_ = requestSize;
    } else {
      desiredImageSize_.width = fileTypeInfo_.width;
      desiredImageSize_.height = fileTypeInfo_.height;
    }
  }
}

std::string ImageCacheTaskInternal::GetTaskInfo() const {
  auto job = GetJob();
  auto addr = reinterpret_cast<uintptr_t>(this);
  std::ostringstream oss;
  oss << "[task:0x" << std::hex << addr
      << " state:" << static_cast<int>(state.load())
      << " ErrorCode:" << static_cast<int>(errorCode_.load())
      << " key:" << (job != nullptr ? job->GetRequestKey() : "unknown") << "]";
  return oss.str();
}

#ifdef IMAGE_CACHE_ENABLE_TRANSFORMATION
bool ImageCacheTaskInternal::TransformImage() {
  if (product.imageDataList.empty()) { // no data, return return true
    return true;
  }
  auto job = GetJob();
  if (GetRequestTypeFromJob(job) != ImageCacheRequestType::MAIN_SRC) {
    return true;
  }
  auto option = GetOptionFromJob(job);
  if (option == nullptr) {
    return true;
  }

  try {
    if (option->multiTransformation != nullptr) {
      option->multiTransformation->Transform(shared_from_this());
    } else if (option->transformation != nullptr) {
      auto& egl = ImageCacheEGL::GetInstance();
      if (!egl.EnsureInitialized()) {
        TASK_SET_ERROR(this, ImageCacheErrorCode::GL_INIT_FAILED);
        return false;
      }
      if (!option->transformation->Transform(shared_from_this())) {
        TASK_SET_ERROR(this, ImageCacheErrorCode::TRANSFORMATION_FAILED);
        return false;
      }
    }
  } catch (const std::exception& err) {
    DLOG(ERROR) << "TransformImage failed: " << err.what();
    TASK_SET_ERROR(this, ImageCacheErrorCode::TRANSFORMATION_FAILED);
    return false;
  }
  return true;
}
#endif

void ImageCacheTaskInternal::UpdateFileTypeInfo() {
  if (product.imageDataList.empty()) {
    return;
  }
  auto& data = product.imageDataList.front();
  auto* pixelmap = data ? data->GetPixelmap() : nullptr;
  if (pixelmap == nullptr) {
    return;
  }
  int32_t pixelFormat = PIXEL_FORMAT_RGBA_8888;
  auto imageSize =
      ImageCachePixelmapUtils::GetImageSize(pixelmap, &pixelFormat);
  if (fileTypeInfo_.width != imageSize.width ||
      fileTypeInfo_.height != imageSize.height ||
      fileTypeInfo_.pixelFormat != pixelFormat) {
    fileTypeInfo_.width = imageSize.width;
    fileTypeInfo_.height = imageSize.height;
    fileTypeInfo_.pixelFormat = pixelFormat;
  }
}

bool ImageCacheTaskInternal::TryReadFromMemoryCache(
    std::shared_ptr<ImageCacheLoaderInternal> loader) {
  auto disp = dispatcher_.lock();
  if (disp == nullptr) {
    return false;
  }
  auto option = GetOptionFromJob(GetJob());
  if (option == nullptr) {
    return false;
  }
  MarkSuccess();
  cacheKey = disp->GenerateMemoryKey(*option, false);
  if (loader->LoadFromMemory(shared_from_this())) {
    return true;
  }
  cacheKey = disp->GenerateMemoryKey(*option, true);
  if (loader->LoadFromMemory(shared_from_this())) {
    return true;
  }
  return false;
}

void ImageCacheTaskInternal::TryWriteToMemoryCache(
    std::shared_ptr<ImageCacheLoaderInternal> loader) {
  auto disp = dispatcher_.lock();
  if (disp == nullptr) {
    return;
  }
  auto option = GetOptionFromJob(GetJob());
  if (option == nullptr) {
    return;
  }
  cacheKey = disp->GenerateMemoryKey(*option, IsAnimation());
  UpdateFileTypeInfo();
  loader->WriteCacheToMemory(shared_from_this());
}
} // namespace ImageCachePro
