/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "TurboModuleHostMocks.h"

#include "RNOH/TaskExecutor/TaskExecutor.h"
#include <react/nativemodule/core/ReactCommon/TurboModuleUtils.h>

using rnoh::testing::turboModuleMockState;

namespace facebook::react {
Promise::Promise(jsi::Runtime& rt, jsi::Function resolve, jsi::Function reject)
    : LongLivedObject(rt),
      resolve_(std::move(resolve)),
      reject_(std::move(reject)) {}

void Promise::resolve(const jsi::Value& /*result*/) {}
void Promise::reject(const std::string& /*message*/) {}
jsi::Value createPromiseAsJSIValue(
    jsi::Runtime& /*rt*/,
    PromiseSetupFunctionType&& /*func*/) {
  return jsi::Value::undefined();
}
} // namespace facebook::react

namespace rnoh {

TaskExecutor::TaskExecutor(
    napi_env /*mainEnv*/,
    std::unique_ptr<AbstractTaskRunner> /*workerTaskRunner*/) {}

TaskExecutor::~TaskExecutor() noexcept {}

void TaskExecutor::runTask(TaskThread /*thread*/, Task&& task) {
  turboModuleMockState().runTaskCount++;
  if (turboModuleMockState().executeLambdas) {
    try {
      task();
    } catch (...) {
      // Swallow exceptions from stub NAPI calls
    }
  }
}

void TaskExecutor::runSyncTask(TaskThread /*thread*/, Task&& task) {
  turboModuleMockState().runSyncTaskCount++;
  if (turboModuleMockState().executeLambdas) {
    try {
      task();
    } catch (...) {
      // Swallow exceptions from stub NAPI calls
    }
  }
}

TaskExecutor::DelayedTask TaskExecutor::runDelayedTask(
    TaskThread /*thread*/,
    Task&& /*task*/,
    uint64_t /*delayMs*/,
    uint64_t /*repeatMs*/) {
  return {0, TaskThread::MAIN};
}

void TaskExecutor::cancelDelayedTask(DelayedTask /*taskId*/) {}

bool TaskExecutor::isOnTaskThread(TaskThread /*thread*/) const {
  return false;
}

std::optional<TaskThread> TaskExecutor::getCurrentTaskThread() const {
  return std::nullopt;
}

bool TaskExecutor::isTaskThreadAvailable(TaskThread /*thread*/) const {
  return false;
}

void TaskExecutor::setExceptionHandler(ExceptionHandler /*handler*/) {}

AbstractTaskRunner::Shared TaskExecutor::getTaskRunner(
    TaskThread /*taskThread*/) const {
  return nullptr;
}

void TaskExecutor::setTaskThreadPriority(QoS_Level /*level*/) {}

} // namespace rnoh
