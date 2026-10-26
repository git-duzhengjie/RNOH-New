/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { UITurboModuleContext, WorkerTurboModuleContext } from './RNOHContext';
import type { SupportedTaskParams, } from './TaskRunner';
import type { UITaskRunnable, UITaskRunner, } from './UITaskRunner';
import type { WorkerTaskRunnable, WorkerTaskRunner } from './WorkerTaskRunner';
import type { WorkerTurboModuleContextDependencies } from './RNOHContext';
import { RNOHContext } from './RNOHContext';
import { DescriptorWrapper, Tag } from './DescriptorBase';
import { RNComponentDataSource } from './RNComponentDataSource';
import { RNOHPackage } from './RNOHPackage';
import { RNInstanceImpl } from './RNInstance';
import { Content } from '@ohos.arkui.node';
import lang from '@arkts.lang';

export interface EtsUITurboModuleContextDependencies {
  rnohContext: RNOHContext;
}

/**
 * @actor RNOH_LIBRARY
 * non-cpp TurboModule Context for UI thread
 */
export class EtsUITurboModuleContext extends UITurboModuleContext {
  private deps: EtsUITurboModuleContextDependencies;

  /**
   * @internal
   */
  constructor(deps: EtsUITurboModuleContextDependencies) {
    super(deps.rnohContext);
    this.deps = deps;
  }

  /**
   * @brief run the task on WORKER thread
   * @template TParams - parameter type
   * @template TResult - result type
   * @template TRunnable - runnable Object type
   * @param runnable - runnable Object
   * @param params - user-defined params
   * @returns user-defined result Promise
   */
  public async runOnWorkerThread<TParams extends SupportedTaskParams, TResult extends lang.ISendable | void, TRunnable extends WorkerTaskRunnable<TParams, TResult>>(runnable: TRunnable,
    params: TParams): Promise<TResult> {
    const workerTaskRunner = this.erasedWorkerTaskRunner as WorkerTaskRunner;
    const rnInstanceId = this.rnInstance.getId();
    return workerTaskRunner.run(workerTaskRunner.createTask(rnInstanceId, runnable, params));
  }
}

/**
 * @internal
 */
export interface EtsWorkerTurboModuleContextDependencies {
  workerTurboModuleContextDependencies: WorkerTurboModuleContextDependencies;
  uiTaskRunner: UITaskRunner;
}

/**
 * @actor RNOH_LIBRARY
 * non-cpp TurboModule Context for Worker thread
 */
export class EtsWorkerTurboModuleContext extends WorkerTurboModuleContext {
  private deps: EtsWorkerTurboModuleContextDependencies;

  /**
   * @internal
   */
  constructor(deps: EtsWorkerTurboModuleContextDependencies) {
    super(deps.workerTurboModuleContextDependencies);
    this.deps = deps;
  }

  /**
   * @brief run the task on UI thread
   * @template TParams - parameter type
   * @template TResult - result type
   * @template TRunnable - runnable Object type
   * @param runnable - runnable Object
   * @param params - user-defined params
   * @returns user-defined result Promise
   */
  async runOnUIThread<TParams extends SupportedTaskParams, TResult extends lang.ISendable | void, TRunnable extends UITaskRunnable<TParams, TResult>>(runnable: TRunnable,
    params: TParams): Promise<TResult> {
    const uiTaskRunner = this.deps.uiTaskRunner;
    return uiTaskRunner.run(uiTaskRunner.createTask(runnable, params));
  }
}

/**
 * @actor RNOH_APP
 * @deprecated It was deprecated when preparing 0.77 branch for release.
 */
interface DescriptorEssence {
  tag: Tag,
  type: string;
}

/**
 * @actor RNOH_APP
 */
export type CustomComponentBuilder = (ctx: ComponentBuilderContext) => void

/**
 * @actor RNOH_APP
 */
export type WrappedCustomRNComponentBuilder = WrappedBuilder<[ComponentBuilderContext]>

/**
 * @actor RNOH_APP
 */
export type WrappedRNChildrenBuilder = WrappedBuilder<[RNComponentContext, Tag]>

/**
 * @actor RNOH_APP
 */
