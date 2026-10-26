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

#include "ImageCacheResourceLoader.h"
#include <cstring>
#include <fstream>
#include "ImageCacheLog.h"
#include "ImageCacheTaskInternal.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"

namespace ImageCachePro {

namespace {

constexpr const char* FILE_PREFIX = "file://";

std::string StripFileUriPrefix(const std::string& path) {
  if (path.rfind(FILE_PREFIX, 0) == 0) {
    return path.substr(std::strlen(FILE_PREFIX));
  }
  return path;
}

bool LoadFromRawfileImpl(
    const NativeResourceManager* resourceManager,
    const std::string& relativePath,
    std::shared_ptr<ImageCacheTask> task,
    bool echoError) {
  if (resourceManager == nullptr) {
    if (echoError && task != nullptr) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::RESOURCE_MANAGER_NULL);
    }
    return false;
  }

  RawFile64* rawFile = OH_ResourceManager_OpenRawFile64(
      const_cast<NativeResourceManager*>(resourceManager),
      relativePath.c_str());
  if (rawFile == nullptr) {
    if (echoError && task != nullptr) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::OPEN_RAWFILE_FAILED);
    }
    return false;
  }

  uint64_t length = OH_ResourceManager_GetRawFileRemainingLength64(rawFile);
  if (length == 0 || length > UINT32_MAX) {
    OH_ResourceManager_CloseRawFile64(rawFile);
    if (echoError && task != nullptr) {
      TASK_SET_ERROR(
          task,
          length == 0 ? ImageCacheErrorCode::EMPTY_LOCAL_FILE
                      : ImageCacheErrorCode::LOCAL_FILE_SIZE_OUT_OF_RANGE);
    }
    return false;
  }

  auto buffer =
      std::shared_ptr<uint8_t[]>(new uint8_t[static_cast<size_t>(length)]);
  uint64_t readLength =
      OH_ResourceManager_ReadRawFile64(rawFile, buffer.get(), length);
  OH_ResourceManager_CloseRawFile64(rawFile);
  if (readLength == 0) {
    if (echoError && task != nullptr) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::READ_RAWFILE_FAILED);
    }
    return false;
  }

  task->product.imageBuffer = buffer;
  task->product.imageLength = static_cast<uint32_t>(readLength);
  return true;
}

} // namespace

bool ImageCacheResourceLoader::LoadFromFilePath(
    const std::string& path,
    std::shared_ptr<ImageCacheTask> task) {
  IMAGE_CACHE_CHECK_RETURN(task == nullptr, false);
  const std::string filePath = StripFileUriPrefix(path);
  std::ifstream file(filePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::OPEN_LOCAL_FILE_FAILED);
    return false;
  }

  auto size = file.tellg();
  if (size <= 0) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::EMPTY_LOCAL_FILE);
    return false;
  }
  if (static_cast<uint64_t>(size) > UINT32_MAX) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::LOCAL_FILE_SIZE_OUT_OF_RANGE);
    return false;
  }

  const auto fileSize = static_cast<size_t>(size);
  auto buffer = std::shared_ptr<uint8_t[]>(new uint8_t[fileSize]);
  file.seekg(0, std::ios::beg);
  file.read(
      reinterpret_cast<char*>(buffer.get()),
      static_cast<std::streamsize>(fileSize));
  if (!file || static_cast<size_t>(file.gcount()) != fileSize) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::READ_LOCAL_FILE_FAILED);
    return false;
  }

  task->product.imageBuffer = buffer;
  task->product.imageLength = static_cast<uint32_t>(fileSize);
  return true;
}

