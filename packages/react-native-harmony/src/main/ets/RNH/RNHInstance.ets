/**
 * RNOH-New: RNHInstance - TypeScript-side instance manager
 *
 * Manages the RNH native instance from the ArkTS/JS side.
 * Communicates with the C++ side via NAPI.
 */

import { RNHLogger } from './RNHLogger';
import { RNHErrorBoundary } from './RNHErrorBoundary';
import { RNHMemoryGuard } from './RNHMemoryGuard';
import { RNHTurboModuleGuard } from './RNHTurboModuleGuard';
import { RNHInstanceLifecycle, RNHInstanceState } from './RNHInstanceLifecycle';
import { RNHSurfacePool } from './RNHSurfacePool';
import rnh from 'librnh.so';

const TAG = 'RNHInstance';

/**
 * Native-side instance configuration
 */
export interface RNHInstanceNativeConfig {
  bundlePath: string;
  bundleUrl: string;
  enableHermes: boolean;
  enableFabric: boolean;
  enableTurboModules: boolean;
  enableBridgeless: boolean;
  enableInspector: boolean;
  enableRemoteDebugging: boolean;
  inspectorPort: number;
  viewportWidth: number;
  viewportHeight: number;
  pixelRatio: number;
}

/**
 * NAPI native module interface.
 * These signatures must match the C++ implementation in librnh.so.
 */
interface RNHNativeModule {
  createInstance(config: RNHInstanceNativeConfig): number;
  destroyInstance(instanceId: number): void;
  loadBundle(instanceId: number, path: string): boolean;
  loadBundleFromUrl(instanceId: number, url: string): boolean;
  startSurface(instanceId: number, surfaceId: number, moduleName: string,
               width: number, height: number, pixelRatio: number): void;
  stopSurface(instanceId: number, surfaceId: number): void;
  updateSurfaceSize(instanceId: number, surfaceId: number,
                    width: number, height: number): void;
  emitEvent(instanceId: number, surfaceId: number,
            eventName: string, payload: string): void;
  sendTouchEvent(instanceId: number, surfaceId: number,
                 action: number, x: number, y: number, timestamp: number): void;
}

// Cast the imported native module to our typed interface
const native: RNHNativeModule = rnh as RNHNativeModule;

/**
 * RNH Instance - manages the lifecycle of one RN runtime
 */
export class RNHInstance {
  private instanceId: number = -1;
  private config: RNHInstanceNativeConfig;
  private surfaces: Set<number> = new Set();

  // Stability guards
  private errorBoundary: RNHErrorBoundary = new RNHErrorBoundary();
  private memoryGuard: RNHMemoryGuard = new RNHMemoryGuard();
  private turboModuleGuard: RNHTurboModuleGuard = new RNHTurboModuleGuard();
  private lifecycle: RNHInstanceLifecycle = new RNHInstanceLifecycle();
  private surfacePool: RNHSurfacePool = new RNHSurfacePool(3);

  constructor(config: RNHInstanceNativeConfig) {
    this.config = config;
    this.createNativeInstance();
  }

  private createNativeInstance(): void {
    this.lifecycle.assertState([RNHInstanceState.Uninitialized]);
    try {
      RNHLogger.info(TAG, 'Creating native RNH instance...');
      this.instanceId = native.createInstance(this.config);
      this.lifecycle.setInstanceId(this.instanceId);
      this.lifecycle.transitionTo(RNHInstanceState.Initialized);
      RNHLogger.info(TAG, `Native instance created, id: ${this.instanceId}`);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to create native instance: ${e}`);
      throw e;
    }
  }

  /**
   * Load a JS bundle from a local path
   */
  loadBundle(path: string): boolean {
    this.lifecycle.assertState([RNHInstanceState.Initialized, RNHInstanceState.Started, RNHInstanceState.Stopped]);
    RNHLogger.info(TAG, `Loading bundle: ${path}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot load bundle');
      return false;
    }
    return this.errorBoundary.wrapCallback<boolean>(() => {
      const result = native.loadBundle(this.instanceId, path);
      RNHLogger.info(TAG, `Bundle loaded: ${result}`);
      return result;
    }, 'loadBundle') ?? false;
  }

