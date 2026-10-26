/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// @ts-ignore
import libRNOHApp from 'librnoh_app.so';
import type {TurboModuleProvider} from './TurboModuleProvider';
import type {StackFrame} from './RNOHError';
import type {Mutation} from './Mutation';
import type {Tag} from './DescriptorBase';
import type {DisplayMode} from './CppBridgeUtils';
import {RNOHLogger} from './RNOHLogger';
import type {
  DisplayMetrics,
  InspectorPackagerConnection,
  InspectorPackagerConnectionDelegate,
  JSEngineName,
} from './types';
import {FatalRNOHError, RNOHError} from './RNOHError';
import type {FrameNodeFactory, JSVMInitOption} from './RNInstance';
import ohosResourceManager from '@ohos.resourceManager';
import {
  AnyThreadTurboModule,
  UITurboModule,
  WorkerTurboModule,
} from './TurboModule';
import {NodeContent} from '@ohos.arkui.node';
import {JsBundle} from './JSBundleProvider';
import {RNOHErrorStack} from './RNOHError';
import {UIContext} from '@kit.ArkUI';
import display from '@ohos.display';
import {deviceInfo} from '@kit.BasicServicesKit';

// Keep in sync with PerformanceMetric in PerformanceMetricsRegistry.h
export interface RNInstancePerformanceMetricsSnapshot {
  BUNDLE_SIZE: number;
  SCRIPT_DOWNLOAD: number;
  SCRIPT_EXECUTION: number;
  REACT_INSTANCE_INIT: number;
  INIT_REACT_RUNTIME: number;
  ROOT_VIEW_TTI: number;
  TURBO_MODULE_SETUP: number;
  APP_STARTUP: number;
  TURBO_MODULE_MAIN_THREAD_COUNT: number;
}

export type CppFeatureFlag =
  | 'PARTIAL_SYNC_OF_DESCRIPTOR_REGISTRY'
  | 'WORKER_THREAD_ENABLED'
  | 'ENABLE_MODAL_CONTENT_SHRINK'
  | 'ENABLE_BACKGROUND_GC'
  | 'ENABLE_PROMISE_REJECT_ERROR_OBJECT';

type RawRNOHError = {
  message: string;
  stacktrace?: string[];
  suggestions?: string[];
  stackFrames?: StackFrame[];
};

type Result<TOK = null> =
  | {
      ok: TOK;
      err: null;
    }
  | {
      ok: null;
      err: RawRNOHError;
    };

export interface ArkTSBridgeHandler {
  getDisplayMetrics: () => DisplayMetrics;
  handleError: (rnohError: RNOHError) => void;
  getMetadata: (name: string) => string;
}

export interface NapiBridgeHandler extends ArkTSBridgeHandler {
  handleError: (rnohError: RNOHError, isDebugModeEnabled?: boolean) => void;
}

export class NapiBridge {
  private logger: RNOHLogger;
  private libRNOHApp: any;

  constructor(logger: RNOHLogger) {
    this.libRNOHApp = libRNOHApp;
    this.logger = logger.clone('NapiBridge');
  }

  private unwrapResult<TOk = null>(result: Result<TOk>): TOk {
    if (result.err) {
      this.logger.error('C++ bridge error: ' + result.err.message);
      throw this.unwrapError(result);
    }
    return result.ok;
  }

  private unwrapError(result: Result<unknown>): RNOHError {
    if (!result.err) {
      this.logger.error('unwrapError called on a non-error result');
      throw new RNOHError({
        whatHappened: "Called unwrapError on result which doesn't have error",
        howCanItBeFixed: [],
      });
    }
    return new RNOHError({
      whatHappened: result.err.message,
      howCanItBeFixed: result.err.suggestions ?? [],
      customStack: (result.err.stacktrace ?? []).join('\n'),
    });
  }

