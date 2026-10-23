/**
 * RNOH-New: RNHInstance - TypeScript-side instance manager
 *
 * Manages the RNH native instance from the ArkTS/JS side.
 * Communicates with the C++ side via NAPI.
 */

import { RNHLogger } from './RNHLogger';
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

  constructor(config: RNHInstanceNativeConfig) {
    this.config = config;
    this.createNativeInstance();
  }

  private createNativeInstance(): void {
    try {
      RNHLogger.info(TAG, 'Creating native RNH instance...');
      this.instanceId = native.createInstance(this.config);
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
    RNHLogger.info(TAG, `Loading bundle: ${path}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot load bundle');
      return false;
    }
    try {
      const result = native.loadBundle(this.instanceId, path);
      RNHLogger.info(TAG, `Bundle loaded: ${result}`);
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `Failed to load bundle: ${e}`);
      return false;
    }
  }

  /**
   * Load a JS bundle from a Metro server URL
   */
  loadBundleFromUrl(url: string): boolean {
    RNHLogger.info(TAG, `Loading bundle from URL: ${url}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot load bundle from URL');
      return false;
    }
    try {
      const result = native.loadBundleFromUrl(this.instanceId, url);
      RNHLogger.info(TAG, `Bundle from URL loaded: ${result}`);
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `Failed to load bundle from URL: ${e}`);
      return false;
    }
  }

  /**
   * Start a new surface (root view)
   */
  startSurface(surfaceId: number, moduleName: string,
               width: number, height: number, pixelRatio: number): void {
    RNHLogger.info(TAG, `Starting surface ${surfaceId}, module: ${moduleName}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot start surface');
      return;
    }
    try {
      native.startSurface(this.instanceId, surfaceId, moduleName, width, height, pixelRatio);
      this.surfaces.add(surfaceId);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to start surface: ${e}`);
    }
  }

  /**
   * Stop a surface
   */
  stopSurface(surfaceId: number): void {
    RNHLogger.info(TAG, `Stopping surface ${surfaceId}`);
    if (this.instanceId < 0) {
      RNHLogger.warn(TAG, 'Instance not created, cannot stop surface');
      return;
    }
    try {
      native.stopSurface(this.instanceId, surfaceId);
      this.surfaces.delete(surfaceId);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to stop surface: ${e}`);
    }
  }

  /**
   * Update surface dimensions
   */
  updateSurfaceSize(surfaceId: number, width: number, height: number): void {
    if (this.instanceId < 0) {
      return;
    }
    try {
      native.updateSurfaceSize(this.instanceId, surfaceId, width, height);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to update surface size: ${e}`);
    }
  }

  /**
   * Emit an event to JS
   */
  emitEvent(surfaceId: number, eventName: string, payload: string): void {
    if (this.instanceId < 0) {
      return;
    }
    try {
      native.emitEvent(this.instanceId, surfaceId, eventName, payload);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to emit event: ${e}`);
    }
  }

  /**
   * Send a touch event to JS
   */
  sendTouchEvent(surfaceId: number, action: number,
                 x: number, y: number, timestamp: number): void {
    if (this.instanceId < 0) {
      return;
    }
    try {
      native.sendTouchEvent(this.instanceId, surfaceId, action, x, y, timestamp);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to send touch event: ${e}`);
    }
  }

  /**
   * Destroy the instance and free resources
   */
  destroy(): void {
    RNHLogger.info(TAG, 'Destroying RNH instance...');
    // Stop all surfaces
    for (const surfaceId of this.surfaces) {
      this.stopSurface(surfaceId);
    }
    this.surfaces.clear();

    // Destroy native instance
    if (this.instanceId >= 0) {
      try {
        native.destroyInstance(this.instanceId);
      } catch (e) {
        RNHLogger.error(TAG, `Failed to destroy instance: ${e}`);
      }
      this.instanceId = -1;
    }
    RNHLogger.info(TAG, 'RNH instance destroyed');
  }

  /**
   * Check if the instance is valid
   */
  isValid(): boolean {
    return this.instanceId >= 0;
  }

  /**
   * Get the instance ID
   */
  getId(): number {
    return this.instanceId;
  }
}
