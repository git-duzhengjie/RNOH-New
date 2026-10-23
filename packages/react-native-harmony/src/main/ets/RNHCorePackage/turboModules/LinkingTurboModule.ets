/**
 * RNOH-New: LinkingTurboModule - LinkingManager native module
 *
 * Implements React Native's LinkingManager interface using @kit.LinkingKit.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import linking from '@kit.LinkingKit';

const TAG = 'LinkingTurboModule';

/**
 * URL change listener
 */
type URLChangeListener = (url: string) => void;

/**
 * LinkingManager TurboModule
 */
export class LinkingTurboModule extends RNHTurboModule {
  private initialURL: string | null = null;
  private urlListeners: Set<URLChangeListener> = new Set();
  private linkingCallback?: (link: string) => void;

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'LinkingManager';
  }

  /**
   * Get the initial URL that launched the app
   */
  async getInitialURL(): Promise<string | null> {
    RNHLogger.debug(TAG, 'getInitialURL');
    if (this.initialURL) {
      return this.initialURL;
    }
    try {
      const url = await linking.getLinkingUri();
      this.initialURL = url;
      return url;
    } catch (e) {
      RNHLogger.warn(TAG, `getInitialURL failed: ${e}`);
      return null;
    }
  }

  /**
   * Open a URL
   */
  async openURL(url: string): Promise<void> {
    RNHLogger.info(TAG, `openURL: ${url}`);
    try {
      const canOpen = await linking.canOpenLink(url);
      if (!canOpen) {
        throw new Error(`Cannot open URL: ${url}`);
      }
      await linking.openLink(url);
    } catch (e) {
      RNHLogger.error(TAG, `openURL failed: ${e}`);
      throw e;
    }
  }

  /**
   * Check if a URL can be opened
   */
  async canOpenURL(url: string): Promise<boolean> {
    RNHLogger.debug(TAG, `canOpenURL: ${url}`);
    try {
      return await linking.canOpenLink(url);
    } catch (e) {
      RNHLogger.error(TAG, `canOpenURL failed: ${e}`);
      return false;
    }
  }

  /**
   * Add event listener for URL changes
   */
  addEventListener(eventName: string, listener: URLChangeListener): void {
    if (eventName !== 'url') {
      RNHLogger.warn(TAG, `Unsupported event: ${eventName}`);
      return;
    }

    const isFirstListener = this.urlListeners.size === 0;
    this.urlListeners.add(listener);

    if (isFirstListener) {
      this.startListening();
    }

    RNHLogger.debug(TAG, `Added URL listener, total: ${this.urlListeners.size}`);
  }

  /**
   * Remove event listener
   */
  removeEventListener(eventName: string, listener: URLChangeListener): void {
    if (eventName !== 'url') {
      return;
    }
    this.urlListeners.delete(listener);
    RNHLogger.debug(TAG, `Removed URL listener, total: ${this.urlListeners.size}`);

    if (this.urlListeners.size === 0) {
      this.stopListening();
    }
  }

  /**
   * Start listening for incoming links
   */
  private startListening(): void {
    RNHLogger.info(TAG, 'Start listening for linking events');
    this.linkingCallback = (link: string) => {
      this.handleURLChange(link);
    };
    try {
      linking.on('url', this.linkingCallback);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to register linking listener: ${e}`);
      this.linkingCallback = undefined;
    }
  }

  /**
   * Stop listening for incoming links
   */
  private stopListening(): void {
    if (this.linkingCallback) {
      try {
        linking.off('url', this.linkingCallback);
      } catch (e) {
        RNHLogger.error(TAG, `Failed to unregister linking listener: ${e}`);
      }
      this.linkingCallback = undefined;
    }
  }

  /**
   * Handle URL change
   */
  private handleURLChange(url: string): void {
    RNHLogger.info(TAG, `URL changed: ${url}`);
    for (const listener of this.urlListeners) {
      try {
        listener(url);
      } catch (e) {
        RNHLogger.error(TAG, `Error in URL listener: ${e}`);
      }
    }
    this.emitEvent('onURLReceived', JSON.stringify({ url }));
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'LinkingTurboModule initialized');
  }

  override invalidate(): void {
    this.stopListening();
    this.urlListeners.clear();
    RNHLogger.info(TAG, 'LinkingTurboModule invalidated');
    super.invalidate();
  }
}
