/**
 * RNOH-New: RNHEventEmitter - Event emitter for native-to-JS events
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHEventEmitter';

/**
 * Event listener type
 */
type RNHEventListener = (payload: string) => void;

/**
 * Event emitter for dispatching events from native to JS
 */
export class RNHEventEmitter {
  private listeners: Map<string, Set<RNHEventListener>> = new Map();

  /**
   * Add an event listener
   */
  addListener(eventName: string, listener: RNHEventListener): void {
    if (!this.listeners.has(eventName)) {
      this.listeners.set(eventName, new Set());
    }
    this.listeners.get(eventName)!.add(listener);
    RNHLogger.debug(TAG, `Listener added for: ${eventName}`);
  }

  /**
   * Remove an event listener
   */
  removeListener(eventName: string, listener: RNHEventListener): void {
    this.listeners.get(eventName)?.delete(listener);
    RNHLogger.debug(TAG, `Listener removed for: ${eventName}`);
  }

  /**
   * Remove all listeners for an event
   */
  removeAllListeners(eventName: string): void {
    this.listeners.delete(eventName);
  }

  /**
   * Emit an event to all listeners
   */
  emit(eventName: string, payload: string): void {
    const eventListeners = this.listeners.get(eventName);
    if (eventListeners) {
      for (const listener of eventListeners) {
        try {
          listener(payload);
        } catch (e) {
          RNHLogger.error(TAG, `Error in event listener for ${eventName}: ${e}`);
        }
      }
    }
  }

  /**
   * Check if there are listeners for an event
   */
  hasListeners(eventName: string): boolean {
    return this.listeners.has(eventName) &&
           this.listeners.get(eventName)!.size > 0;
  }

  /**
   * Get listener count for an event
   */
  listenerCount(eventName: string): number {
    return this.listeners.get(eventName)?.size ?? 0;
  }
}
