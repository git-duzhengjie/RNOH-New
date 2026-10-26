/**
 * RNOH-New: ExpoFontModule - ExpoFontLoader native module
 *
 * Implements the Expo 'ExpoFontLoader' interface. Copies font files
 * from rawfile to the cache directory and returns a usable fontFamily.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';
import { toError } from '../RNH/RNHErrorUtils';
import fs from '@ohos.file.fs';

const TAG = 'ExpoFontModule';

/**
 * Load font options
 */
interface LoadFontOptions {
  fontFamilyName: string;
  localUri: string;
}

/**
 * Loaded font result
 */
interface LoadFontResult {
  fontFamilyName: string;
  localUri: string;
  fontName: string;
}

/**
 * ExpoFontLoader TurboModule
 */
export class ExpoFontModule extends RNHTurboModule {
  private loadedFonts: Map<string, LoadFontResult> = new Map();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'ExpoFontLoader';
  }

  /**
   * Load a font into cache and make it usable
   */
  async loadAsync(options: LoadFontOptions): Promise<LoadFontResult> {
    RNHLogger.info(TAG, `loadAsync: ${options.fontFamilyName}, localUri: ${options.localUri}`);
    try {
      const cached = this.loadedFonts.get(options.fontFamilyName);
      if (cached) {
        return cached;
      }

      const ctx = this.context.appContext as Context;
      const cacheDir = ctx.cacheDir;
      const fontFileName = this.extractFileName(options.localUri);
      const targetPath = `${cacheDir}/${fontFileName}`;

      // localUri may point to rawfile (rawfile:///) or a sandbox path.
      if (options.localUri.startsWith('rawfile:///')) {
        // Copy from rawfile to cache via resource manager
        const rawPath = options.localUri.replace('rawfile:///', '');
        const resourceMgr = ctx.resourceManager;
        const rawFile = await resourceMgr.getRawFileContent(rawPath);
        const buffer: ArrayBuffer = rawFile.buffer as ArrayBuffer;
        this.writeBufferToFile(targetPath, buffer);
      } else {
        // Direct copy from sandbox path
        this.copyFile(options.localUri, targetPath);
      }

      const result: LoadFontResult = {
        fontFamilyName: options.fontFamilyName,
        localUri: targetPath,
        fontName: options.fontFamilyName,
      };

      this.loadedFonts.set(options.fontFamilyName, result);
      RNHLogger.info(TAG, `Font loaded: ${options.fontFamilyName} -> ${targetPath}`);
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `loadAsync failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Write a buffer to a file
   */
  private writeBufferToFile(path: string, buffer: ArrayBuffer): void {
    const file = fs.openSync(path, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
    try {
      fs.writeSync(file.fd, buffer);
      fs.fsyncSync(file.fd);
    } finally {
      fs.closeSync(file);
    }
  }

  /**
   * Copy a file from source to target
   */
  private copyFile(from: string, to: string): void {
    const source = fs.openSync(from, fs.OpenMode.READ_ONLY);
    try {
      const dest = fs.openSync(to, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
      try {
        fs.copyFileSync(source.fd, dest.fd);
      } finally {
        fs.closeSync(dest);
      }
    } finally {
      fs.closeSync(source);
    }
  }

  /**
   * Extract file name from a URI
   */
  private extractFileName(uri: string): string {
    const segments = uri.split('/');
    return segments[segments.length - 1];
  }

  /**
   * Check if a font family is loaded
   */
  hasFont(fontFamilyName: string): boolean {
    return this.loadedFonts.has(fontFamilyName);
  }

  /**
   * Unload all fonts
   */
  unloadAll(): void {
    this.loadedFonts.clear();
    RNHLogger.info(TAG, 'Unloaded all fonts');
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'ExpoFontModule initialized');
  }

  override invalidate(): void {
    this.loadedFonts.clear();
    RNHLogger.info(TAG, 'ExpoFontModule invalidated');
    super.invalidate();
  }
}
