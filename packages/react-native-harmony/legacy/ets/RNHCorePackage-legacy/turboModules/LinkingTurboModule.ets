/**
 * RNOH-New: LinkingTurboModule - LinkingManager native module
 *
 * Implements React Native's LinkingManager interface.
 * On HarmonyOS, external URL handling is delegated to the application context
 * via startAbility when available; unsupported operations return safe defaults
 * to satisfy the TurboModule contract.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import { common } from '@kit.AbilityKit';

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

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'LinkingManager';
  }

  /**
   * Get the initial URL that launched the app.
   * HarmonyOS does not expose a dedicated Linking URI API, return null.
   */
  async getInitialURL(): Promise<string | null> {
    RNHLogger.debug(TAG, 'getInitialURL');
    return this.initialURL;
  }

  /**
   * Open a URL.
   * Best-effort via the application context's startAbility when supported.
   */
  async openURL(url: string): Promise<void> {
    RNHLogger.info(TAG, `openURL: ${url}`);
    try {
      const ctx = this.context.appContext as common.UIAbilityContext;
      if (ctx && ctx.startAbility) {
        // Use an implicit want to open URLs through the system browser
        await ctx.startAbility({
          action: 'ohos.want.action.viewData',
          uri: url,
        });
        return;
      }
      RNHLogger.warn(TAG, `No application context available to open ${url}`);
    } catch (e) {
      RNHLogger.error(TAG, `openURL failed: ${e}`);
      throw new Error(`Cannot open URL: ${url}`);
    }
  }

  /**
   * Check if a URL can be opened.
   * Returns true for http/https as a conservative estimate.
   */
  async canOpenURL(url: string): Promise<boolean> {
    RNHLogger.debug(TAG, `canOpenURL: ${url}`);
    return url.startsWith('http://') || url.startsWith('https://');
  }

  /**
   * Add event listener for URL changes.
   * This is a stub on HarmonyOS because there is no public Linking event API.
   */
  addEventListener(eventName: string, listener: URLChangeListener): void {
    if (eventName !== 'url') {
      RNHLogger.warn(TAG, `Unsupported event: ${eventName}`);
      return;
    }

    this.urlListeners.add(listener);
    RNHLogger.debug(TAG, `Added URL listener, total: ${this.urlListeners.size}`);
    RNHLogger.info(TAG, 'URL change listening is stubbed on HarmonyOS');
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
  }

  /**
   * Notify JS listeners of an incoming URL (called by the host Ability).
   */
  handleURL(url: string): void {
    RNHLogger.info(TAG, `URL received: ${url}`);
    this.initialURL = url;
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
    this.urlListeners.clear();
    RNHLogger.info(TAG, 'LinkingTurboModule invalidated');
    super.invalidate();
  }
}
