/**
 * RNOH-New: ExpoFileSystemModule - ExponentFileSystem native module
 *
 * Implements the Expo 'ExponentFileSystem' interface using @ohos.file.fs.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';
import { toError } from '../RNH/RNHErrorUtils';
import fs from '@ohos.file.fs';
import util from '@ohos.util';

const TAG = 'ExpoFileSystemModule';

/**
 * FileSystem directory info returned to JS
 */
export interface FileSystemDirectories {
  documentDirectory: string;
  cacheDirectory: string;
}

/**
 * Copy operation options
 */
interface CopyOptions {
  from: string;
  to: string;
}

/**
 * Delete operation options
 */
interface DeleteOptions {
  id?: string;
  uri: string;
}

/**
 * Make directory options
 */
interface MakeDirectoryOptions {
  id?: string;
  uri: string;
  intermediates: boolean;
}

/**
 * Read string options
 */
interface ReadAsStringOptions {
  uri: string;
  encoding?: string;
}

/**
 * Write string options
 */
interface WriteAsStringOptions {
  uri: string;
  encoding?: string;
  string: string;
}

/**
 * ExponentFileSystem TurboModule
 */
export class ExpoFileSystemModule extends RNHTurboModule {
  private textDecoder: util.TextDecoder = new util.TextDecoder('utf-8');
  private textEncoder: util.TextEncoder = new util.TextEncoder();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'ExponentFileSystem';
  }

  /**
   * Get document and cache directory paths
   */
  getDirectories(): FileSystemDirectories {
    try {
      const ctx = this.context.appContext as Context;
      const documentDir = ctx.filesDir;
      const cacheDir = ctx.cacheDir;
      const result: FileSystemDirectories = {
        documentDirectory: `${documentDir}/`,
        cacheDirectory: `${cacheDir}/`,
      };
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `getDirectories failed: ${e}`);
      return {
        documentDirectory: '',
        cacheDirectory: '',
      };
    }
  }

  /**
   * Copy a file from source to destination
   */
  async copyAsync(options: CopyOptions): Promise<void> {
    RNHLogger.info(TAG, `copyAsync: ${options.from} -> ${options.to}`);
    try {
      const source = fs.openSync(options.from, fs.OpenMode.READ_ONLY);
      try {
        const dest = fs.openSync(options.to, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
        try {
          fs.copyFileSync(source.fd, dest.fd);
        } finally {
          fs.closeSync(dest);
        }
      } finally {
        fs.closeSync(source);
      }
    } catch (e) {
      RNHLogger.error(TAG, `copyAsync failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Delete a file or directory
   */
  async deleteAsync(options: DeleteOptions): Promise<void> {
    RNHLogger.info(TAG, `deleteAsync: ${options.uri}`);
    try {
      const uri = options.uri;
      if (fs.accessSync(uri)) {
        const stat = fs.statSync(uri);
        if (stat.isDirectory()) {
          fs.rmdirSync(uri);
        } else {
          fs.unlinkSync(uri);
        }
      }
    } catch (e) {
      RNHLogger.error(TAG, `deleteAsync failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Make a directory (optionally creating intermediate dirs)
   */
  async makeDirectoryAsync(options: MakeDirectoryOptions): Promise<void> {
    RNHLogger.info(TAG, `makeDirectoryAsync: ${options.uri}, intermediates=${options.intermediates}`);
    try {
      if (options.intermediates) {
        this.mkdirRecursive(options.uri);
      } else {
        fs.mkdirSync(options.uri);
      }
    } catch (e) {
      RNHLogger.error(TAG, `makeDirectoryAsync failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Read a file as string
   */
  async readAsStringAsync(options: ReadAsStringOptions): Promise<string> {
    RNHLogger.info(TAG, `readAsStringAsync: ${options.uri}`);
    try {
      const file = fs.openSync(options.uri, fs.OpenMode.READ_ONLY);
      try {
        const stat = fs.statSync(file.fd);
        const buffer = new ArrayBuffer(stat.size);
        fs.readSync(file.fd, buffer);
        return this.arrayBufferToText(buffer);
      } finally {
        fs.closeSync(file);
      }
    } catch (e) {
      RNHLogger.error(TAG, `readAsStringAsync failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Write a string to a file
   */
  async writeAsStringAsync(options: WriteAsStringOptions): Promise<void> {
    RNHLogger.info(TAG, `writeAsStringAsync: ${options.uri}`);
    try {
      const file = fs.openSync(options.uri, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
      try {
        const buffer = this.textToArrayBuffer(options.string);
        fs.writeSync(file.fd, buffer);
        fs.fsyncSync(file.fd);
      } finally {
        fs.closeSync(file);
      }
    } catch (e) {
      RNHLogger.error(TAG, `writeAsStringAsync failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Recursively create directories
   */
  private mkdirRecursive(dirPath: string): void {
    const segments = dirPath.split('/');
    let current = '';
    for (const segment of segments) {
      if (segment.length === 0) {
        continue;
      }
      current = current.length === 0 ? segment : `${current}/${segment}`;
      if (!fs.accessSync(current)) {
        try {
          fs.mkdirSync(current);
        } catch (e) {
          RNHLogger.debug(TAG, `mkdir ${current} skipped (may already exist): ${e}`);
        }
      }
    }
  }

  /**
   * Convert ArrayBuffer to UTF-8 text
   */
  private arrayBufferToText(buffer: ArrayBuffer): string {
    return this.textDecoder.decodeToString(new Uint8Array(buffer));
  }

  /**
   * Convert UTF-8 text to ArrayBuffer
   */
  private textToArrayBuffer(text: string): ArrayBuffer {
    const encoded: Uint8Array = this.textEncoder.encodeInto(text);
    return encoded.buffer as ArrayBuffer;
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'ExpoFileSystemModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'ExpoFileSystemModule invalidated');
    super.invalidate();
  }
}
