/**
 * RNOH-New: RNHInstance - TypeScript-side instance manager
 *
 * Manages the RNH native instance from the ArkTS/JS side.
 * Communicates with the C++ side via NAPI.
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHInstance';

// NAPI bridge types
declare const rnohNew: {
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
};

/**
 * Native-side instance configuration
 */
interface RNHInstanceNativeConfig {
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
      // TODO: Load NAPI module and call createInstance
      // this.instanceId = rnohNew.createInstance(this.config);
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
    // TODO: Call native loadBundle
    return true;
  }

  /**
   * Load a JS bundle from a Metro server URL
   */
  loadBundleFromUrl(url: string): boolean {
    RNHLogger.info(TAG, `Loading bundle from URL: ${url}`);
    // TODO: Call native loadBundleFromUrl
    return true;
  }

  /**
   * Start a new surface (root view)
   */
  startSurface(surfaceId: number, moduleName: string,
               width: number, height: number, pixelRatio: number): void {
    RNHLogger.info(TAG, `Starting surface ${surfaceId}, module: ${moduleName}`);
    // TODO: Call native startSurface
    this.surfaces.add(surfaceId);
  }

  /**
   * Stop a surface
   */
  stopSurface(surfaceId: number): void {
    RNHLogger.info(TAG, `Stopping surface ${surfaceId}`);
    // TODO: Call native stopSurface
    this.surfaces.delete(surfaceId);
  }

  /**
   * Update surface dimensions
   */
  updateSurfaceSize(surfaceId: number, width: number, height: number): void {
    // TODO: Call native updateSurfaceSize
  }

  /**
   * Emit an event to JS
   */
  emitEvent(surfaceId: number, eventName: string, payload: string): void {
    // TODO: Call native emitEvent
  }

  /**
   * Send a touch event to JS
   */
  sendTouchEvent(surfaceId: number, action: number,
                 x: number, y: number, timestamp: number): void {
    // TODO: Call native sendTouchEvent
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
      // TODO: Call native destroyInstance
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
