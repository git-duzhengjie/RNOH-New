/**
 * RNOH-New: PlatformTurboModule - Provides PlatformConstants native module
 *
 * Implements React Native's PlatformConstants interface using @ohos.deviceInfo.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import deviceInfo from '@ohos.deviceInfo';

const TAG = 'PlatformTurboModule';

/**
 * Platform constants returned to JS
 */
export interface PlatformConstants {
  osVersion: string;
  osVersionCode: number;
  systemName: string;
  Model: string;
  brand: string;
}

/**
 * PlatformConstants TurboModule
 */
export class PlatformTurboModule extends RNHTurboModule {
  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'PlatformConstants';
  }

  /**
   * Get platform constants
   */
  getConstants(): PlatformConstants {
    const sdkApiVersion = Number(deviceInfo.sdkApiVersion);
    const osVersionCode = Number.isNaN(sdkApiVersion) ? 0 : sdkApiVersion;
    const constants: PlatformConstants = {
      osVersion: deviceInfo.osFullName,
      osVersionCode,
      systemName: 'HarmonyOS',
      Model: deviceInfo.marketName,
      brand: deviceInfo.brand,
    };
    return constants;
  }

  /**
   * Get OS version string
   */
  getOSVersion(): string {
    return deviceInfo.osFullName;
  }

  /**
   * Get OS SDK API version code
   */
  getAPILevel(): number {
    const version = Number(deviceInfo.sdkApiVersion);
    return Number.isNaN(version) ? 0 : version;
  }

  /**
   * Get system name
   */
  getSystemName(): string {
    return 'HarmonyOS';
  }

  /**
   * Get device model
   */
  getModel(): string {
    return deviceInfo.marketName;
  }

  /**
   * Get device brand
   */
  getBrand(): string {
    return deviceInfo.brand;
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'PlatformTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'PlatformTurboModule invalidated');
    super.invalidate();
  }
}
