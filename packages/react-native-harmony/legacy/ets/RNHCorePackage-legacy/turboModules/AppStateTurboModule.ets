/**
 * RNOH-New: AppStateTurboModule - Provides app state to JS
 *
 * Implements the RN AppState native module interface.
 * Tracks app foreground/background state and emits change events.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';

const TAG = 'AppStateTurboModule';

/**
 * App state values matching RN AppState
 */
export type AppStateStatus = 'active' | 'background' | 'inactive';

/**
 * App state event types
 */
export type AppStateEventType = 'change' | 'memoryWarning' | 'blur' | 'focus';

/**
 * Event listener callback type
 */
type AppStateListener = (state: AppStateStatus) => void;

/**
 * AppState TurboModule
 */
export class AppStateTurboModule extends RNHTurboModule {
  private currentState: AppStateStatus = 'active';
  private listeners: Map<string, Set<AppStateListener>> = new Map();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'AppState';
  }

  /**
   * Get current app state
   */
  getCurrentAppState(): AppStateStatus {
    return this.currentState;
  }

  /**
   * Set app state (called by native side when app state changes)
   */
  setAppState(state: AppStateStatus): void {
    if (this.currentState !== state) {
      const prevState = this.currentState;
      this.currentState = state;
      RNHLogger.info(TAG, `App state changed: ${prevState} -> ${state}`);
      this.notifyListeners('change', state);
    }
  }

  /**
   * Add event listener
   */
  addEventListener(eventType: AppStateEventType, listener: AppStateListener): void {
    if (!this.listeners.has(eventType)) {
      this.listeners.set(eventType, new Set());
    }
    this.listeners.get(eventType)?.add(listener);
    RNHLogger.debug(TAG, `Added listener for event: ${eventType}`);
  }

  /**
   * Remove event listener
   */
  removeEventListener(eventType: AppStateEventType, listener: AppStateListener): void {
    this.listeners.get(eventType)?.delete(listener);
    RNHLogger.debug(TAG, `Removed listener for event: ${eventType}`);
  }

  /**
   * Notify all listeners of an event
   */
  private notifyListeners(eventType: AppStateEventType, state: AppStateStatus): void {
    const listeners = this.listeners.get(eventType);
    if (listeners) {
      for (const listener of listeners) {
        try {
          listener(state);
        } catch (e) {
          RNHLogger.error(TAG, `Error in AppState listener: ${e}`);
        }
      }
    }
    // Also emit to JS side via bridge
    this.emitEvent(eventType, JSON.stringify({ state }));
  }

  /**
   * Handle app foreground
   */
  onForeground(): void {
    this.setAppState('active');
  }

  /**
   * Handle app background
   */
  onBackground(): void {
    this.setAppState('background');
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'AppStateTurboModule initialized');
  }

  override invalidate(): void {
    this.listeners.clear();
    RNHLogger.info(TAG, 'AppStateTurboModule invalidated');
    super.invalidate();
  }
}
