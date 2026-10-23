/**
 * RNOH-New: RNHSurface - Surface component for rendering RN content
 *
 * Each RNHSurface represents one root React Native view.
 * It manages the Fabric surface lifecycle and layout.
 */

import { RNHInstance } from './RNHInstance';
import { RNHLogger } from './RNHLogger';
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

  private config: RNHSurfaceConfig = {
    surfaceId: 0,
    moduleName: 'App',
    instance: null as unknown as RNHInstance,
  };

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
    RNHLogger.info(TAG, `Surface ${this.config.surfaceId} aboutToAppear`);
    // If the component already has a size (e.g. re-appearing), start immediately
    if (this.surfaceWidth > 0 && this.surfaceHeight > 0 && !this.isStarted) {
      this.startSurface(this.surfaceWidth, this.surfaceHeight);
    }
  }

  onAreaChange(oldArea: Area, newArea: Area): void {
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
    RNHLogger.debug(TAG, `Updating surface size: ${width}x${height}`);
    this.config.instance.updateSurfaceSize(
      this.config.surfaceId,
      width,
      height
    );
  }

  aboutToDisappear(): void {
    RNHLogger.info(TAG, `Surface ${this.config.surfaceId} aboutToDisappear`);
    if (this.isStarted) {
      this.config.instance.stopSurface(this.config.surfaceId);
      this.isStarted = false;
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
      this.onAreaChange(oldArea, newArea);
    })
  }
}