  onInit(
    shouldCleanUpRNInstances: boolean,
    arkTSBridgeHandler: ArkTSBridgeHandler,
  ) {
    if (!this.libRNOHApp) {
      const err = new FatalRNOHError({
        whatHappened:
          "Couldn't create bindings between ETS and CPP. libRNOHApp is undefined.",
        howCanItBeFixed: [],
      });
      this.logger.fatal(err);
      throw err;
    }
    const deviceType = this.getDeviceInfo();
    this.libRNOHApp?.setDeviceInfo(deviceType);
    return this.unwrapResult<{
      isDebugModeEnabled: boolean;
      jsEngineName: JSEngineName;
      envId: number;
    }>(
      this.libRNOHApp?.onInit(shouldCleanUpRNInstances, {
        handleError: async (
          err: RawRNOHError,
          isDebugModeEnabled?: boolean,
        ) => {
          arkTSBridgeHandler.handleError(
            new RNOHError({
              whatHappened: err.message,
              howCanItBeFixed: err.suggestions ?? [],
              customStack: err.stackFrames
                ? await RNOHErrorStack.fromBundledStackFrames(
                    err.stackFrames,
                    isDebugModeEnabled,
                  )
                : (err.stacktrace ?? []).join('\n'),
            }),
          );
        },
        getDisplayMetrics: () => arkTSBridgeHandler.getDisplayMetrics(),
        getMetadata: (name: string) => arkTSBridgeHandler.getMetadata(name),
      } satisfies NapiBridgeHandler),
    );
  }

  registerWorkerTurboModuleProvider(
    turboModuleProvider: TurboModuleProvider<
      WorkerTurboModule | AnyThreadTurboModule
    >,
    rnInstanceId: number,
  ) {
    return this.unwrapResult<undefined>(
      this.libRNOHApp?.registerWorkerTurboModuleProvider(
        turboModuleProvider,
        rnInstanceId,
      ),
    );
  }

  getNextRNInstanceId(): number {
    return this.unwrapResult<number>(this.libRNOHApp?.getNextRNInstanceId());
  }

  createInspectorHostTarget(
    instanceId: number,
    onCppMessage: (type: string, payload: any) => void,
  ) {
    this.libRNOHApp?.createInspectorHostTarget(instanceId, onCppMessage);
  }

  releaseInspectorHostTarget(instanceId: number) {
    this.libRNOHApp?.releaseInspectorHostTarget(instanceId);
  }

  resumeDebugger(instanceId: number) {
    this.libRNOHApp?.resumeDebugger(instanceId);
  }

  onCreateRNInstance(
    envId: number,
    instanceId: number,
    turboModuleProvider: TurboModuleProvider<
      UITurboModule | AnyThreadTurboModule
    >,
    frameNodeFactoryRef: {frameNodeFactory: FrameNodeFactory | null},
    mutationsListener: (mutations: Mutation[]) => void,
    componentCommandsListener: (
      tag: Tag,
      commandName: string,
      args: unknown,
    ) => void,
    onCppMessage: (type: string, payload: any) => void,
    shouldEnableDebugger: boolean,
    cppFeatureFlags: CppFeatureFlag[],
    resourceManager: ohosResourceManager.ResourceManager,
    fontPathByFontFamily: Record<string, string>,
    jsvmInitOptions: ReadonlyArray<JSVMInitOption>,
    hermesMemoryWaterMark: number | undefined,
    hspModuleName: string,
    imageSourceByName: Record<string, string>,
    cacheDir: string,
  ) {
    const cppFeatureFlagStatusByName = cppFeatureFlags.reduce(
      (acc, cppFeatureFlag) => {
        acc[cppFeatureFlag] = true;
        return acc;
      },
      {} as Record<CppFeatureFlag, boolean>,
    );
    const result = this.libRNOHApp?.onCreateRNInstance(
      instanceId,
      turboModuleProvider,
      mutationsListener,
      componentCommandsListener,
      onCppMessage,
      shouldEnableDebugger,
      cppFeatureFlagStatusByName,
      frameNodeFactoryRef,
      resourceManager,
      envId,
      fontPathByFontFamily,
      jsvmInitOptions,
      hermesMemoryWaterMark,
      hspModuleName,
      imageSourceByName,
      cacheDir,
    );
    this.logger.info('C++ RNInstance created, id=' + instanceId);
    return this.unwrapResult(result);
  }

  onDestroyRNInstance(instanceId: number) {
    return this.unwrapResult(this.libRNOHApp?.onDestroyRNInstance(instanceId));
  }

