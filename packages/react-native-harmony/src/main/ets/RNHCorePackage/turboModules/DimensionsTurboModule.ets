/**
 * RNOH-New: DimensionsTurboModule - Provides Dimensions native module
 *
 * Implements React Native's Dimensions interface using @ohos.display.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import display from '@ohos.display';

const TAG = 'DimensionsTurboModule';

/**
 * Dimension type
 */
export type DimensionName = 'screen' | 'window';

/**
 * Dimension values
 */
export interface DimensionValues {
  width: number;
  height: number;
  scale: number;
  fontScale: number;
}

/**
 * Dimensions change event payload
 */
interface DimensionsChangePayload {
  screen: DimensionValues;
  window: DimensionValues;
}

/**
 * Change event listener
 */
type DimensionsChangeListener = (payload: DimensionsChangePayload) => void;

/**
 * Dimensions TurboModule
 */
export class DimensionsTurboModule extends RNHTurboModule {
  private listeners: Set<DimensionsChangeListener> = new Set();
  private displayChangeHandler?: (data: display.Display) => void;

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'Dimensions';
  }

  /**
   * Get screen dimensions
   */
  get(dimension: DimensionName): DimensionValues {
    RNHLogger.debug(TAG, `Getting dimensions for: ${dimension}`);
    try {
      const disp = display.getDefaultDisplaySync();
      const values: DimensionValues = {
        width: disp.width,
        height: disp.height,
        scale: disp.densityPixels,
        fontScale: disp.fontScale ?? 1.0,
      };
      return values;
    } catch (e) {
      RNHLogger.error(TAG, `Failed to get dimensions: ${e}`);
      return {
        width: 0,
        height: 0,
        scale: 1.0,
        fontScale: 1.0,
      };
    }
  }

  /**
   * Set dimensions (no-op on HarmonyOS, kept for API compatibility)
   */
  set(dimension: DimensionName, dims: DimensionValues): void {
    RNHLogger.warn(TAG, `set(${dimension}, ${JSON.stringify(dims)}) not supported on HarmonyOS`);
    // TODO: Implement if HarmonyOS provides a way to override window dimensions
  }

  /**
   * Add event listener for display changes
   */
  addEventListener(eventName: 'change', listener: DimensionsChangeListener): void {
    if (eventName !== 'change') {
      RNHLogger.warn(TAG, `Unsupported event: ${eventName}`);
      return;
    }

    const isFirstListener = this.listeners.size === 0;
    this.listeners.add(listener);

    if (isFirstListener) {
      this.startListening();
    }

    RNHLogger.debug(TAG, `Added Dimensions change listener, total: ${this.listeners.size}`);
  }

  /**
   * Remove event listener
   */
  removeEventListener(eventName: 'change', listener: DimensionsChangeListener): void {
    if (eventName !== 'change') {
      return;
    }
    this.listeners.delete(listener);
    RNHLogger.debug(TAG, `Removed Dimensions change listener, total: ${this.listeners.size}`);

    if (this.listeners.size === 0) {
      this.stopListening();
    }
  }

  /**
   * Start listening to display changes
   */
  private startListening(): void {
    try {
      this.displayChangeHandler = (data: display.Display) => {
        this.handleDisplayChange(data);
      };
      display.on('change', this.displayChangeHandler);
      RNHLogger.info(TAG, 'Started listening for display changes');
    } catch (e) {
      RNHLogger.error(TAG, `Failed to listen for display changes: ${e}`);
    }
  }

  /**
   * Stop listening to display changes
   */
  private stopListening(): void {
    if (this.displayChangeHandler) {
      try {
        display.off('change', this.displayChangeHandler);
        RNHLogger.info(TAG, 'Stopped listening for display changes');
      } catch (e) {
        RNHLogger.error(TAG, `Failed to stop listening: ${e}`);
      }
      this.displayChangeHandler = undefined;
    }
  }

  /**
   * Handle display change event
   */
  private handleDisplayChange(data: display.Display): void {
    const values: DimensionValues = {
      width: data.width,
      height: data.height,
      scale: data.densityPixels,
      fontScale: data.fontScale ?? 1.0,
    };

    const payload: DimensionsChangePayload = {
      screen: values,
      window: values,
    };

    // Notify ArkTS listeners
    for (const listener of this.listeners) {
      try {
        listener(payload);
      } catch (e) {
        RNHLogger.error(TAG, `Error in Dimensions listener: ${e}`);
      }
    }

    // Emit to JS
    this.emitEvent('didUpdateDimensions', JSON.stringify(payload));
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'DimensionsTurboModule initialized');
  }

  override invalidate(): void {
    this.stopListening();
    this.listeners.clear();
    RNHLogger.info(TAG, 'DimensionsTurboModule invalidated');
    super.invalidate();
  }
}
