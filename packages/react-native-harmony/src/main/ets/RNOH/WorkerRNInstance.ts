/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type {NapiBridge} from './NapiBridge';
import type {WebSocketModuleOptions} from '../RNOHCorePackage/ts';
import {RNOHError} from './RNOHError';
import {AnyThreadTurboModule, WorkerTurboModule} from './TurboModule';
import type {TurboModuleProvider} from './TurboModuleProvider';
import type {AnyThreadRNInstance} from './RNOHContext';

/**
 * @actor RNOH_LIBRARY
 * @thread: WORKER
 * @brief asynchronous RNInstance to run on a WORKER thread
 */
export class WorkerRNInstance implements AnyThreadRNInstance {
  /**
   * @brief Indicates if RNFunction can be called synchronously.
   * Function calls are queued if false
   */
  protected canCallRNFunction = false;
  /**
   * @brief queued Function calls
   */
  protected rnFunctionCallsQueue: {
    moduleName: string;
    functionName: string;
    args: unknown[];
  }[] = [];
  /**
   * @brief initial Bundle Url
   */
  protected initialBundleUrl: string | undefined = undefined;

  /**
   * @param id - RNInstance Id
   * @param napiBridge - Native API Bridge
   * @param architecture - fixed to 'C_API'
   * @param assetsDest - the path for RN to locate assets
   * @param getTurboModuleProvider - callback to get TurboModuleProvider
   */
  constructor(
    protected id: number,
    protected napiBridge: NapiBridge,
    protected architecture: 'ARK_TS' | 'C_API',
    protected assetsDest: string,
    protected getTurboModuleProvider: () =>
      | TurboModuleProvider<WorkerTurboModule | AnyThreadTurboModule>
      | undefined,
    protected webSocketOptions?: WebSocketModuleOptions,
  ) {}

  /**
   * @brief Calls RCTDeviceEventEmitter.emit to JavaScript
   * @param eventName - event name
   * @param payload - list of arguments
   */
  emitDeviceEvent(eventName: string, payload: any): void {
    this.callRNFunction('RCTDeviceEventEmitter', 'emit', [eventName, payload]);
  }

  /**
   * @brief Calls module.function to JavaScript
   * @param moduleName - module name
   * @param functionName - function name
   * @param args - list of arguments
   */
  callRNFunction(
    moduleName: string,
    functionName: string,
    args: unknown[],
  ): void {
    if (!this.canCallRNFunction) {
      this.rnFunctionCallsQueue.push({moduleName, functionName, args});
      return;
    }
    this.napiBridge.callRNFunction(this.id, moduleName, functionName, args);
  }

  /**
   * @brief post message to Cpp
   * @param name - message name
   * @param payload - message centent
   */
  postMessageToCpp(name: string, payload: any): void {
    this.napiBridge.postMessageToCpp(name, {rnInstanceId: this.id, payload});
  }

  /**
   * @returns current architecture, fixed to 'C_API'
   */
  getArchitecture() {
    return this.architecture;
  }

  /**
   * @template T - turboModule type
   * @param name - turboModule name
   * @returns turboModule instance
   */
  getTurboModule<T>(name: string): T {
    return this.getWorkerTurboModule(name) as T;
  }

  /**
   * @template T - WorkerTurboModule type
   * @param name - WorkerTurboModule name
   * @returns WorkerTurboModule instance
   * @throws RNOHError if failed to get TurboModuleProvider
   */
  getWorkerTurboModule<T extends WorkerTurboModule>(name: string): T {
    const tmProvider = this.getTurboModuleProvider();
    if (!tmProvider) {
      throw new RNOHError({
        whatHappened:
          'TurboModuleProvider is undefined. This should never happen.',
        howCanItBeFixed: [],
      });
    }
    return tmProvider.getModule<T>(name);
  }

  /**
   * @returns the path for RN to locate assets
   */
  getAssetsDest(): string {
    return this.assetsDest;
  }

  /**
   * @returns initial Bundle Url
   */
  getInitialBundleUrl(): string | undefined {
    return this.initialBundleUrl;
  }

  /**
   * @internal
   * @deprecated: This method was added and deprecated when preparing 0.77 version.
   * Instead of using this method, you should use the `setBundleUrl` method in the
   * `WorkerRNInstanceInternal` class.
   */
  setBunleUrl(bundleURL: string) {
    this.initialBundleUrl = this.initialBundleUrl ?? bundleURL;
  }

  /**
   * @returns ID of this RNInstance
   */
  getId(): number {
    return this.id;
  }

  /**
   * @returns the default WebSocket heartbeat configuration provided when creating the RNInstance.
   */
  getWebSocketOptions(): WebSocketModuleOptions | undefined {
    return this.webSocketOptions;
  }
}

/**
 * @internal
 */
export class WorkerRNInstanceInternal extends WorkerRNInstance {
  flushRNFunctionCalls() {
    this.rnFunctionCallsQueue.forEach(({moduleName, functionName, args}) => {
      this.napiBridge.callRNFunction(this.id, moduleName, functionName, args);
    });
    this.rnFunctionCallsQueue = [];
    this.canCallRNFunction = true;
  }

  setBundleUrl(bundleURL: string) {
    this.initialBundleUrl = this.initialBundleUrl ?? bundleURL;
  }
}
