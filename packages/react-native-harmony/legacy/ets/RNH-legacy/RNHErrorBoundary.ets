/**
 * RNOH-New: RNHErrorBoundary - Global exception catcher
 *
 * Wraps callbacks, promises and TurboModule calls with try-catch.
 * Catches errors, logs them, and emits 'rnhError' events to JS.
 */

import { RNHLogger } from './RNHLogger';
import { toError } from './RNHErrorUtils';

const TAG = 'RNHErrorBoundary';

/**
 * Error payload emitted to JS
 */
export interface RNHErrorPayload {
  message: string;
  stack: string;
  source: string;
  timestamp: number;
}

/**
 * Error event listener signature
 */
export type ErrorListener = (payload: RNHErrorPayload) => void;

/**
 * Global error boundary for the RNH instance
 */
export class RNHErrorBoundary {
  private listeners: Set<ErrorListener> = new Set();
  private errorCount: number = 0;
  private lastErrorTime: number = 0;
  private readonly maxErrorsPerSecond: number = 10;

  /**
   * Wrap a synchronous callback with error handling.
   * Returns the result on success, returns null on error.
   */
  wrapCallback<T>(fn: () => T, source: string = 'callback'): T | null {
    try {
      return fn();
    } catch (e) {
      this.handleError(e, source);
      return null;
    }
  }

  /**
   * Wrap a promise with error handling.
   * Rejects with the original error after logging.
   */
  wrapPromise<T>(promise: Promise<T>, source: string = 'promise'): Promise<T> {
    return promise.catch((e: Object) => {
      this.handleError(e, source);
      throw toError(e);
    });
  }

  /**
   * Wrap an async function with error handling.
   * Returns a new function that catches errors.
   */
  wrapAsyncFn<T extends Object[]>(
    fn: (...args: T) => Promise<Object>,
    source: string = 'asyncFn'
  ): (...args: T) => Promise<Object> {
    return (...args: T): Promise<Object> => {
      try {
        const result = fn(...args);
        return this.wrapPromise(result, source);
      } catch (e) {
        this.handleError(e, source);
        return Promise.reject(e);
      }
    };
  }

  /**
   * Handle a caught error: log, rate-limit, emit to listeners
   */
  handleError(error: Object, source: string): void {
    const now = Date.now();

    // Rate limit error emission to avoid flooding
    if (now - this.lastErrorTime < 1000 / this.maxErrorsPerSecond) {
      this.errorCount++;
      RNHLogger.warn(TAG, `Error suppressed (rate limit): ${error}`);
      return;
    }
    this.lastErrorTime = now;
    this.errorCount++;

    const message = this.extractMessage(error);
    const stack = this.extractStack(error);

    RNHLogger.error(TAG, `[${source}] ${message}`);
    if (stack.length > 0) {
      RNHLogger.error(TAG, `Stack: ${stack}`);
    }

    const payload: RNHErrorPayload = {
      message,
      stack,
      source,
      timestamp: now,
    };

    // Notify all listeners
    for (const listener of this.listeners) {
      try {
        listener(payload);
      } catch (listenerError) {
        RNHLogger.error(TAG, `Error listener threw: ${listenerError}`);
      }
    }
  }

  /**
   * Add an error listener
   */
  addListener(listener: ErrorListener): void {
    this.listeners.add(listener);
  }

  /**
   * Remove an error listener
   */
  removeListener(listener: ErrorListener): void {
    this.listeners.delete(listener);
  }

  /**
   * Get total error count
   */
  getErrorCount(): number {
    return this.errorCount;
  }

  /**
   * Extract message from an error object
   */
  private extractMessage(error: Object): string {
    if (error instanceof Error) {
      return error.message;
    }
    if (typeof error === 'string') {
      return error;
    }
    try {
      return JSON.stringify(error);
    } catch {
      return String(error);
    }
  }

  /**
   * Extract stack trace from an error object
   */
  private extractStack(error: Object): string {
    if (error instanceof Error && error.stack) {
      return error.stack;
    }
    return '';
  }

  /**
   * Reset error counter (for testing / diagnostics)
   */
  reset(): void {
    this.errorCount = 0;
    this.lastErrorTime = 0;
    RNHLogger.info(TAG, 'Error boundary reset');
  }

  /**
   * Destroy the error boundary and clear all listeners
   */
  destroy(): void {
    this.listeners.clear();
    RNHLogger.info(TAG, `Error boundary destroyed (total errors: ${this.errorCount})`);
  }
}