export type WrappedRNComponentBuilder = WrappedBuilder<[RNComponentContext, Tag]>

/**
 * @actor RNOH_APP
 */
export interface RNComponentDataSourceFactoryContext {
  tag: Tag;
}

/**
 * @actor RNOH_APP
 */
export type RNComponentDataSourceFactory = (ctx: RNComponentDataSourceFactoryContext) => RNComponentDataSource

/**
 * @api
 */
export type RNArkTSComponentDelegate = Record<string, Function>;

/**
 * @actor RNOH_LIBRARY
 *
 * @brief RNComponentContext Provides utilities, and dependencies useful for
 * RNComponents.
 */
export class RNComponentContext extends RNOHContext {
  /**
   * @internal
   * This wrapped builder uses components registered in RNOHPackage.
   */
  public wrappedCustomRNComponentBuilder: WrappedCustomRNComponentBuilder;

  /**
   * @internal
   * WrappedCustomRNComponentBuilder provided by the application developer.
   */
  public customWrappedCustomRNComponentBuilder: WrappedCustomRNComponentBuilder | undefined;

  /**
   * @brief Builder used to construct RN children components.
   */
  public wrappedRNChildrenBuilder: WrappedRNChildrenBuilder;

  /**
   * @brief Builder for native RN components on the ArkTS side.
   */
  public wrappedRNComponentBuilder: WrappedRNComponentBuilder;

  private dataSourceFactoriesByType: Map<string, RNComponentDataSourceFactory>;

  private arkTSComponentDelegateByTag: Map<Tag, RNArkTSComponentDelegate> = new Map();

  /**
   * Creates a data source for a component based on its tag.
   * Falls back to default RNComponentDataSource if no factory is found.
   *
   * @param ctx - Context object containing tag and other metadata.
   * @returns The instantiated RNComponentDataSource or a custom data source.
   */
  public createComponentDataSource(ctx: RNComponentDataSourceFactoryContext) {
    const descriptorWrapper = this.descriptorRegistry.findDescriptorWrapperByTag(ctx.tag);
    const createComponentDataSource = descriptorWrapper && this.dataSourceFactoriesByType.get(descriptorWrapper.type_);
    return createComponentDataSource ? createComponentDataSource(ctx) : new RNComponentDataSource(this, ctx.tag);
  }

  /**
   * @brief Sets the delegate for a specified ArkTS component identified by tag.
   *
   * @param tag - Component instance identifier
   * @param delegate - Delegate is a collection of external interfaces provided by ArkTS components,
   *                   through which internal methods of the component can be invoked
   */
  public setArkTSComponentDelegateForTag(tag: Tag, delegate: RNArkTSComponentDelegate) {
    this.arkTSComponentDelegateByTag.set(tag, delegate);
  }

  /**
   * @internal
   */
  public __getArkTSComponentDelegateForTag(tag: Tag): RNArkTSComponentDelegate | undefined {
    return this.arkTSComponentDelegateByTag.get(tag);
  }

  /**
   * @internal
   */
  public __deleteArkTSComponentDelegateForTag(tag: Tag) {
    this.arkTSComponentDelegateByTag.delete(tag);
  }

  private contentByTag: Map<Tag, Content> = new Map();

  /**
   * @brief Retrieves the content instance associated with a specific tag.
   *
   * @param tag - Component instance identifier
   * @returns The content object if available, otherwise undefined.
   */
  public getContentForTag(tag: Tag): Content | undefined {
    return this.contentByTag.get(tag);
  }

  /**
   * @brief Runs a task on the WORKER thread and returns the result
   * asynchronously.
   * Requires the worker thread feature to be enabled.
   *
   * @param runnable - The task object implementing WorkerTaskRunnable interface.
   * @param params - The parameters to be passed to the task.
   * @returns Promise resolving to the result of the task.
   */
  public async runOnWorkerThread<TParams extends SupportedTaskParams, TResult extends lang.ISendable | void, TRunnable extends WorkerTaskRunnable<TParams, TResult>>(runnable: TRunnable,
    params: TParams): Promise<TResult> {
    const workerTaskRunner = this.erasedWorkerTaskRunner as WorkerTaskRunner;
    const rnInstanceId = this.rnInstance.getId();
    return workerTaskRunner.run(workerTaskRunner.createTask(rnInstanceId, runnable, params));
  }

