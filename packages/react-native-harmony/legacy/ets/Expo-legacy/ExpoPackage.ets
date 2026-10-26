/**
 * RNOH-New: ExpoPackage - Expo core modules package
 *
 * Registers the Expo core TurboModules (Constants, Asset, FileSystem,
 * FontLoader, Linking) with the RN runtime.
 */

import { RNHPackage, RNHPackageContext } from '../RNH/RNHPackage';
import { RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';
import { ExpoConstantsModule } from './ExpoConstantsModule';
import { ExpoAssetModule } from './ExpoAssetModule';
import { ExpoFileSystemModule } from './ExpoFileSystemModule';
import { ExpoFontModule } from './ExpoFontModule';
import { ExpoLinkingModule } from './ExpoLinkingModule';

const TAG = 'ExpoPackage';

/**
 * Package that registers all Expo core TurboModules
 */
export class ExpoPackage extends RNHPackage {
  constructor(context: RNHPackageContext) {
    super(context);
  }

  override getName(): string {
    return 'ExpoPackage';
  }

  override getVersion(): string {
    return '0.1.0';
  }

  override onRegister(): void {
    super.onRegister();
    RNHLogger.info(TAG, 'Registering Expo core TurboModules...');
  }

  override getTurboModuleFactoryMap(): Map<string, () => Object> {
    const map = new Map<string, () => Object>();

    // Convert package context to TurboModule context
    const tmContext: RNHTurboModuleContext = {
      instanceId: this.context.instanceId,
      appContext: this.context.appContext,
      eventEmitter: (eventName: string, payload: string) => {
        // Forward to the package context's event emitter
        RNHLogger.debug(TAG, `TurboModule event: ${eventName}, payload: ${payload}`);
      },
    };

    // Register Expo core TurboModules
    map.set('ExpoConstants', () => new ExpoConstantsModule(tmContext));
    map.set('ExpoAsset', () => new ExpoAssetModule(tmContext));
    map.set('ExponentFileSystem', () => new ExpoFileSystemModule(tmContext));
    map.set('ExpoFontLoader', () => new ExpoFontModule(tmContext));
    map.set('ExpoLinking', () => new ExpoLinkingModule(tmContext));

    RNHLogger.info(TAG, `Registered ${map.size} Expo TurboModules`);
    return map;
  }
}
