/**
 * RNOH-New: PixelRatioTurboModule - Provides PixelRatio native module
 *
 * Implements React Native's PixelRatio interface using @ohos.display.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import display from '@ohos.display';

const TAG = 'PixelRatioTurboModule';

/**
 * Font scale change listener
 */
type FontScaleChangeListener = (fontScale: number) => void;

/**
 * PixelRatio TurboModule
 */
export class PixelRatioTurboModule extends RNHTurboModule {
  private fontScaleListeners: Set<FontScaleChangeListener> = new Set();
  private displayChangeHandler?: (displayId: number) => void;
  private previousFontScale: number = 1.0;

  constructor(context: RNHTurboModuleContext) {
    super(context);
    this.previousFontScale = this.getFontScale();
  }

  override getName(): string {
    return 'PixelRatio';
  }

  /**
   * Get the device pixel ratio
   */
  get(): number {
    try {
      const disp = display.getDefaultDisplaySync();
      return disp.densityPixels;
    } catch (e) {
      RNHLogger.warn(TAG, `Failed to get pixel ratio: ${e}`);
      return 1.0;
    }
  }

  /**
   * Get the font scale.
   * HarmonyOS does not expose fontScale via @ohos.display, so we return 1.0.
   */
  getFontScale(): number {
    return 1.0;
  }

  /**
   * Convert DIP to pixels
   */
  getPixelSizeForLayoutSize(layoutSize: number): number {
    return Math.round(layoutSize * this.get());
  }

  /**
   * Round to nearest pixel
   */
  roundToNearestPixel(layoutSize: number): number {
    const ratio = this.get();
    return Math.round(layoutSize * ratio) / ratio;
  }

  /**
   * Start listening for display changes that may affect font scale
   */
  startListeningForMinimumFontScaleChanged(): void {
    if (this.displayChangeHandler) {
      return;
    }

    // HarmonyOS display.on('change') callback receives a display ID number,
    // not a Display object. Re-fetch the display inside the handler.
    this.displayChangeHandler = (displayId: number) => {
      RNHLogger.debug(TAG, `Display changed: ${displayId}`);
      const currentFontScale = this.getFontScale();
      if (currentFontScale !== this.previousFontScale) {
        this.previousFontScale = currentFontScale;
        this.notifyFontScaleChanged(currentFontScale);
      }
    };

    try {
      display.on('change', this.displayChangeHandler);
      RNHLogger.info(TAG, 'Started listening for font scale changes');
    } catch (e) {
      RNHLogger.error(TAG, `Failed to listen for font scale changes: ${e}`);
      this.displayChangeHandler = undefined;
    }
  }

  /**
   * Stop listening for font scale changes
   */
  stopListeningForMinimumFontScaleChanged(): void {
    if (this.displayChangeHandler) {
      try {
        display.off('change', this.displayChangeHandler);
        RNHLogger.info(TAG, 'Stopped listening for font scale changes');
      } catch (e) {
        RNHLogger.error(TAG, `Failed to stop listening: ${e}`);
      }
      this.displayChangeHandler = undefined;
    }
  }

  /**
   * Add event listener for font scale changes
   */
  addEventListener(eventName: string, listener: FontScaleChangeListener): void {
    if (eventName !== 'change') {
      RNHLogger.warn(TAG, `Unsupported event: ${eventName}`);
      return;
    }

    const isFirstListener = this.fontScaleListeners.size === 0;
    this.fontScaleListeners.add(listener);

    if (isFirstListener) {
      this.startListeningForMinimumFontScaleChanged();
    }

    RNHLogger.debug(TAG, `Added font scale listener, total: ${this.fontScaleListeners.size}`);
  }

  /**
   * Remove event listener for font scale changes
   */
  removeEventListener(eventName: string, listener: FontScaleChangeListener): void {
    if (eventName !== 'change') {
      return;
    }
    this.fontScaleListeners.delete(listener);
    RNHLogger.debug(TAG, `Removed font scale listener, total: ${this.fontScaleListeners.size}`);

    if (this.fontScaleListeners.size === 0) {
      this.stopListeningForMinimumFontScaleChanged();
    }
  }

  /**
   * Notify listeners of font scale change
   */
  private notifyFontScaleChanged(fontScale: number): void {
    for (const listener of this.fontScaleListeners) {
      try {
        listener(fontScale);
      } catch (e) {
        RNHLogger.error(TAG, `Error in font scale listener: ${e}`);
      }
    }

    this.emitEvent('didUpdateFontScale', JSON.stringify({ fontScale }));
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'PixelRatioTurboModule initialized');
  }

  override invalidate(): void {
    this.stopListeningForMinimumFontScaleChanged();
    this.fontScaleListeners.clear();
    RNHLogger.info(TAG, 'PixelRatioTurboModule invalidated');
    super.invalidate();
  }
}
