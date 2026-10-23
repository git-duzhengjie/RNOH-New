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
import { AlertTurboModule } from './turboModules/AlertTurboModule';
import { ClipboardTurboModule } from './turboModules/ClipboardTurboModule';
import { DimensionsTurboModule } from './turboModules/DimensionsTurboModule';
import { PixelRatioTurboModule } from './turboModules/PixelRatioTurboModule';
import { PlatformTurboModule } from './turboModules/PlatformTurboModule';
import { NetworkingTurboModule } from './turboModules/NetworkingTurboModule';
import { AsyncStorageTurboModule } from './turboModules/AsyncStorageTurboModule';
import { FileSystemTurboModule } from './turboModules/FileSystemTurboModule';
import { AnimatedTurboModule } from './turboModules/AnimatedTurboModule';
import { LinkingTurboModule } from './turboModules/LinkingTurboModule';
import { KeyboardTurboModule } from './turboModules/KeyboardTurboModule';
import { TimingTurboModule } from './turboModules/TimingTurboModule';
import { ExpoPackage } from '../Expo/ExpoPackage';

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
    map.set('AlertManager', () => new AlertTurboModule(tmContext));
    map.set('Clipboard', () => new ClipboardTurboModule(tmContext));
    map.set('Dimensions', () => new DimensionsTurboModule(tmContext));
    map.set('PixelRatio', () => new PixelRatioTurboModule(tmContext));
    map.set('PlatformConstants', () => new PlatformTurboModule(tmContext));
    map.set('Networking', () => new NetworkingTurboModule(tmContext));
    map.set('RNCAsyncStorage', () => new AsyncStorageTurboModule(tmContext));
    map.set('RNFSManager', () => new FileSystemTurboModule(tmContext));
    map.set('NativeAnimatedModule', () => new AnimatedTurboModule(tmContext));
    map.set('LinkingManager', () => new LinkingTurboModule(tmContext));
    map.set('KeyboardObserver', () => new KeyboardTurboModule(tmContext));
    map.set('Timing', () => new TimingTurboModule(tmContext));

    // Merge Expo core TurboModules
    const expoPackage = new ExpoPackage(this.context);
    const expoMap = expoPackage.getTurboModuleFactoryMap();
    for (const [name, factory] of expoMap) {
      map.set(name, factory);
    }

    RNHLogger.info(TAG, `Registered ${map.size} TurboModules`);
    return map;
  }
}
