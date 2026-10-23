/**
 * RNOH-New: FileSystemTurboModule - File system native module
 *
 * Implements a subset of React Native FS using @ohos.file.fs.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import fs from '@ohos.file.fs';

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
 * RNFSManager TurboModule
 */
export class FileSystemTurboModule extends RNHTurboModule {
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
      throw e;
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
      throw e;
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
          result.push({
            name,
            path: entryPath,
            size: stat.size,
            ctime: stat.ctime,
            mtime: stat.mtime,
          });
        } catch (e) {
          RNHLogger.warn(TAG, `Failed to stat ${entryPath}: ${e}`);
        }
      }
      return result;
    } catch (e) {
      RNHLogger.error(TAG, `readDir failed: ${e}`);
      throw e;
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
      throw e;
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
      throw e;
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
  async stat(path: string): Promise<{ size: number; ctime: number; mtime: number; isFile: boolean; isDirectory: boolean }> {
    RNHLogger.debug(TAG, `stat: ${path}`);
    try {
      const stat = fs.statSync(path);
      return {
        size: stat.size,
        ctime: stat.ctime,
        mtime: stat.mtime,
        isFile: stat.isFile(),
        isDirectory: stat.isDirectory(),
      };
    } catch (e) {
      RNHLogger.error(TAG, `stat failed: ${e}`);
      throw e;
    }
  }

  /**
   * Convert ArrayBuffer to base64 string
   */
  private arrayBufferToBase64(buffer: ArrayBuffer): string {
    const bytes = new Uint8Array(buffer);
    let binary = '';
    for (let i = 0; i < bytes.byteLength; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }

  /**
   * Convert base64 string to ArrayBuffer
   */
  private base64ToArrayBuffer(base64: string): ArrayBuffer {
    const binary = atob(base64);
    const buffer = new ArrayBuffer(binary.length);
    const bytes = new Uint8Array(buffer);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return buffer;
  }

  /**
   * Convert ArrayBuffer to UTF-8 text
   */
  private arrayBufferToText(buffer: ArrayBuffer): string {
    const decoder = new TextDecoder('utf-8');
    return decoder.decode(buffer);
  }

  /**
   * Convert UTF-8 text to ArrayBuffer
   */
  private textToArrayBuffer(text: string): ArrayBuffer {
    const encoder = new TextEncoder();
    return encoder.encodeInto(text).buffer;
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
