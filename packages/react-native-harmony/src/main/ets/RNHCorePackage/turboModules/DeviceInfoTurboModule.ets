/**
 * RNOH-New: DeviceInfoTurboModule - Provides device information to JS
 *
 * Implements the RN DeviceInfo native module interface.
 * Uses HarmonyOS @ohos.deviceInfo and @ohos.display APIs.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import deviceInfo from '@ohos.deviceInfo';
import display from '@ohos.display';

const TAG = 'DeviceInfoTurboModule';

/**
 * Screen dimensions structure
 */
interface ScreenDimensions {
  width: number;
  height: number;
  scale: number;
  fontScale: number;
}

/**
 * Device constants returned to JS
 */
export interface DeviceInfoConstants {
  // Device info
  model: string;
  brand: string;
  deviceId: string;
  systemName: string;
  systemVersion: string;

  // Screen
  dimensions: ScreenDimensions;

  // Other
  isTablet: boolean;
  isEmulator: boolean;
}

/**
 * DeviceInfo TurboModule
 */
export class DeviceInfoTurboModule extends RNHTurboModule {
  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'DeviceInfo';
  }

  /**
   * Get device constants (sync method for RN DeviceInfo)
   */
  getConstants(): DeviceInfoConstants {
    const screenDims = this.getScreenDimensions();

    return {
      model: deviceInfo.productModel,
      brand: deviceInfo.brand,
      deviceId: deviceInfo.udid,
      systemName: 'HarmonyOS',
      systemVersion: deviceInfo.osFullName,
      dimensions: screenDims,
      isTablet: deviceInfo.deviceType === 'tablet',
      isEmulator: false,
    };
  }

  /**
   * Get current screen dimensions
   */
  private getScreenDimensions(): ScreenDimensions {
    try {
      const disp = display.getDefaultDisplaySync();
      return {
        width: disp.width,
        height: disp.height,
        scale: disp.densityPixels,
        fontScale: 1.0,
      };
    } catch (e) {
      RNHLogger.warn(TAG, `Failed to get screen dimensions: ${e}`);
      return {
        width: 0,
        height: 0,
        scale: 1.0,
        fontScale: 1.0,
      };
    }
  }

  /**
   * Get device model
   */
  getModel(): string {
    return deviceInfo.productModel;
  }

  /**
   * Get device brand
   */
  getBrand(): string {
    return deviceInfo.brand;
  }

  /**
   * Get system version
   */
  getSystemVersion(): string {
    return deviceInfo.osFullName;
  }

  /**
   * Get unique device ID
   */
  getUniqueId(): string {
    return deviceInfo.udid;
  }

  /**
   * Check if device is a tablet
   */
  isTablet(): boolean {
    return deviceInfo.deviceType === 'tablet';
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'DeviceInfoTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'DeviceInfoTurboModule invalidated');
    super.invalidate();
  }
}
