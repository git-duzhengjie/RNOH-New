/**
 * RNOH-New: RNHSurface - Surface component for rendering RN content
 *
 * Each RNHSurface represents one root React Native view.
 * It manages the Fabric surface lifecycle and layout.
 */

import { RNHInstance } from './RNHInstance';
import { RNHLogger } from './RNHLogger';
import { RNHMemoryGuard } from './RNHMemoryGuard';
import display from '@ohos.display';

const TAG = 'RNHSurface';

/**
 * Surface configuration
 */
export interface RNHSurfaceConfig {
  // Surface ID (unique identifier)
  surfaceId: number;

  // RN module name
  moduleName: string;

  // Initial props
  initialProps?: Record<string, Object>;

  // RNH instance
  instance: RNHInstance;
}

/**
 * Surface component that hosts a React Native root view
 */
@Component
export struct RNHSurface {
  @State private isStarted: boolean = false;
  @State private surfaceWidth: number = 0;
  @State private surfaceHeight: number = 0;
  private memoryGuard: RNHMemoryGuard | null = null;

  // Surface configuration is set by the parent before build()
  private config: RNHSurfaceConfig | null = null;

  /**
   * Set surface configuration
   */
  setConfig(config: RNHSurfaceConfig): void {
    this.config = config;
  }

  /**
   * Get the device pixel ratio via @ohos.display
   */
  private getPixelRatio(): number {
    try {
      const disp = display.getDefaultDisplaySync();
      return disp.densityPixels;
    } catch (e) {
      RNHLogger.warn(TAG, `Failed to get pixel ratio, fallback to 1.0: ${e}`);
      return 1.0;
    }
  }

  aboutToAppear(): void {
    if (this.config === null) {
      RNHLogger.warn(TAG, 'Surface config is not set');
      return;
    }
    RNHLogger.info(TAG, `Surface ${this.config.surfaceId} aboutToAppear`);
    // Register with memory guard for cleanup tracking
    this.memoryGuard = this.config.instance.getMemoryGuard();
    this.memoryGuard.registerSurface(this.config.surfaceId);
    // If the component already has a size (e.g. re-appearing), start immediately
    if (this.surfaceWidth > 0 && this.surfaceHeight > 0 && !this.isStarted) {
      this.startSurface(this.surfaceWidth, this.surfaceHeight);
    }
  }

  private handleAreaChange(oldArea: Area, newArea: Area): void {
    const width = newArea.width as number;
    const height = newArea.height as number;

    if (width !== this.surfaceWidth || height !== this.surfaceHeight) {
      this.surfaceWidth = width;
      this.surfaceHeight = height;

      if (!this.isStarted) {
        this.startSurface(width, height);
      } else {
        this.updateSurfaceSize(width, height);
      }
    }
  }

  private startSurface(width: number, height: number): void {
    if (this.config === null) {
      RNHLogger.warn(TAG, 'Surface config is not set, cannot start');
      return;
    }
    RNHLogger.info(TAG, `Starting surface ${this.config.surfaceId}: ${width}x${height}`);
    const pixelRatio = this.getPixelRatio();
    this.config.instance.startSurface(
      this.config.surfaceId,
      this.config.moduleName,
      width,
      height,
      pixelRatio
    );
    this.isStarted = true;
  }

  private updateSurfaceSize(width: number, height: number): void {
    if (this.config === null) {
      RNHLogger.warn(TAG, 'Surface config is not set, cannot update size');
      return;
    }
    RNHLogger.debug(TAG, `Updating surface size: ${width}x${height}`);
    this.config.instance.updateSurfaceSize(
      this.config.surfaceId,
      width,
      height
    );
  }

  aboutToDisappear(): void {
    if (this.config === null) {
      RNHLogger.warn(TAG, 'Surface config is not set during disappear');
      return;
    }
    RNHLogger.info(TAG, `Surface ${this.config.surfaceId} aboutToDisappear`);
    if (this.isStarted) {
      this.config.instance.stopSurface(this.config.surfaceId);
      this.isStarted = false;
    }
    // Clean up tracked resources via memory guard
    if (this.memoryGuard) {
      this.memoryGuard.cleanupSurface(this.config.surfaceId);
      this.memoryGuard = null;
    }
  }

  build() {
    // Surface content is rendered by the native side
    // This component just provides the container
    Stack() {
      // Native content will be attached here
    }
    .width('100%')
    .height('100%')
    .onAreaChange((oldArea: Area, newArea: Area) => {
      this.handleAreaChange(oldArea, newArea);
    })
  }
}