  emitComponentEvent(
    instanceId: number,
    tag: Tag,
    eventEmitRequestHandlerName: string,
    payload: any,
  ) {
    return this.unwrapResult(
      this.libRNOHApp?.emitComponentEvent(
        instanceId,
        tag,
        eventEmitRequestHandlerName,
        payload,
      ),
    );
  }

  loadScript(
    instanceId: number,
    bundle: JsBundle,
    sourceURL: string,
  ): Promise<void> {
    this.logger.info('Loading script: instance=' + instanceId + ', url=' + sourceURL);
    return new Promise((resolve, reject) => {
      const result = this.libRNOHApp?.loadScript(
        instanceId,
        bundle,
        sourceURL,
        (errorMsg: string) => {
          errorMsg ? reject(new Error(errorMsg)) : resolve();
        },
      ) as Result<null>;
      if (result.err) {
        reject(this.unwrapError(result));
      }
    });
  }

  startSurface(
    instanceId: number,
    surfaceTag: number,
    surfaceMinWidth: number,
    surfaceMinHeight: number,
    surfaceMaxWidth: number,
    surfaceMaxHeight: number,
    surfaceOffsetX: number,
    surfaceOffsetY: number,
    pixelRatio: number,
    isRTL: boolean,
    initialProps: any,
    fontSizeMultiplier?: number,
    doLeftAndRightSwapInRTL?: boolean,
  ) {
    this.logger.debug('Starting surface: instance=' + instanceId + ', tag=' + surfaceTag);
    const result = this.libRNOHApp?.startSurface(
      instanceId,
      surfaceTag,
      surfaceMinWidth,
      surfaceMinHeight,
      surfaceMaxWidth,
      surfaceMaxHeight,
      surfaceOffsetX,
      surfaceOffsetY,
      pixelRatio,
      fontSizeMultiplier ?? 1.0,
      isRTL,
      initialProps,
      doLeftAndRightSwapInRTL ?? true,
    );
    return this.unwrapResult(result);
  }

  updateSurfaceConstraints(
    instanceId: number,
    surfaceTag: number,
    surfaceMinWidth: number,
    surfaceMinHeight: number,
    surfaceMaxWidth: number,
    surfaceMaxHeight: number,
    surfaceOffsetX: number,
    surfaceOffsetY: number,
    pixelRatio: number,
    isRTL: boolean,
    fontSizeMultiplier?: number,
    doLeftAndRightSwapInRTL?: boolean,
  ) {
    const result = this.libRNOHApp?.updateSurfaceConstraints(
      instanceId,
      surfaceTag,
      surfaceMinWidth,
      surfaceMinHeight,
      surfaceMaxWidth,
      surfaceMaxHeight,
      surfaceOffsetX,
      surfaceOffsetY,
      pixelRatio,
      fontSizeMultiplier ?? 1.0,
      isRTL,
      doLeftAndRightSwapInRTL ?? true,
    );
    return this.unwrapResult(result);
  }

  measureSurface(
    instanceId: number,
    surfaceTag: number,
    surfaceMinWidth: number,
    surfaceMinHeight: number,
    surfaceMaxWidth: number,
    surfaceMaxHeight: number,
    surfaceOffsetX: number,
    surfaceOffsetY: number,
    pixelRatio: number,
    isRTL: boolean,
    fontSizeMultiplier?: number,
    doLeftAndRightSwapInRTL?: boolean,
  ) {
    const result = this.libRNOHApp?.measureSurface(
      instanceId,
      surfaceTag,
      surfaceMinWidth,
      surfaceMinHeight,
      surfaceMaxWidth,
      surfaceMaxHeight,
      surfaceOffsetX,
      surfaceOffsetY,
      pixelRatio,
      fontSizeMultiplier ?? 1.0,
      isRTL,
      doLeftAndRightSwapInRTL ?? true,
    );
    return this.unwrapResult<{width: number; height: number}>(result);
  }

  createSurface(instanceId: number, surfaceTag: number, appKey: string) {
    const result = this.libRNOHApp?.createSurface(
      instanceId,
      surfaceTag,
      appKey,
    );
    return this.unwrapResult(result);
  }

