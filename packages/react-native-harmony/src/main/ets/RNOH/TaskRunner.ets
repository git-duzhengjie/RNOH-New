/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/**
 * This file location relative to the package root shouldn't change to avoid introducing breaking changes.
 */
import { lang } from '@kit.ArkTS';
import { RNOHError } from './RNOHError';

type ISendable = lang.ISendable

/**
 * @actor RNOH_LIBRARY
 */
export type SupportedTaskParams = ISendable | undefined

/**
 * @actor RNOH_LIBRARY
 * @brief interface that defines a task to run on main thread
 * @template TContext - Context type
 * @template TParams - parameter type
 * @template TResult - result type
 */
export interface TaskRunnable<TContext, TParams extends SupportedTaskParams = undefined, TResult extends ISendable | void = ISendable | void> extends ISendable {
  /**
   * @brief run the task
   * @param context - context
   * @param params - user-defined params
   * @returns user-defined result
   */
  run(context: TContext, params: TParams): TResult;
}

/**
 * @internal
 */
@Sendable
export class Task<TContext,
TTaskRunnable extends TaskRunnable<TContext, TParams> = TaskRunnable<TContext, SupportedTaskParams>,
TParams extends SupportedTaskParams = SupportedTaskParams> {
  id: string;
  runnable: TTaskRunnable;
  params: TParams;

  constructor(id: string, runnable: TTaskRunnable, params: TParams) {
    this.id = id;
    this.runnable = runnable;
    this.params = params;
  }
}


/**
 * @internal
 */
export abstract class TaskRunner<TContext, TTask extends Task<TContext>> {
  private taskResultResolverByTaskId =
    new Map<string, (result: ISendable | void) => void>();
  private onScheduleTask: (task: TTask) => void;

  constructor(onScheduleTask: (task: TTask) => void) {
    this.onScheduleTask = onScheduleTask;
  }

  run<TParams extends SupportedTaskParams, TResult extends ISendable | void, TRunnable extends TaskRunnable<TContext, TParams>>(task: TTask): Promise<TResult> {
    this.onScheduleTask(task);
    const promisedResultAndPromiseResolver = createPromiseAndResolver<ISendable | void>();
    this.taskResultResolverByTaskId.set(task.id, promisedResultAndPromiseResolver[1]);
    return promisedResultAndPromiseResolver[0] as Promise<TResult>;
  }

  resolve(taskId: string, result: ISendable | void) {
    const resolveResult = this.taskResultResolverByTaskId.get(taskId);
    if (!resolveResult) {
      throw new RNOHError({
        whatHappened: 'Tried to resolve unknown task result ' +
        JSON.stringify({ taskId, registeredTaskIds: Array.from(this.taskResultResolverByTaskId.keys()) }),
        howCanItBeFixed: []
      });
    }
    resolveResult(result);
    this.taskResultResolverByTaskId.delete(taskId);
  }
}

function createPromiseAndResolver<T = Object | undefined>(): [Promise<T>, ((result: T) => void)] {
  let resolve: ((result: T) => void) = (_value: T) => {
  }
  const promise = new Promise<T>((_resolve) => resolve = _resolve);
  return [promise, resolve];
}