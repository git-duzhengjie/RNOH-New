/**
 * RNOH-New: RNHCorePackage - Core package with built-in components and modules
 *
 * This package provides the standard React Native components
 * (View, Text, Image, etc.) and core TurboModules.
 */

import { RNHPackage, RNHPackageContext } from '../RNH/RNHPackage';
import { RNHLogger } from '../RNH/RNHLogger';

const TAG = 'RNHCorePackage';

/**
 * Core package that registers all built-in RN components and modules
 */
export class RNHCorePackage extends RNHPackage {
  constructor(context: RNHPackageContext) {
    super(context);
  }

  override getName(): string {
    return 'RNHCorePackage';
  }

  override getVersion(): string {
    return '0.1.0';
  }

  override onRegister(): void {
    super.onRegister();
    RNHLogger.info(TAG, 'Registering core components and TurboModules...');
  }

  override getTurboModuleFactoryMap(): Map<string, () => Object> {
    const map = new Map<string, () => Object>();
    // TODO: Register core TurboModules
    // map.set('DeviceInfo', () => new DeviceInfoTurboModule(this.context));
    // map.set('AppState', () => new AppStateTurboModule(this.context));
    // map.set('Networking', () => new NetworkingTurboModule(this.context));
    return map;
  }
}