  setSurfaceProps(
    instanceId: number,
    surfaceTag: number,
    props: Record<string, any>,
  ) {
    const result = this.libRNOHApp?.setSurfaceProps(
      instanceId,
      surfaceTag,
      props,
    );
    return this.unwrapResult(result);
  }

  async stopSurface(instanceId: number, surfaceTag: number) {
    let resolveWait = () => {};
    const wait = new Promise<void>(resolve => {
      resolveWait = () => {
        resolve();
      };
    });
    const result = this.libRNOHApp?.stopSurface(instanceId, surfaceTag, () =>
      resolveWait(),
    );
    this.unwrapResult(result);
    return wait;
  }

  async destroySurface(instanceId: number, surfaceTag: number) {
    const result = this.libRNOHApp?.destroySurface(instanceId, surfaceTag);
    return this.unwrapResult(result);
  }

  setSurfaceDisplayMode(
    instanceId: number,
    surfaceTag: Tag,
    displayMode: DisplayMode,
  ): void {
    const result = this.libRNOHApp?.setSurfaceDisplayMode(
      instanceId,
      surfaceTag,
      displayMode,
    );
    return this.unwrapResult(result);
  }

  callRNFunction(
    instanceId: number,
    moduleName: string,
    functionName: string,
    args: unknown[],
  ): void {
    const result = this.libRNOHApp?.callRNFunction(
      instanceId,
      moduleName,
      functionName,
      args,
    );
    return this.unwrapResult(result);
  }

  onMemoryLevel(level: number): void {
    const result = this.libRNOHApp?.onMemoryLevel(level);
    return this.unwrapResult(result);
  }

  updateState(
    instanceId: number,
    componentName: string,
    tag: Tag,
    state: unknown,
  ): void {
    const result = this.libRNOHApp?.updateState(
      instanceId,
      componentName,
      tag,
      state,
    );
    return this.unwrapResult(result);
  }

  getInspectorPackagerConnection(
    url: string,
    app: string,
    delegate: InspectorPackagerConnectionDelegate,
  ): InspectorPackagerConnection {
    return this.libRNOHApp!.getInspectorPackagerConnection(url, app, delegate);
  }

  getInspectorFlagIsFuseboxEnabled(): boolean {
    return this.libRNOHApp!.getInspectorFlagIsFuseboxEnabled();
  }

  inspectorReporterIsDebuggingEnabled(): boolean {
    return this.libRNOHApp?.inspectorReporterIsDebuggingEnabled() ?? false;
  }

  inspectorReporterReportRequestStart(
    requestId: string,
    requestUrl: string,
    requestMethod: string,
    requestHeaders: Record<string, string>,
    requestBody: string,
    encodedDataLength: number,
  ): void {
    this.libRNOHApp?.inspectorReporterReportRequestStart(
      requestId,
      requestUrl,
      requestMethod,
      requestHeaders,
      requestBody,
      encodedDataLength,
    );
  }

  inspectorReporterReportConnectionTiming(
    requestId: string,
    headers: Record<string, string>,
  ): void {
    this.libRNOHApp?.inspectorReporterReportConnectionTiming(
      requestId,
      headers,
    );
  }

  inspectorReporterReportResponseStart(
    requestId: string,
    requestUrl: string,
    responseStatus: number,
    responseHeaders: Record<string, string>,
    expectedDataLength: number,
  ): void {
    this.libRNOHApp?.inspectorReporterReportResponseStart(
      requestId,
      requestUrl,
      responseStatus,
      responseHeaders,
      expectedDataLength,
    );
  }

  inspectorReporterReportDataReceived(
    requestId: string,
    dataLength: number,
  ): void {
    this.libRNOHApp?.inspectorReporterReportDataReceived(requestId, dataLength);
  }

  inspectorReporterReportResponseEnd(
    requestId: string,
    encodedDataLength: number,
  ): void {
    this.libRNOHApp?.inspectorReporterReportResponseEnd(
      requestId,
      encodedDataLength,
    );
  }

  inspectorReporterReportRequestFailed(
    requestId: string,
    cancelled: boolean,
  ): void {
    this.libRNOHApp?.inspectorReporterReportRequestFailed(requestId, cancelled);
  }