  /**
   * @internal
   */
  public __setContentForTag(tag: Tag, content: Content) {
    this.contentByTag.set(tag, content);
  }

  /**
   * @internal
   */
  public __deleteContentForTag(tag: Tag) {
    this.contentByTag.delete(tag);
  }

  /**
   * @actor RNOH_APP
   */
  constructor(
    rnohContext: RNOHContext,
    wrappedCustomRNComponentBuilder: WrappedCustomRNComponentBuilder,
    wrappedRNComponentBuilder: WrappedRNComponentBuilder,
    rnComponentDataSourceFactoriesByDescriptorType: Map<string, RNComponentDataSourceFactory>,
    customWrappedCustomRNComponentBuilder: WrappedCustomRNComponentBuilder | undefined = undefined,
  ) {
    super(
      rnohContext._rnohContextDeps
    );
    this.wrappedCustomRNComponentBuilder = wrappedCustomRNComponentBuilder;
    this.wrappedRNChildrenBuilder = wrapBuilder(buildRNChildren);
    this.wrappedRNComponentBuilder = wrappedRNComponentBuilder;
    this.dataSourceFactoriesByType = rnComponentDataSourceFactoriesByDescriptorType;
    this.customWrappedCustomRNComponentBuilder = customWrappedCustomRNComponentBuilder;
  }
}

@Builder
function buildRNChildren(ctx: RNComponentContext, parentTag: Tag) {
  Stack() {
    LazyForEach(ctx.createComponentDataSource({ tag: parentTag }),
      (descriptorWrapper: DescriptorWrapper) => {
        ctx.wrappedRNComponentBuilder.builder(ctx, descriptorWrapper.tag);
      },
      (descriptorWrapper: DescriptorWrapper) => descriptorWrapper.tag.toString() + '@' + descriptorWrapper.renderKey
    );
  }
}

/**
 * @actor RNOH_APP
 *
 * @brief Provides context information required when constructing a component.
 */
export class ComponentBuilderContext {
  /**
   * @brief The current RNComponentContext instance associated with this
   * component.
   * Preferred over the deprecated `rnohContext`.
   */
  public rnComponentContext: RNComponentContext;

  /**
   * @deprecated Use `rnComponentContext` instead.
   * It was deprecated when preparing 0.77 branch for release.
   */
  public rnohContext: RNOHContext;

  /**
   * @brief Unique identifier for this component instance, assigned by the
   * React Native runtime.
   */
  public tag: Tag;

  /**
   * @brief Name of the component being constructed.
   */
  public componentName: string;

  /**
   * @deprecated: Use tag and componentName instead.
   * It was deprecated when preparing 0.77 branch for release.
   */
  public descriptor: DescriptorEssence;
  
  /**
   * @internal
   */
  public customRNComponentWrappedBuilderByName = new Map<string, WrappedBuilder<[ComponentBuilderContext]>>();

  /**
   * @internal
   */
  constructor(rnComponentContext: RNComponentContext, rnohContext: RNOHContext, tag: Tag, componentName: string,
    descriptor: DescriptorEssence) {
    this.rnComponentContext = rnComponentContext;
    this.rnohContext = rnohContext;
    this.tag = tag;
    this.componentName = componentName;
    this.descriptor = descriptor;
    const rnInstance = this.rnComponentContext.rnInstance;
    if (rnInstance instanceof RNInstanceImpl) {
      this.customRNComponentWrappedBuilderByName = rnInstance.getPackages().flatMap(pkg => {
        if (!(pkg instanceof RNOHPackage)) {
          return [];
        }
        return pkg.createWrappedCustomRNComponentBuilderByComponentNameMap();
      }).reduce(mergeMaps, new Map<string, WrappedBuilder<[ComponentBuilderContext]>>());
    }
  }
}

function mergeMaps<K, V>(map1: Map<K, V>, map2: Map<K, V>): Map<K, V> {
  const mergedMap = new Map<K, V>(map1);
  for (const keyAndValue of map2) {
    mergedMap.set(keyAndValue[0], keyAndValue[1]);
  }
  return mergedMap;
}