/**
 * RNOH-New: RNHCorePackage - Core package with built-in components and modules
 *
 * This package provides the standard React Native components
 * (View, Text, Image, etc.) and core TurboModules.
 */

import { RNHPackage, RNHPackageContext } from '../RNH/RNHPackage';
import { RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';
import { DeviceInfoTurboModule } from './turboModules/DeviceInfoTurboModule';
import { AppStateTurboModule } from './turboModules/AppStateTurboModule';
import { AccessibilityInfoTurboModule } from './turboModules/AccessibilityInfoTurboModule';

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

    // Convert package context to TurboModule context
    const tmContext: RNHTurboModuleContext = {
      instanceId: this.context.instanceId,
      appContext: this.context.appContext,
      eventEmitter: (eventName: string, payload: string) => {
        // Forward to the package context's event emitter
        // This will be connected to the JS bridge
        RNHLogger.debug(TAG, `TurboModule event: ${eventName}, payload: ${payload}`);
      },
    };

    // Register core TurboModules
    map.set('DeviceInfo', () => new DeviceInfoTurboModule(tmContext));
    map.set('AppState', () => new AppStateTurboModule(tmContext));
    map.set('AccessibilityInfo', () => new AccessibilityInfoTurboModule(tmContext));

    RNHLogger.info(TAG, `Registered ${map.size} TurboModules`);
    return map;
  }
}