bool ImageCacheResourceLoader::LoadFromRawfile(
    const std::string& relativePath,
    std::shared_ptr<ImageCacheTask> task,
    const NativeResourceManager* resourceManager,
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
    bool echoError) {
  IMAGE_CACHE_CHECK_RETURN(task == nullptr, false);
  if (relativePath.empty()) {
    if (echoError) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::EMPTY_RAWFILE_PATH);
    }
    return false;
  }

  // Already on MAIN thread - execute directly (no cross-thread risk).
  if (taskExecutor == nullptr ||
      taskExecutor->isOnTaskThread(rnoh::TaskThread::MAIN)) {
    return LoadFromRawfileImpl(resourceManager, relativePath, task, echoError);
  }

  // On WORKER/other thread - schedule asynchronously to MAIN to avoid
  // deadlock (runSyncTask would block WORKER waiting for MAIN, which may
  // be busy processing NotifyCompletedRequests callbacks).
  // After loading on MAIN, Resume the suspended task back to WORKER.
  auto taskInternal =
      std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  if (taskInternal == nullptr) {
    return LoadFromRawfileImpl(resourceManager, relativePath, task, echoError);
  }

  auto weakSelf =
      std::weak_ptr<ImageCacheTaskInternal>(taskInternal->shared_from_this());

  // Suspend before runTask to prevent race: if MAIN thread executes the
  // lambda before Suspend(), Resume would fail (task still RUNNING).
  taskInternal->Suspend();

  taskExecutor->runTask(rnoh::TaskThread::MAIN,
      [weakSelf, resourceManager, relativePath, echoError]() {
        auto self = weakSelf.lock();
        if (!self || self->ShouldAbort()) {
          return;
        }
        // Guard against resourceManager becoming dangling if the manager
        // is destroyed between scheduling and execution.
        if (resourceManager == nullptr) {
          self->Resume(false);
          return;
        }
        bool loaded = LoadFromRawfileImpl(
            resourceManager, relativePath, self, echoError);
        self->Resume(loaded);
      });

  return true;
}

bool ImageCacheResourceLoader::LoadFromRawfileCandidates(
    std::vector<std::string> candidates,
    std::shared_ptr<ImageCacheTask> task,
    const NativeResourceManager* resourceManager,
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor) {
  IMAGE_CACHE_CHECK_RETURN(task == nullptr, false);
  if (candidates.empty()) {
    TASK_SET_ERROR(task, ImageCacheErrorCode::EMPTY_RAWFILE_PATH);
    return false;
  }

  // Already on MAIN thread - try each candidate directly.
  if (taskExecutor == nullptr ||
      taskExecutor->isOnTaskThread(rnoh::TaskThread::MAIN)) {
    for (size_t i = 0; i < candidates.size(); ++i) {
      if (LoadFromRawfileImpl(
              resourceManager, candidates[i], task, i + 1 == candidates.size())) {
        return true;
      }
    }
    return false;
  }

  // On WORKER/other thread - schedule all candidates to MAIN at once,
  // so we don't Suspend/Resume per candidate (which would skip remaining
  // candidates on failure).
  auto taskInternal =
      std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  if (taskInternal == nullptr) {
    for (size_t i = 0; i < candidates.size(); ++i) {
      if (LoadFromRawfileImpl(
              resourceManager, candidates[i], task, i + 1 == candidates.size())) {
        return true;
      }
    }
    return false;
  }

  auto weakSelf =
      std::weak_ptr<ImageCacheTaskInternal>(taskInternal->shared_from_this());

  // Suspend before runTask to prevent race: if MAIN thread executes the
  // lambda before Suspend(), Resume would fail (task still RUNNING).
  taskInternal->Suspend();

  taskExecutor->runTask(rnoh::TaskThread::MAIN,
      [weakSelf, resourceManager,
       candidates = std::move(candidates)]() mutable {
        auto self = weakSelf.lock();
        if (!self || self->ShouldAbort()) {
          return;
        }
        // Guard against resourceManager becoming dangling if the manager
        // is destroyed between scheduling and execution.
        if (resourceManager == nullptr) {
          self->Resume(false);
          return;
        }
        bool loaded = false;
        for (size_t i = 0; i < candidates.size(); ++i) {
          if (LoadFromRawfileImpl(
                  resourceManager, candidates[i], self,
                  i + 1 == candidates.size())) {
            loaded = true;
            break;
          }
        }
        self->Resume(loaded);
      });

  return true;
}
} // namespace ImageCachePro
