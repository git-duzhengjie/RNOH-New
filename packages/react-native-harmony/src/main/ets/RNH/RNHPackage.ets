/**
 * RNOH-New: RNHPackage - Base class for native packages
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHPackage';

/**
 * Context provided to packages during registration
 */
export interface RNHPackageContext {
  // Instance ID
  instanceId: number;

  // App context (HarmonyOS)
  appContext: Object;

  // UI context
  uiContext: Object;
}

/**
 * Base class for RNOH-New packages.
 * Extend this to create packages that provide native modules
 * and/or custom components.
 */
export class RNHPackage {
  protected context: RNHPackageContext;

  constructor(context: RNHPackageContext) {
    this.context = context;
  }

  /**
   * Get the package name
   */
  getName(): string {
    return 'RNHPackage';
  }

  /**
   * Get the package version
   */
  getVersion(): string {
    return '0.1.0';
  }

  /**
   * Called when the package is registered
   */
  onRegister(): void {
    RNHLogger.info(TAG, `Package registered: ${this.getName()}`);
  }

  /**
   * Called when the package is unregistered
   */
  onUnregister(): void {
    RNHLogger.info(TAG, `Package unregistered: ${this.getName()}`);
  }

  /**
   * Get TurboModule factory map
   * Override to provide custom TurboModules
   */
  getTurboModuleFactoryMap(): Map<string, () => Object> {
    return new Map();
  }

  /**
   * Get component descriptor list
   * Override to provide custom components
   */
  getComponentDescriptors(): Array<Object> {
    return [];
  }
}
