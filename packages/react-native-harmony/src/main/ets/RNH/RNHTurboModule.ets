/**
 * RNOH-New: RNHTurboModule - Base class for TurboModules
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHTurboModule';

/**
 * Context provided to TurboModules
 */
export interface RNHTurboModuleContext {
  // Instance ID
  instanceId: number;

  // App context
  appContext: Object;

  // Event emitter
  eventEmitter: (eventName: string, payload: string) => void;
}

/**
 * Base class for all TurboModule implementations
 */
export class RNHTurboModule {
  protected context: RNHTurboModuleContext;

  constructor(context: RNHTurboModuleContext) {
    this.context = context;
  }

  /**
   * Get the module name (must match JS-side name)
   */
  getName(): string {
    return 'RNHTurboModule';
  }

  /**
   * Initialize the module
   */
  initialize(): void {
    RNHLogger.debug(TAG, `Initializing TurboModule: ${this.getName()}`);
  }

  /**
   * Cleanup the module
   */
  invalidate(): void {
    RNHLogger.debug(TAG, `Invalidating TurboModule: ${this.getName()}`);
  }

  /**
   * Emit an event to JS
   */
  protected emitEvent(eventName: string, payload: string): void {
    this.context.eventEmitter(eventName, payload);
  }
}
