/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOH/MessageQueueThread.h"
#include <cxxreact/TraceSection.h>
#include <glog/logging.h>

using namespace rnoh;

MessageQueueThread::MessageQueueThread(TaskExecutor::Shared const& taskExecutor)
    : taskExecutor(taskExecutor) {}

MessageQueueThread::~MessageQueueThread() {
  DLOG(INFO) << "~MessageQueueThread";
};

void MessageQueueThread::runOnQueue(std::function<void()>&& func) {
  facebook::react::TraceSection s("#RNOH::MessageQueueThread::runOnQueue");
  taskExecutor->runTask(TaskThread::JS, std::move(func));
}

void MessageQueueThread::runOnQueueSync(std::function<void()>&& func) {
  facebook::react::TraceSection s("#RNOH::MessageQueueThread::runOnQueueSync");
  // NOTE: we store the shared_ptr in a local variable to ensure that the
  // TaskExecutor is not destroyed before the task is executed.
  auto taskExecutor = this->taskExecutor;
  taskExecutor->runSyncTask(TaskThread::JS, std::move(func));
}

void MessageQueueThread::quitSynchronous() {
  // TODO!
}