  /**
   * Load a JS bundle from a Metro server URL
   */
  loadBundleFromUrl(url: string): boolean {
    this.lifecycle.assertState([RNHInstanceState.Initialized, RNHInstanceState.Started, RNHInstanceState.Stopped]);
    RNHLogger.info(TAG, `Loading bundle from URL: ${url}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot load bundle from URL');
      return false;
    }
    return this.errorBoundary.wrapCallback<boolean>(() => {
      const result = native.loadBundleFromUrl(this.instanceId, url);
      RNHLogger.info(TAG, `Bundle from URL loaded: ${result}`);
      return result;
    }, 'loadBundleFromUrl') ?? false;
  }

  /**
   * Start a new surface (root view)
   */
  startSurface(surfaceId: number, moduleName: string,
               width: number, height: number, pixelRatio: number): void {
    this.lifecycle.assertState([RNHInstanceState.Initialized, RNHInstanceState.Started, RNHInstanceState.Stopped]);
    RNHLogger.info(TAG, `Starting surface ${surfaceId}, module: ${moduleName}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot start surface');
      return;
    }
    this.errorBoundary.wrapCallback<void>(() => {
      native.startSurface(this.instanceId, surfaceId, moduleName, width, height, pixelRatio);
      this.surfaces.add(surfaceId);
      this.memoryGuard.registerSurface(surfaceId);
      // Transition to Started state on first surface start
      if (this.lifecycle.isInState([RNHInstanceState.Initialized, RNHInstanceState.Stopped])) {
        this.lifecycle.transitionTo(RNHInstanceState.Started);
      }
    }, 'startSurface');
  }

  /**
   * Stop a surface
   */
  stopSurface(surfaceId: number): void {
    this.lifecycle.assertState([RNHInstanceState.Started, RNHInstanceState.Stopped]);
    RNHLogger.info(TAG, `Stopping surface ${surfaceId}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot stop surface');
      return;
    }
    this.errorBoundary.wrapCallback<void>(() => {
      native.stopSurface(this.instanceId, surfaceId);
      this.surfaces.delete(surfaceId);
      this.memoryGuard.cleanupSurface(surfaceId);
      // Transition to Stopped state when all surfaces are stopped
      if (this.surfaces.size === 0 && this.lifecycle.isInState([RNHInstanceState.Started])) {
        this.lifecycle.transitionTo(RNHInstanceState.Stopped);
      }
    }, 'stopSurface');
  }

  /**
   * Update surface dimensions
   */
  updateSurfaceSize(surfaceId: number, width: number, height: number): void {
    if (this.instanceId < 0 || !this.lifecycle.isUsable()) {
      return;
    }
    this.errorBoundary.wrapCallback<void>(() => {
      native.updateSurfaceSize(this.instanceId, surfaceId, width, height);
    }, 'updateSurfaceSize');
  }

  /**
   * Emit an event to JS
   */
  emitEvent(surfaceId: number, eventName: string, payload: string): void {
    if (this.instanceId < 0 || !this.lifecycle.isUsable()) {
      return;
    }
    this.errorBoundary.wrapCallback<void>(() => {
      native.emitEvent(this.instanceId, surfaceId, eventName, payload);
    }, 'emitEvent');
  }

  /**
   * Send a touch event to JS
   */
  sendTouchEvent(surfaceId: number, action: number,
                 x: number, y: number, timestamp: number): void {
    if (this.instanceId < 0 || !this.lifecycle.isUsable()) {
      return;
    }
    this.errorBoundary.wrapCallback<void>(() => {
      native.sendTouchEvent(this.instanceId, surfaceId, action, x, y, timestamp);
    }, 'sendTouchEvent');
  }

  /**
   * Destroy the instance and free resources
   */
  destroy(): void {
    if (this.lifecycle.isDestroyed()) {
      RNHLogger.warn(TAG, 'Instance already destroyed');
      return;
    }
    RNHLogger.info(TAG, 'Destroying RNH instance...');

    // Stop all surfaces and clean up memory
    this.memoryGuard.cleanupAll();
    this.surfaces.clear();

    // Destroy native instance
    if (this.instanceId >= 0) {
      this.errorBoundary.wrapCallback<void>(() => {
        native.destroyInstance(this.instanceId);
      }, 'destroyInstance');
      this.instanceId = -1;
    }

    // Clean up stability guards
    this.errorBoundary.destroy();
    this.turboModuleGuard.destroy();
    this.surfacePool.destroy();

    this.lifecycle.transitionTo(RNHInstanceState.Destroyed);
    RNHLogger.info(TAG, 'RNH instance destroyed');
  }

  /**
   * Check if the instance is valid
   */
  isValid(): boolean {
    return this.instanceId >= 0 && this.lifecycle.isUsable();
  }

  /**
   * Get the error boundary for this instance
   */
  getErrorBoundary(): RNHErrorBoundary {
    return this.errorBoundary;
  }

  /**
   * Get the memory guard for this instance
   */
  getMemoryGuard(): RNHMemoryGuard {
    return this.memoryGuard;
  }

  /**
   * Get the TurboModule guard for this instance
   */
  getTurboModuleGuard(): RNHTurboModuleGuard {
    return this.turboModuleGuard;
  }

  /**
   * Get the lifecycle state machine for this instance
   */
  getLifecycle(): RNHInstanceLifecycle {
    return this.lifecycle;
  }

  /**
   * Get the surface pool for this instance
   */
  getSurfacePool(): RNHSurfacePool {
    return this.surfacePool;
  }

  /**
   * Get the instance ID
   */
  getId(): number {
    return this.instanceId;
  }
}