  inspectorReporterMaybeStoreResponseBody(
    requestId: string,
    body: string,
    base64Encoded: boolean,
  ): void {
    this.libRNOHApp?.inspectorReporterMaybeStoreResponseBody(
      requestId,
      body,
      base64Encoded,
    );
  }

  inspectorReporterMaybeStoreResponseBodyIncremental(
    requestId: string,
    data: string,
  ): void {
    this.libRNOHApp?.inspectorReporterMaybeStoreResponseBodyIncremental(
      requestId,
      data,
    );
  }

  postMessageToCpp(name: string, payload: any) {
    const result = this.libRNOHApp?.onArkTSMessage(name, payload);
    return this.unwrapResult(result);
  }

  setImageCacheEnabled(enabled: boolean): void {
    this.libRNOHApp?.setImageCacheEnabled(enabled)
  }

  setParallelizationEnabled(enabled: boolean): void {
    this.libRNOHApp?.setParallelizationEnabled(enabled);
  }

  logMarker(markerId: string, rnInstanceId: number) {
    const result = this.libRNOHApp?.logMarker(markerId, rnInstanceId);
    return this.unwrapResult(result);
  }

  initializePerformanceMetrics(rnInstanceId: number) {
    const result = this.libRNOHApp?.initializePerformanceMetrics(rnInstanceId);
    return this.unwrapResult(result);
  }

  getNativeNodeIdByTag(instanceId: number, tag: Tag): string | undefined {
    const result = this.libRNOHApp?.getNativeNodeIdByTag(instanceId, tag);
    return this.unwrapResult(result);
  }

  registerFont(instanceId: number, fontFamily: string, path: string) {
    return this.unwrapResult(
      this.libRNOHApp?.registerFont(instanceId, fontFamily, path),
    );
  }

  attachRootView(
    instanceId: number,
    surfaceId: number,
    nodeContent: NodeContent,
  ) {
    const result = this.libRNOHApp?.attachRootView(
      instanceId,
      surfaceId,
      nodeContent,
    );
    return this.unwrapResult(result);
  }

  detachRootView(instanceId: number, surfaceId: number) {
    const result = this.libRNOHApp?.detachRootView(instanceId, surfaceId);
    return this.unwrapResult(result);
  }

  setUIContext(instanceId: number, context: UIContext) {
    const result = this.libRNOHApp?.setUIContext(instanceId, context);
    return this.unwrapResult(result);
  }

  startJSFpsMonitor(instanceId: number, cb: (fps: number) => void): void {
    const result = this.libRNOHApp?.startJSFpsMonitor(instanceId, cb);
    return this.unwrapResult(result);
  }

  stopJsFpsMonitor(instanceId: number): void {
    const result = this.libRNOHApp?.stopJsFpsMonitor(instanceId);
    return this.unwrapResult(result);
  }

  getComponentInstancesCount(rnInstanceId: number): number {
    const result = this.libRNOHApp?.getComponentInstancesCount(rnInstanceId);
    return this.unwrapResult(result);
  }

  getJSRuntimeHeapUsage(rnInstanceId: number): number {
    const result = this.libRNOHApp?.getJSRuntimeHeapUsage(rnInstanceId);
    return this.unwrapResult(result);
  }

  getPerformanceMetricsSnapshot(
    rnInstanceId: number,
  ): RNInstancePerformanceMetricsSnapshot | undefined {
    const result = this.libRNOHApp?.getPerformanceMetricsSnapshot(rnInstanceId);
    return this.unwrapResult(result);
  }

  getDeviceInfo(): string {
    const originalDeviceType: string = deviceInfo.deviceType; // 'phone' | 'tablet' | 'pc'
    let deviceType = 'phone';
    // display.isFoldable() can throw 1400003 when the display service is
    // briefly unavailable. This runs inside onInit(), so an uncaught throw
    // here would abort RN instance creation and leave the app on a blank
    // screen; fall back to the non-foldable phone path on failure.
    if (originalDeviceType === 'phone') {
      try {
        if (display.isFoldable()) {
          deviceType = 'isFoldableDevice';
        }
      } catch (e) {
        this.logger.error('display.isFoldable failed: ' + JSON.stringify(e));
      }
    } else {
      deviceType = originalDeviceType;
    }
    return deviceType;
  }
}
