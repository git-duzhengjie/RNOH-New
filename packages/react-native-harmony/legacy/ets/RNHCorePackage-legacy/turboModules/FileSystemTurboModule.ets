/**
 * RNOH-New: FileSystemTurboModule - File system native module
 *
 * Implements a subset of React Native FS using @ohos.file.fs.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import { toError } from '../../RNH/RNHErrorUtils';
import fs from '@ohos.file.fs';
import util from '@ohos.util';

const TAG = 'FileSystemTurboModule';

/**
 * File encoding type
 */
type FileEncoding = 'utf8' | 'base64' | 'ascii' | undefined;

/**
 * Directory entry info
 */
export interface DirectoryEntry {
  name: string;
  path: string;
  size: number;
  ctime: number;
  mtime: number;
}

/**
 * File stat result
 */
export interface FileStatResult {
  size: number;
  ctime: number;
  mtime: number;
  isFile: boolean;
  isDirectory: boolean;
}

/**
 * RNFSManager TurboModule
 */
export class FileSystemTurboModule extends RNHTurboModule {
  private base64Helper: util.Base64Helper = new util.Base64Helper();
  private textDecoder: util.TextDecoder = new util.TextDecoder('utf-8');
  private textEncoder: util.TextEncoder = new util.TextEncoder();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'RNFSManager';
  }

  /**
   * Read file content
   */
  async readFile(path: string, encoding?: FileEncoding): Promise<string | ArrayBuffer> {
    RNHLogger.info(TAG, `readFile: ${path} (${encoding ?? 'binary'})`);
    try {
      const file = fs.openSync(path, fs.OpenMode.READ_ONLY);
      try {
        const stat = fs.statSync(file.fd);
        const buffer = new ArrayBuffer(stat.size);
        fs.readSync(file.fd, buffer);

        if (encoding === 'utf8') {
          return this.arrayBufferToText(buffer);
        } else if (encoding === 'base64') {
          return this.arrayBufferToBase64(buffer);
        } else {
          return buffer;
        }
      } finally {
        fs.closeSync(file);
      }
    } catch (e) {
      RNHLogger.error(TAG, `readFile failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Write content to file
   */
  async writeFile(path: string, content: string | ArrayBuffer,
                  encoding?: FileEncoding): Promise<void> {
    RNHLogger.info(TAG, `writeFile: ${path} (${encoding ?? 'binary'})`);
    try {
      const file = fs.openSync(path, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
      try {
        let buffer: ArrayBuffer;
        if (typeof content === 'string') {
          if (encoding === 'base64') {
            buffer = this.base64ToArrayBuffer(content);
          } else {
            buffer = this.textToArrayBuffer(content);
          }
        } else {
          buffer = content;
        }
        fs.writeSync(file.fd, buffer);
        fs.fsyncSync(file.fd);
      } finally {
        fs.closeSync(file);
      }
    } catch (e) {
      RNHLogger.error(TAG, `writeFile failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Read directory entries
   */
  async readDir(path: string): Promise<DirectoryEntry[]> {
    RNHLogger.info(TAG, `readDir: ${path}`);
    try {
      const list = fs.listFileSync(path);
      const result: DirectoryEntry[] = [];
      for (const name of list) {
        const entryPath = `${path}/${name}`;
        try {
          const stat = fs.statSync(entryPath);
          const entry: DirectoryEntry = {
            name,
            path: entryPath,
            size: stat.size,
            ctime: stat.ctime,
            mtime: stat.mtime,
          };
          result.push(entry);
        } catch (e) {
          RNHLogger.warn(TAG, `Failed to stat ${entryPath}: ${e}`);
        }
      }
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `readDir failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Create a directory
   */
  async mkdir(path: string): Promise<void> {
    RNHLogger.info(TAG, `mkdir: ${path}`);
    try {
      fs.mkdirSync(path);
    } catch (e) {
      RNHLogger.error(TAG, `mkdir failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Delete a file or directory
   */
  async delete(path: string): Promise<void> {
    RNHLogger.info(TAG, `delete: ${path}`);
    try {
      fs.unlinkSync(path);
    } catch (e) {
      RNHLogger.error(TAG, `delete failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Check if path exists
   */
  async exists(path: string): Promise<boolean> {
    RNHLogger.debug(TAG, `exists: ${path}`);
    try {
      return fs.accessSync(path);
    } catch (e) {
      RNHLogger.error(TAG, `exists failed: ${e}`);
      return false;
    }
  }

  /**
   * Get file stat
   */
  async stat(path: string): Promise<FileStatResult> {
    RNHLogger.debug(TAG, `stat: ${path}`);
    try {
      const stat = fs.statSync(path);
      const result: FileStatResult = {
        size: stat.size,
        ctime: stat.ctime,
        mtime: stat.mtime,
        isFile: stat.isFile(),
        isDirectory: stat.isDirectory(),
      };
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `stat failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Convert ArrayBuffer to base64 string
   */
  private arrayBufferToBase64(buffer: ArrayBuffer): string {
    return this.base64Helper.encodeToStringSync(new Uint8Array(buffer));
  }

  /**
   * Convert base64 string to ArrayBuffer
   */
  private base64ToArrayBuffer(base64: string): ArrayBuffer {
    return this.base64Helper.decodeSync(base64).buffer;
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
    RNHLogger.info(TAG, 'FileSystemTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'FileSystemTurboModule invalidated');
    super.invalidate();
  }
}
