/**
 * RNOH-New: ExpoLinkingModule - ExpoLinking native module
 *
 * Implements the Expo 'ExpoLinking' interface. On HarmonyOS, external
 * URL handling is delegated to the application context when available.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../RNH/RNHTurboModule';
import { RNHLogger } from '../RNH/RNHLogger';
import { toError } from '../RNH/RNHErrorUtils';
import { common } from '@kit.AbilityKit';

const TAG = 'ExpoLinkingModule';

/**
 * Parsed URL structure
 */
export interface ParsedURL {
  hostname: string | null;
  path: string;
  queryParams: Map<string, string>;
  scheme: string;
}

/**
 * ExpoLinking TurboModule
 */
export class ExpoLinkingModule extends RNHTurboModule {
  private baseScheme: string = 'exp';

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'ExpoLinking';
  }

  /**
   * Create a URL from a path
   */
  createURL(path: string): string {
    RNHLogger.debug(TAG, `createURL: ${path}`);
    try {
      // Use the bundle's scheme as base if available
      const scheme = this.getAppScheme();
      const host = 'exp.host';
      return `${scheme}://${host}/${path}`;
    } catch (e) {
      RNHLogger.error(TAG, `createURL failed: ${e}`);
      return `${this.baseScheme}://exp.host/${path}`;
    }
  }

  /**
   * Parse a URL into components
   */
  parse(url: string): ParsedURL {
    RNHLogger.debug(TAG, `parse: ${url}`);
    try {
      const parsed = this.parseUrl(url);
      return parsed;
    } catch (e) {
      RNHLogger.error(TAG, `parse failed: ${e}`);
      return {
        hostname: null,
        path: '',
        queryParams: new Map(),
        scheme: '',
      };
    }
  }

  /**
   * Open a URL
   */
  async openURL(url: string): Promise<void> {
    RNHLogger.info(TAG, `openURL: ${url}`);
    try {
      const ctx = this.context.appContext as common.UIAbilityContext;
      if (ctx && ctx.startAbility) {
        await ctx.startAbility({
          action: 'ohos.want.action.viewData',
          uri: url,
        });
        return;
      }
      throw new Error(`Cannot open URL: ${url}`);
    } catch (e) {
      RNHLogger.error(TAG, `openURL failed: ${e}`);
      throw toError(e);
    }
  }

  /**
   * Get the initial URL that launched the app
   */
  async getInitialURL(): Promise<string | null> {
    RNHLogger.debug(TAG, 'getInitialURL');
    return null;
  }

  /**
   * Check if a URL can be opened
   */
  async canOpenURL(url: string): Promise<boolean> {
    RNHLogger.debug(TAG, `canOpenURL: ${url}`);
    return url.startsWith('http://') || url.startsWith('https://');
  }

  /**
   * Parse a URL string into components (pure string parsing, no deps)
   */
  private parseUrl(url: string): ParsedURL {
    // scheme://host/path?query#fragment
    const schemeMatch = url.match(/^([a-zA-Z][a-zA-Z0-9+.-]*):\/\//);
    let scheme = '';
    let rest = url;

    if (schemeMatch) {
      scheme = schemeMatch[1];
      rest = url.substring(schemeMatch[0].length);
    }

    // Split host and path
    const slashIndex = rest.indexOf('/');
    let hostname: string | null = rest;
    let path = '';

    if (slashIndex >= 0) {
      hostname = rest.substring(0, slashIndex);
      path = rest.substring(slashIndex + 1);
    }

    // Extract query params
    const queryParams: Map<string, string> = new Map();
    const hashIndex = path.indexOf('#');
    if (hashIndex >= 0) {
      path = path.substring(0, hashIndex);
    }
    const queryIndex = path.indexOf('?');
    if (queryIndex >= 0) {
      const queryStr = path.substring(queryIndex + 1);
      path = path.substring(0, queryIndex);
      const pairs = queryStr.split('&');
      for (const pair of pairs) {
        const eqIndex = pair.indexOf('=');
        if (eqIndex > 0) {
          const key = pair.substring(0, eqIndex);
          const value = pair.substring(eqIndex + 1);
          queryParams.set(decodeURIComponent(key), decodeURIComponent(value));
        }
      }
    }

    return {
      hostname,
      path,
      queryParams,
      scheme,
    };
  }

  /**
   * Get the app's URL scheme (best-effort from bundle)
   */
  private getAppScheme(): string {
    // Expo uses the scheme derived from the app slug.
    // Without manifest access, fall back to a stable scheme.
    return this.baseScheme;
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'ExpoLinkingModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'ExpoLinkingModule invalidated');
    super.invalidate();
  }
}
