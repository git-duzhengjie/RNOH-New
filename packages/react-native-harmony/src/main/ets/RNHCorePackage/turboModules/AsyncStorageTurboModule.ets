/**
 * RNOH-New: AsyncStorageTurboModule - AsyncStorage native module
 *
 * Implements React Native's AsyncStorage (RNCAsyncStorage) using @ohos.data.preferences.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import preferences from '@ohos.data.preferences';

const TAG = 'AsyncStorageTurboModule';
const STORE_NAME = 'RNHAsyncStorage';

/**
 * Key-value pair
 */
interface StorageEntry {
  key: string;
  value: string;
}

/**
 * AsyncStorage TurboModule
 */
export class AsyncStorageTurboModule extends RNHTurboModule {
  private store: preferences.Preferences | null = null;

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'RNCAsyncStorage';
  }

  /**
   * Get or create the preferences store
   */
  private getStore(): preferences.Preferences {
    if (this.store) {
      return this.store;
    }
    // appContext is Object, but preferences.getPreferencesSync expects Context.
    // The actual object passed from the ability will be a Context subclass.
    const ctx = this.context.appContext as Context;
    this.store = preferences.getPreferencesSync(ctx, { name: STORE_NAME });
    return this.store;
  }

  /**
   * Flush store changes to disk
   */
  private async flush(): Promise<void> {
    if (this.store) {
      await this.store.flush();
    }
  }

  /**
   * Get an item by key
   */
  async getItem(key: string): Promise<string | null> {
    RNHLogger.debug(TAG, `getItem: ${key}`);
    try {
      const store = this.getStore();
      if (!store.hasSync(key)) {
        return null;
      }
      const value = store.getSync(key, '');
      return String(value);
    } catch (e) {
      RNHLogger.error(TAG, `getItem failed: ${e}`);
      throw e;
    }
  }

  /**
   * Set an item
   */
  async setItem(key: string, value: string): Promise<void> {
    RNHLogger.debug(TAG, `setItem: ${key}`);
    try {
      const store = this.getStore();
      store.putSync(key, value);
      await this.flush();
    } catch (e) {
      RNHLogger.error(TAG, `setItem failed: ${e}`);
      throw e;
    }
  }

  /**
   * Remove an item
   */
  async removeItem(key: string): Promise<void> {
    RNHLogger.debug(TAG, `removeItem: ${key}`);
    try {
      const store = this.getStore();
      if (store.hasSync(key)) {
        store.deleteSync(key);
        await this.flush();
      }
    } catch (e) {
      RNHLogger.error(TAG, `removeItem failed: ${e}`);
      throw e;
    }
  }

  /**
   * Get all keys
   */
  async getAllKeys(): Promise<string[]> {
    RNHLogger.debug(TAG, 'getAllKeys');
    try {
      const store = this.getStore();
      const all: Object = store.getAllSync();
      return Object.keys(all);
    } catch (e) {
      RNHLogger.error(TAG, `getAllKeys failed: ${e}`);
      throw e;
    }
  }

  /**
   * Clear all items
   */
  async clear(): Promise<void> {
    RNHLogger.info(TAG, 'clear');
    try {
      const store = this.getStore();
      store.clearSync();
      await this.flush();
    } catch (e) {
      RNHLogger.error(TAG, `clear failed: ${e}`);
      throw e;
    }
  }

  /**
   * Get multiple items
   */
  async multiGet(keys: string[]): Promise<StorageEntry[]> {
    RNHLogger.debug(TAG, `multiGet: ${keys.length} keys`);
    const result: StorageEntry[] = [];
    for (const key of keys) {
      const value = await this.getItem(key);
      result.push({ key, value: value ?? '' });
    }
    return result;
  }

  /**
   * Set multiple items
   */
  async multiSet(entries: StorageEntry[]): Promise<void> {
    RNHLogger.debug(TAG, `multiSet: ${entries.length} entries`);
    try {
      const store = this.getStore();
      for (const entry of entries) {
        store.putSync(entry.key, entry.value);
      }
      await this.flush();
    } catch (e) {
      RNHLogger.error(TAG, `multiSet failed: ${e}`);
      throw e;
    }
  }

  /**
   * Merge multiple items (combines JSON values)
   */
  async multiMerge(entries: StorageEntry[]): Promise<void> {
    RNHLogger.debug(TAG, `multiMerge: ${entries.length} entries`);
    try {
      const store = this.getStore();
      for (const entry of entries) {
        if (!store.hasSync(entry.key)) {
          store.putSync(entry.key, entry.value);
          continue;
        }
        const existing = store.getSync(entry.key, '{}');
        const existingObj = JSON.parse(String(existing)) as Record<string, Object>;
        const newObj = JSON.parse(entry.value) as Record<string, Object>;
        const merged = { ...existingObj, ...newObj };
        store.putSync(entry.key, JSON.stringify(merged));
      }
      await this.flush();
    } catch (e) {
      RNHLogger.error(TAG, `multiMerge failed: ${e}`);
      throw e;
    }
  }

  /**
   * Remove multiple items
   */
  async multiRemove(keys: string[]): Promise<void> {
    RNHLogger.debug(TAG, `multiRemove: ${keys.length} keys`);
    try {
      const store = this.getStore();
      for (const key of keys) {
        if (store.hasSync(key)) {
          store.deleteSync(key);
        }
      }
      await this.flush();
    } catch (e) {
      RNHLogger.error(TAG, `multiRemove failed: ${e}`);
      throw e;
    }
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'AsyncStorageTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'AsyncStorageTurboModule invalidated');
    // preferences does not have an explicit close API on all versions,
    // so we just clear the reference.
    this.store = null;
    super.invalidate();
  }
}
