/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include "TaskExecutor/TaskExecutor.h"
namespace rnoh {
void TaskExecutor::runTask(TaskThread, Task&& task) { task(); }
void TaskExecutor::runSyncTask(TaskThread, Task&& task) { task(); }
TaskExecutor::DelayedTask TaskExecutor::runDelayedTask(
    TaskThread thread, Task&&, uint64_t, uint64_t) {
  return DelayedTask(0, thread);
}
void TaskExecutor::cancelDelayedTask(DelayedTask) {}
bool TaskExecutor::isOnTaskThread(TaskThread) const { return true; }
std::optional<TaskThread> TaskExecutor::getCurrentTaskThread() const {
  return TaskThread::MAIN;
}
bool TaskExecutor::isTaskThreadAvailable(TaskThread) const { return true; }
void TaskExecutor::setExceptionHandler(ExceptionHandler) {}
}
