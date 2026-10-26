/**
 * RNOH-New: RNHTurboModuleGuard - TurboModule call protection
 *
 * Provides timeout enforcement and rate limiting for all TurboModule calls.
 * Prevents hangs from slow native operations and abuse from rapid calls.
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHTurboModuleGuard';

/**
 * Default timeout for TurboModule calls (ms)
 */
const DEFAULT_TIMEOUT_MS: number = 30000;

/**
 * Default max calls per second per module method
 */
const DEFAULT_MAX_CALLS_PER_SECOND: number = 100;

/**
 * TurboModule guard statistics
 */
export interface TurboModuleGuardStats {
  totalCalls: number;
  rejectedCalls: number;
  timedOutCalls: number;
  trackedMethods: number;
}

/**
 * Rate limit entry for a specific module+method
 */
interface RateLimitEntry {
  timestamps: number[];
  maxCalls: number;
}

/**
 * Execute options for TurboModule call protection
 */
interface ExecuteOptions {
  timeoutMs?: number;
  maxCallsPerSecond?: number;
}

/**
 * TurboModule call guard with timeout and rate limiting
 */
export class RNHTurboModuleGuard {
  private rateLimits: Map<string, RateLimitEntry> = new Map();
  private defaultTimeoutMs: number = DEFAULT_TIMEOUT_MS;
  private defaultMaxCallsPerSecond: number = DEFAULT_MAX_CALLS_PER_SECOND;
  private totalCalls: number = 0;
  private rejectedCalls: number = 0;
  private timedOutCalls: number = 0;

  /**
   * Wrap a promise with timeout protection.
   * Rejects with a timeout error if the promise doesn't resolve in time.
   */
  withTimeout<T>(promise: Promise<T>, timeoutMs: number = this.defaultTimeoutMs): Promise<T> {
    return new Promise<T>((resolve, reject) => {
      const timeoutId = setTimeout(() => {
        this.timedOutCalls++;
        RNHLogger.error(TAG, `TurboModule call timed out after ${timeoutMs}ms`);
        reject(new Error(`TurboModule call timed out after ${timeoutMs}ms`));
      }, timeoutMs);

      promise
        .then((result: T) => {
          clearTimeout(timeoutId);
          resolve(result);
        })
        .catch((error: Object) => {
          clearTimeout(timeoutId);
          reject(error);
        });
    });
  }

  /**
   * Check if a module+method call is within the rate limit.
   * Returns true if allowed, false if rate limited.
   */
  checkRateLimit(moduleName: string, methodName: string,
                 maxCallsPerSecond: number = this.defaultMaxCallsPerSecond): boolean {
    const key = `${moduleName}.${methodName}`;
    const now = Date.now();
    const windowStart = now - 1000;

    let entry = this.rateLimits.get(key);
    if (!entry) {
      entry = {
        timestamps: [],
        maxCalls: maxCallsPerSecond,
      };
      this.rateLimits.set(key, entry);
    }

    // Remove timestamps outside the 1-second window
    while (entry.timestamps.length > 0 && entry.timestamps[0] < windowStart) {
      entry.timestamps.shift();
    }

    if (entry.timestamps.length >= entry.maxCalls) {
      this.rejectedCalls++;
      RNHLogger.warn(
        TAG,
        `Rate limit exceeded for ${key}: ${entry.timestamps.length} calls in 1s`
      );
      return false;
    }

    entry.timestamps.push(now);
    this.totalCalls++;
    return true;
  }

  /**
   * Execute a TurboModule method with both timeout and rate limit protection.
   */
  execute<T>(
    moduleName: string,
    methodName: string,
    fn: () => Promise<T>,
    options?: ExecuteOptions
  ): Promise<T> {
    const timeoutMs = options?.timeoutMs ?? this.defaultTimeoutMs;
    const maxCallsPerSecond = options?.maxCallsPerSecond ?? this.defaultMaxCallsPerSecond;

    // Check rate limit first
    if (!this.checkRateLimit(moduleName, methodName, maxCallsPerSecond)) {
      return Promise.reject(
        new Error(`Rate limit exceeded for ${moduleName}.${methodName}`)
      );
    }

    // Execute with timeout
    try {
      const promise = fn();
      return this.withTimeout(promise, timeoutMs);
    } catch (e) {
      RNHLogger.error(TAG, `Failed to execute ${moduleName}.${methodName}: ${e}`);
      return Promise.reject(e);
    }
  }

  /**
   * Set the default timeout for all calls
   */
  setDefaultTimeout(timeoutMs: number): void {
    this.defaultTimeoutMs = timeoutMs;
    RNHLogger.info(TAG, `Default timeout set to ${timeoutMs}ms`);
  }

  /**
   * Set the default max calls per second
   */
  setDefaultMaxCallsPerSecond(max: number): void {
    this.defaultMaxCallsPerSecond = max;
    RNHLogger.info(TAG, `Default max calls per second set to ${max}`);
  }

  /**
   * Configure a custom rate limit for a specific module method
   */
  configureRateLimit(moduleName: string, methodName: string, maxCallsPerSecond: number): void {
    const key = `${moduleName}.${methodName}`;
    let entry = this.rateLimits.get(key);
    if (!entry) {
      entry = {
        timestamps: [],
        maxCalls: maxCallsPerSecond,
      };
      this.rateLimits.set(key, entry);
    } else {
      entry.maxCalls = maxCallsPerSecond;
    }
    RNHLogger.info(TAG, `Rate limit configured: ${key} = ${maxCallsPerSecond}/s`);
  }

  /**
   * Get guard statistics for diagnostics
   */
  getStats(): TurboModuleGuardStats {
    const stats: TurboModuleGuardStats = {
      totalCalls: this.totalCalls,
      rejectedCalls: this.rejectedCalls,
      timedOutCalls: this.timedOutCalls,
      trackedMethods: this.rateLimits.size,
    };
    return stats;
  }

  /**
   * Reset all counters and rate limit state
   */
  reset(): void {
    this.rateLimits.clear();
    this.totalCalls = 0;
    this.rejectedCalls = 0;
    this.timedOutCalls = 0;
    RNHLogger.info(TAG, 'TurboModule guard reset');
  }

  /**
   * Destroy the guard
   */
  destroy(): void {
    this.rateLimits.clear();
    RNHLogger.info(
      TAG,
      `TurboModule guard destroyed (total: ${this.totalCalls}, rejected: ${this.rejectedCalls}, timed out: ${this.timedOutCalls})`
    );
  }
}
