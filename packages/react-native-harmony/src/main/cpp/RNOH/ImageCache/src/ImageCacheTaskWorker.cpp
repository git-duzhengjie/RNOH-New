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

#include "ImageCacheTaskWorker.h"
#include "ImageCacheLog.h"
#include "ImageCacheTaskInternal.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"

namespace ImageCachePro {

bool ImageCacheTaskWorker::IsMainThread(
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor) {
  if (taskExecutor == nullptr) {
    return false;
  }
  return taskExecutor->isOnTaskThread(rnoh::TaskThread::MAIN);
}

bool ImageCacheTaskWorker::PushTask(
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
    AsyncTask execute,
    AsyncTask complete,
    std::shared_ptr<ImageCacheTaskInternal> task) {
  if (taskExecutor == nullptr) {
    LOG(ERROR) << "PushTask failed: TaskExecutor is not set";
    return false;
  }

  taskExecutor->runTask(
      rnoh::TaskThread::WORKER,
      [execute = std::move(execute),
       complete = std::move(complete),
       task = std::move(task)]() mutable {
        if (execute) {
          execute(task);
        }
        if (complete) {
          complete(task);
        }
      });
  return true;
}

bool ImageCacheTaskWorker::PushTask(
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
    AsyncFunc execute,
    AsyncFunc complete,
    void* data) {
  if (taskExecutor == nullptr) {
    LOG(ERROR) << "PushTask failed: TaskExecutor is not set";
    return false;
  }

  taskExecutor->runTask(
      rnoh::TaskThread::WORKER,
      [execute = std::move(execute),
       complete = std::move(complete),
       data]() mutable {
        if (execute) {
          execute(data);
        }
        if (complete) {
          complete(data);
        }
      });
  return true;
}

void ImageCacheTaskWorker::ToMainThread(
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
    std::function<void(void*)> func,
    void* data) {
  if (func == nullptr) {
    return;
  }

  if (taskExecutor == nullptr) {
    LOG(ERROR) << "ToMainThread failed: TaskExecutor is not set";
    return;
  }

  if (taskExecutor->isOnTaskThread(rnoh::TaskThread::MAIN)) {
    func(data);
    return;
  }

  taskExecutor->runTask(
      rnoh::TaskThread::MAIN, [func = std::move(func), data]() { func(data); });
}

std::optional<ImageCacheTaskWorker::DelayedTaskHandle>
ImageCacheTaskWorker::RunWithDelay(
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
    rnoh::TaskThread thread,
    std::function<void()> func,
    uint64_t delayMs,
    uint64_t repeatMs) {
  if (func == nullptr || taskExecutor == nullptr) {
    return std::nullopt;
  }
  return taskExecutor->runDelayedTask(
      thread, std::move(func), delayMs, repeatMs);
}

void ImageCacheTaskWorker::CancelDelayedTask(
    const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
    DelayedTaskHandle handle) {
  if (taskExecutor == nullptr) {
    return;
  }
  taskExecutor->cancelDelayedTask(std::move(handle));
}

} // namespace ImageCachePro
