/**
 * RNOH-New: ExpoConstantsModule - Expo Constants native module
 *
 * Implements the Expo 'ExpoConstants' interface using HarmonyOS system APIs.
 * Reuses data sources from DeviceInfoTurboModule (@ohos.deviceInfo / @ohos.display).
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';
import deviceInfo from '@ohos.deviceInfo';
import display from '@ohos.display';
import bundleManager from '@ohos.bundle.bundleManager';

const TAG = 'ExpoConstantsModule';

/**
 * Platform info returned by ExpoConstants
 */
export interface ExpoPlatform {
  // Always 'harmony' on HarmonyOS
  platform: string;
  model: string;
  deviceName: string;
  deviceYearClass: number;
  osVersion: string;
  systemVersion: string;
  systemName: string;
}

/**
 * Manifest / app config info
 */
export interface ExpoAppConfig {
  name: string;
  slug: string;
  version: string;
  sdkVersion: string;
}

/**
 * Native constants returned by getConstants()
 */
export interface ExpoConstants {
  platform: ExpoPlatform;
  deviceName: string;
  deviceYearClass: number;
  systemVersion: string;
  statusBarHeight: number;
  screenHeight: number;
  screenWidth: number;
  isDevice: boolean;
  isHeadless: boolean;
  isNewArchEnabled: boolean;
  expoConfig: ExpoAppConfig | null;
}

/**
 * ExpoConstants TurboModule
 */
export class ExpoConstantsModule extends RNHTurboModule {
  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'ExpoConstants';
  }

  /**
   * Get the native constants exposed to JS
   */
  getConstants(): ExpoConstants {
    const dims = this.getScreenDimensions();
    const appConfig = this.getAppConfig();

    return {
      platform: {
        platform: 'harmony',
        model: deviceInfo.productModel,
        deviceName: deviceInfo.marketName.length > 0 ? deviceInfo.marketName : deviceInfo.productModel,
        deviceYearClass: this.getDeviceYearClass(),
        osVersion: deviceInfo.osFullName,
        systemVersion: deviceInfo.osFullName,
        systemName: 'HarmonyOS',
      },
      deviceName: deviceInfo.marketName.length > 0 ? deviceInfo.marketName : deviceInfo.productModel,
      deviceYearClass: this.getDeviceYearClass(),
      systemVersion: deviceInfo.osFullName,
      statusBarHeight: this.getStatusBarHeight(),
      screenHeight: dims.height,
      screenWidth: dims.width,
      isDevice: true,
      isHeadless: false,
      isNewArchEnabled: true,
      expoConfig: appConfig,
    };
  }

  /**
   * Get screen dimensions from @ohos.display
   */
  private getScreenDimensions(): { width: number; height: number } {
    try {
      const disp = display.getDefaultDisplaySync();
      return {
        width: disp.width,
        height: disp.height,
      };
    } catch (e) {
      RNHLogger.warn(TAG, `Failed to get screen dimensions: ${e}`);
      return {
        width: 0,
        height: 0,
      };
    }
  }

  /**
   * Get the status bar height (vp-based approximation)
   */
  private getStatusBarHeight(): number {
    try {
      // HarmonyOS does not expose status bar height via display directly.
      // Use a common default based on screen density for now.
      const disp = display.getDefaultDisplaySync();
      return Math.round(24 * disp.densityPixels);
    } catch (e) {
      RNHLogger.warn(TAG, `Failed to get status bar height: ${e}`);
      return 24;
    }
  }

  /**
   * Approximate device year class based on model / hardware
   */
  private getDeviceYearClass(): number {
    // Best-effort estimate; return a conservative value (2023+ devices).
    return 2023;
  }

  /**
   * Read the app config from bundleManager (app.json5)
   */
  private getAppConfig(): ExpoAppConfig | null {
    try {
      const ctx = this.context.appContext as Context;
      const bundleInfo = bundleManager.getBundleInfoForSelfSync(
        bundleManager.BundleFlag.GET_BUNDLE_INFO_DEFAULT
      );
      return {
        name: bundleInfo.name,
        slug: bundleInfo.name,
        version: bundleInfo.versionName,
        sdkVersion: '52.0.0',
      };
    } catch (e) {
      RNHLogger.warn(TAG, `Failed to read app config: ${e}`);
      return null;
    }
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'ExpoConstantsModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'ExpoConstantsModule invalidated');
    super.invalidate();
  }
}
