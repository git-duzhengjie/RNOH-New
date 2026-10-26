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

#ifndef IMAGE_CACHE_TASK_WORKER_H
#define IMAGE_CACHE_TASK_WORKER_H

#include <functional>
#include <memory>
#include <optional>
#include "RNOH/TaskExecutor/TaskExecutor.h"

namespace ImageCachePro {

class ImageCacheTaskInternal;

class ImageCacheTaskWorker {
 public:
  using AsyncTask =
      std::function<void(std::shared_ptr<ImageCacheTaskInternal> task)>;
  using AsyncFunc = std::function<void(void* data)>;
  using DelayedTaskHandle = rnoh::TaskExecutor::DelayedTask;

  static bool IsMainThread(
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor);

  static bool PushTask(
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
      AsyncTask execute,
      AsyncTask complete,
      std::shared_ptr<ImageCacheTaskInternal> task);
  static bool PushTask(
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
      AsyncFunc execute,
      AsyncFunc complete,
      void* data);
  static void ToMainThread(
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
      std::function<void(void*)> func,
      void* data);
  static std::optional<DelayedTaskHandle> RunWithDelay(
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
      rnoh::TaskThread thread,
      std::function<void()> func,
      uint64_t delayMs,
      uint64_t repeatMs = 0);
  static void CancelDelayedTask(
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
      DelayedTaskHandle handle);
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_TASK_WORKER_H
