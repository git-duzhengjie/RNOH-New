/**
 * RNOH-New: ExpoAssetModule - Expo Asset native module
 *
 * Implements the Expo 'ExpoAsset' interface. Locates bundled assets
 * in the HarmonyOS rawfile 'assets' directory (produced by `rnh-new bundle`).
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';

const TAG = 'ExpoAssetModule';

/**
 * Asset metadata resolved from a moduleId
 */
export interface AssetDescriptor {
  id: number;
  name: string;
  type: string;
  uri: string;
  localUri: string;
  hash: string;
  width: number;
  height: number;
}

/**
 * Source info for Image.resolveAssetSource
 */
export interface AssetSource {
  uri: string;
  width: number;
  height: number;
  scale: number;
}

/**
 * ExpoAsset TurboModule
 */
export class ExpoAssetModule extends RNHTurboModule {
  private assetIdCounter: number = 1;
  private assetCache: Map<string, AssetDescriptor> = new Map();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'ExpoAsset';
  }

  /**
   * Resolve an asset descriptor from a moduleId
   *
   * moduleId is typically the numeric hash of the module (from metro).
   * We resolve it relative to the rawfile 'assets' directory.
   */
  fromModule(moduleId: number): AssetDescriptor {
    RNHLogger.debug(TAG, `fromModule: ${moduleId}`);
    try {
      const cacheKey = `module_${moduleId}`;
      const cached = this.assetCache.get(cacheKey);
      if (cached) {
        return cached;
      }

      // rawfile base path for bundled assets
      const rawfileBase = 'rawfile:///assets/';
      // In Expo, moduleId usually maps to an asset like `assets/assets/<hash>.png`.
      // We build the URI from the module id hash.
      const name = `asset_${moduleId}`;
      const type = this.guessAssetType(moduleId);
      const uri = `${rawfileBase}${name}.${type}`;

      const descriptor: AssetDescriptor = {
        id: this.assetIdCounter++,
        name,
        type,
        uri,
        localUri: uri,
        hash: `${moduleId}`,
        width: 0,
        height: 0,
      };

      this.assetCache.set(cacheKey, descriptor);
      return descriptor;
    } catch (e) {
      RNHLogger.error(TAG, `fromModule failed: ${e}`);
      throw e;
    }
  }

  /**
   * Get an asset descriptor by its id
   */
  getAssetByID(assetId: number): AssetDescriptor | null {
    RNHLogger.debug(TAG, `getAssetByID: ${assetId}`);
    for (const descriptor of this.assetCache.values()) {
      if (descriptor.id === assetId) {
        return descriptor;
      }
    }
    RNHLogger.warn(TAG, `Asset with id ${assetId} not found`);
    return null;
  }

  /**
   * Resolve the asset source for Image component
   */
  resolveAssetSource(moduleId: number): AssetSource {
    RNHLogger.debug(TAG, `resolveAssetSource: ${moduleId}`);
    const descriptor = this.fromModule(moduleId);
    return {
      uri: descriptor.uri,
      width: descriptor.width,
      height: descriptor.height,
      scale: 1.0,
    };
  }

  /**
   * Register an asset with a known local path
   */
  registerAsset(localUri: string): AssetDescriptor {
    RNHLogger.debug(TAG, `registerAsset: ${localUri}`);
    const cacheKey = `uri_${localUri}`;
    const cached = this.assetCache.get(cacheKey);
    if (cached) {
      return cached;
    }

    const descriptor: AssetDescriptor = {
      id: this.assetIdCounter++,
      name: this.guessFileName(localUri),
      type: this.guessExtension(localUri),
      uri: localUri,
      localUri,
      hash: localUri,
      width: 0,
      height: 0,
    };

    this.assetCache.set(cacheKey, descriptor);
    return descriptor;
  }

  /**
   * Guess asset type from module id (best-effort)
   */
  private guessAssetType(moduleId: number): string {
    // Metro assigns numeric module ids; type cannot be inferred reliably.
    // Default to png, which is the most common static asset type.
    return 'png';
  }

  /**
   * Extract file name from a URI
   */
  private guessFileName(uri: string): string {
    const segments = uri.split('/');
    const last = segments[segments.length - 1];
    const dotIndex = last.lastIndexOf('.');
    return dotIndex > 0 ? last.substring(0, dotIndex) : last;
  }

  /**
   * Extract extension from a URI
   */
  private guessExtension(uri: string): string {
    const dotIndex = uri.lastIndexOf('.');
    if (dotIndex > 0 && dotIndex < uri.length - 1) {
      return uri.substring(dotIndex + 1);
    }
    return '';
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'ExpoAssetModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'ExpoAssetModule invalidated');
    this.assetCache.clear();
    super.invalidate();
  }
}
