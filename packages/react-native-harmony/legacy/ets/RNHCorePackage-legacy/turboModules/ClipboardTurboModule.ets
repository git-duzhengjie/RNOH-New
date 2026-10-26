/**
 * RNOH-New: ClipboardTurboModule - Provides Clipboard native module
 *
 * Implements React Native's Clipboard interface using @ohos.pasteboard.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import pasteboard from '@ohos.pasteboard';

const TAG = 'ClipboardTurboModule';

/**
 * Clipboard TurboModule
 */
export class ClipboardTurboModule extends RNHTurboModule {
  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'Clipboard';
  }

  /**
   * Get string from system clipboard
   */
  async getString(): Promise<string> {
    RNHLogger.info(TAG, 'Getting string from clipboard');
    try {
      const systemPasteboard = pasteboard.getSystemPasteboard();
      const pasteData = await systemPasteboard.getData();
      if (!pasteData) {
        return '';
      }
      const text = pasteData.getPrimaryText();
      RNHLogger.info(TAG, 'Clipboard string retrieved successfully');
      return text ?? '';
    } catch (e) {
      RNHLogger.error(TAG, `Failed to get clipboard string: ${e}`);
      return '';
    }
  }

  /**
   * Set string to system clipboard
   */
  async setString(text: string): Promise<boolean> {
    RNHLogger.info(TAG, 'Setting string to clipboard');
    try {
      const systemPasteboard = pasteboard.getSystemPasteboard();
      const pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, text);
      await systemPasteboard.setData(pasteData);
      RNHLogger.info(TAG, 'Clipboard string set successfully');
      return true;
    } catch (e) {
      RNHLogger.error(TAG, `Failed to set clipboard string: ${e}`);
      return false;
    }
  }

  /**
   * Check if clipboard has strings
   */
  async hasString(): Promise<boolean> {
    try {
      const systemPasteboard = pasteboard.getSystemPasteboard();
      const hasData = await systemPasteboard.hasData();
      if (!hasData) {
        return false;
      }
      const mimeTypes = await systemPasteboard.getMimeTypes();
      return mimeTypes.includes(pasteboard.MIMETYPE_TEXT_PLAIN);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to check clipboard: ${e}`);
      return false;
    }
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'ClipboardTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'ClipboardTurboModule invalidated');
    super.invalidate();
  }
}
