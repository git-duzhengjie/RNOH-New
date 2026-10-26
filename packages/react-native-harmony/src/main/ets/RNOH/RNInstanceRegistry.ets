/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import font from '@ohos.font';
import type { RNInstance, RNInstanceOptions } from './RNInstance';
import { RNInstanceImpl } from './RNInstance';
import type { NapiBridge } from './NapiBridge';
import type { UITurboModuleContext } from './TurboModule';
import type { RNOHLogger } from './RNOHLogger';
import { DevToolsController, InternalDevToolsController } from './DevToolsController';
import { DefaultHttpClient, HttpClient } from '../HttpClient';
import { DisplayMetricsManager } from './DisplayMetricsManager';
import resourceManager from '@ohos.resourceManager';
import { WorkerThread } from './WorkerThread';
import common from '@ohos.app.ability.common';
import { RNOHCorePackage } from '../RNOHCorePackage/Package';
import { RNOHError, RNOHErrorEventEmitter } from './RNOHError';
import { JSEngineName } from './types';
import { DevServerHelper } from './DevServerHelper';

const DEFAULT_ASSETS_DEST: string = 'assets/'; // assets destination path "assets/subpath/"

interface CreateWorkerRNInstanceAckPayload {
  rnInstanceId: number;
}

interface DestroyWorkerRNInstanceAckPayload {
  rnInstanceId: number;
}

export class RNInstanceRegistry {
  private instanceMap: Map<number, RNInstanceImpl> = new Map();
  private unregisterMessageListeners: (() => void)[] = [];
  private uiAbilityContext: common.UIAbilityContext;
  private envId: number;
  private isDebugModeEnabled: boolean;
  private jsEngineName: JSEngineName;
  private logger: RNOHLogger;
  private globalRNOHErrorEventEmitter: RNOHErrorEventEmitter;
  private napiBridge: NapiBridge;
  private devToolsController: InternalDevToolsController;
  private createUITurboModuleContext: (rnInstance: RNInstanceImpl) => UITurboModuleContext;
  private defaultHttpClient: HttpClient;
  private resourceManager: resourceManager.ResourceManager;
  private workerThreadPromise: Promise<WorkerThread> | undefined;
  private instanceOptionsMap: Map<number, RNInstanceOptions> = new Map();
  private reactNativeVersion: string;
  private launchUri?: string;
  private cacheDir?: string;
  private onInstanceCreated?: (rnInstance: RNInstanceImpl) => void;

  constructor(
    uiAbilityContext: common.UIAbilityContext,
    envId: number,
    isDebugModeEnabled: boolean,
    jsEngineName: JSEngineName,
    logger: RNOHLogger,
    rnohErrorEventEmitter: RNOHErrorEventEmitter,
    napiBridge: NapiBridge,
    devToolsController: InternalDevToolsController,
    createUITurboModuleContext: (rnInstance: RNInstanceImpl) => UITurboModuleContext,
    defaultHttpClient: HttpClient,
    resourceManager: resourceManager.ResourceManager,
    workerThreadPromise: Promise<WorkerThread> | undefined,
    reactNativeVersion: string,
    onInstanceCreated?: (rnInstance: RNInstanceImpl) => void,
    launchUri?: string,
    cacheDir?: string
  ) {
    this.uiAbilityContext = uiAbilityContext;
    this.envId = envId;
    this.isDebugModeEnabled = isDebugModeEnabled;
    this.jsEngineName = jsEngineName;
    this.logger = logger;
    this.globalRNOHErrorEventEmitter = rnohErrorEventEmitter;
    this.napiBridge = napiBridge;
    this.devToolsController = devToolsController;
    this.createUITurboModuleContext = createUITurboModuleContext;
    this.defaultHttpClient = defaultHttpClient;
    this.resourceManager = resourceManager;
    this.workerThreadPromise = workerThreadPromise;
    this.reactNativeVersion = reactNativeVersion;
    this.launchUri = launchUri;
    this.cacheDir = cacheDir;
    this.onInstanceCreated = onInstanceCreated;
  }

