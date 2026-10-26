/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { lang } from '@kit.ArkTS';
import { UITurboModuleContext } from './RNOHContext';
import { SupportedTaskParams, Task, TaskRunnable, TaskRunner } from './TaskRunner';

type ISendable = lang.ISendable

/**
 * @actor RNOH_LIBRARY
 * @deprecated Use `SupportedTaskParams` instead (latestRNOHVersion: 0.72.40)
 */
export type SupportedUITaskParams = SupportedTaskParams

/**
 * @internal
 */
export class UITaskRunner extends TaskRunner<UITurboModuleContext, UITask> {
  private id: string
  private nextTaskId = 0;

  constructor(id: string, onScheduleUITask: (uiTask: UITask) => void) {
    super(onScheduleUITask)
    this.id = id
  }

  createTask<TParams extends SupportedTaskParams,
  TResult extends void | lang.ISendable,
  TRunnable extends UITaskRunnable<TParams, TResult>>(
    runnable: TRunnable,
    params: TParams): UITask<TParams, TRunnable> {
    return new UITask(`${this.id}::${this.nextTaskId++}`, runnable, params);
  }
}

/**
 * @actor RNOH_LIBRARY
 * @template TParams The type of the parameters to pass to the task.
 * @template TResult The type of the result of the task.
 */
export interface UITaskRunnable<TParams extends SupportedTaskParams = undefined, TResult extends ISendable | void = ISendable | void> extends TaskRunnable<UITurboModuleContext, TParams, TResult> {
  
  /**
   * Runs the task on the WorkerTurboModule thread.
   * @param uiTurboModuleContext The `EtsUITurboModuleContext` or `RNComponentContext` to run the task on.
   * @param params The parameters to pass to the task.
   * @returns The result of the task.
   */
  run(uiTurboModuleContext: UITurboModuleContext, params: TParams): TResult;
}

/**
 * @internal
 */
@Sendable
export class UITask<TParams extends SupportedTaskParams = SupportedTaskParams,
TUITaskRunnable extends UITaskRunnable<TParams> = UITaskRunnable<TParams>>
  extends Task<UITurboModuleContext, TUITaskRunnable, TParams> {
  constructor(id: string, runnable: TUITaskRunnable, params: TParams) {
    super(id, runnable, params)
  }
}