  private async createAndInitRNInstanceImpl(instanceId: number, options: RNInstanceOptions): Promise<RNInstance> {
    const workerThread = await this.workerThreadPromise;
    if (workerThread !== undefined) {
      await this.createRNInstanceEnvOnWorker(workerThread, instanceId, options);
    }
    const fontPathByFontFamily: Record<string, string> = {}
    for (const fontFamilyAndResource of Object.entries(options.fontResourceByFontFamily ?? {})) {
      const fontFamily = fontFamilyAndResource[0];
      const fontResource = fontFamilyAndResource[1];
      if (typeof fontResource === 'string') {
        fontPathByFontFamily[fontFamily] = fontResource;
        font.registerFont({ familyName: fontFamily, familySrc: `file://${fontResource}` });
      } else {
        fontPathByFontFamily[fontFamily] = fontResource.params![0];
        font.registerFont({ familyName: fontFamily, familySrc: fontResource });
      }
    }
    const imageSourceByName: Record<string, string> = {};
    for (const imageNameAndSource of Object.entries(options.imageSourceByName ?? {})) {
      const imageName = imageNameAndSource[0];
      const imageSource = imageNameAndSource[1];
      imageSourceByName[imageName] = imageSource;
    }
    // We have to initialize PerformanceMetricsRegistry before
    // REACT_INSTANCE_INIT_START is logged
    this.napiBridge.initializePerformanceMetrics(instanceId);
    const rnInstance = new RNInstanceImpl(
      this.envId,
      this.isDebugModeEnabled,
      this.jsEngineName,
      instanceId,
      options?.name,
      this.logger,
      this.globalRNOHErrorEventEmitter,
      this.napiBridge,
      options.disableConcurrentRoot,
      this.devToolsController,
      this.createUITurboModuleContext,
      workerThread,
      options.enableDebugger ?? false,
      options?.disablePartialSyncOfDescriptorRegistryInCAPI ?? false,
      options.assetsDest ?? DEFAULT_ASSETS_DEST,
      this.resourceManager,
      fontPathByFontFamily,
      imageSourceByName,
      options?.httpClient ?? this.defaultHttpClient,
      options?.caPathProvider ?? ((url: string) => ''),
      options?.remoteValidation,
      options.backPressHandler,
      options.jsvmInitOptions,
      options.hermesMemoryWaterMark,
      options.hspModuleName,
      options.enableModalContentShrink ?? false,
      options.enableBackgroundGC ?? false,
      options.enablePromiseRejectErrorObject ?? false,
      options.enableBackgroundModeOnInvisible ?? false,
      options.webSocketOptions,
      this.cacheDir,
    );
    rnInstance.setUIAbilityContext(this.uiAbilityContext);
    const packages = options.createRNPackages({});
    packages.unshift(new RNOHCorePackage({}));
    await rnInstance.initialize(packages);
    this.instanceMap.set(instanceId, rnInstance);
    this.onInstanceCreated?.(rnInstance);
    this.logger.info('RNInstance id=' + instanceId + ' initialized');
    return rnInstance;
  }

  public async reloadInstance(rnInstance: RNInstance): Promise<RNInstance> {
    const id = rnInstance.getId();
    this.logger.info('Reloading RNInstance id=' + id);

    const isFuseboxEnabled = this.napiBridge.getInspectorFlagIsFuseboxEnabled();
    if (!isFuseboxEnabled) {
      // Disable debugger to resume the JsVM & avoid thread locks while reloading
      DevServerHelper.disableDebugger();
    }

    if (rnInstance instanceof RNInstanceImpl) {
      await rnInstance.onDestroy(false);
    }
    this.instanceMap.delete(id);
    await this.destroyRNInstanceWorker(id);
    const options = this.instanceOptionsMap.get(id);

    if (!options) {
      this.logger.error('reloadInstance: no RNInstanceOptions found for id=' + id);
      throw new RNOHError({
        whatHappened: 'Attempted to reload RNInstance, but no RNInstanceOptions were found for the given ID.',
        howCanItBeFixed: []
      });
    }

    const newRNInstance = await this.createAndInitRNInstanceImpl(id, options);
    return newRNInstance;
  }

  public async createInstance(options: RNInstanceOptions): Promise<RNInstance> {
    const id = this.napiBridge.getNextRNInstanceId();
    this.logger.info('Creating RNInstance id=' + id);
    if (options.enableDebugger) {
      this.napiBridge.createInspectorHostTarget(id,
        (type: string, payload: string) => {
          this.devToolsController.processInspectorMessage(type, payload);
        });
    }
    this.instanceOptionsMap.set(id, options);
    return this.createAndInitRNInstanceImpl(id, options);
  }

  private async createRNInstanceEnvOnWorker(workerThread: WorkerThread, rnInstanceId: number,
    options: RNInstanceOptions) {
    const logger = this.logger.clone(['RNInstanceRegistry', 'createRNInstanceEnvOnWorker']);
    logger.info("waiting for worker's rnInstance environment");
    setTimeout(() => {
      workerThread.postMessage('RNOH_CREATE_WORKER_RN_INSTANCE', {
        rnInstanceId,
        rnInstanceName: options.name,
        uiAbilityContext: this.uiAbilityContext,
        architecture: 'C_API',
        assetsDest: options.assetsDest ?? DEFAULT_ASSETS_DEST,
        launchUri: this.launchUri,
        reactNativeVersion: this.reactNativeVersion,
        webSocketOptions: options.webSocketOptions,
      });
    }, 0);
    await workerThread.waitForMessage('RNOH_CREATE_WORKER_RN_INSTANCE_ACK',
      (payload: CreateWorkerRNInstanceAckPayload) => payload.rnInstanceId === rnInstanceId);
  }

  public getInstance(id: number): RNInstance {
    return this.instanceMap.get(id)!;
  }

  public async deleteInstance(id: number): Promise<boolean> {
    if (this.instanceMap.has(id)) {
      this.instanceMap.delete(id);
      this.instanceOptionsMap.delete(id);
      this.destroyRNInstanceWorker(id);
      this.logger.info('Deleted RNInstance id=' + id);
      return true;
    }
    this.logger.warn('RNInstance id=' + id + ' not found');
    return false;
  }

  async destroyRNInstanceWorker(id: number) {
    if (this.workerThreadPromise !== undefined) {
      const worker = await this.workerThreadPromise;
      const ack =
        worker.waitForMessage('RNOH_DESTROY_WORKER_RN_INSTANCE_ACK',
          (payload: DestroyWorkerRNInstanceAckPayload) => payload.rnInstanceId === id);
      worker.postMessage('RNOH_DESTROY_WORKER_RN_INSTANCE', { rnInstanceId: id });
      await ack;
    }
  }

  public forEach(cb: (rnInstance: RNInstanceImpl) => void) {
    this.instanceMap.forEach(cb);
  }

  onDestroy() {
    this.unregisterMessageListeners.forEach(unregister => unregister());
  }
}
