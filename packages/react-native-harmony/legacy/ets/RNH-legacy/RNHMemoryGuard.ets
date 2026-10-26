/**
 * RNOH-New: RNHMemoryGuard - Memory leak prevention tool
 *
 * Tracks surfaces and ensures all resources (timers, listeners, refs)
 * are properly cleaned up when a surface is destroyed.
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHMemoryGuard';

/**
 * Registered cleanup function signature
 */
type CleanupFn = () => void;

/**
 * Surface metadata tracked by the guard
 */
interface SurfaceEntry {
  surfaceId: number;
  cleanupFns: Set<CleanupFn>;
  timers: Set<number>;
  listeners: Set<Object>;
  weakRefs: Set<WeakRef<Object>>;
  createdAt: number;
}

/**
 * Memory guard statistics
 */
export interface MemoryGuardStats {
  surfaces: number;
  timers: number;
  listeners: number;
  cleanupFns: number;
  weakRefs: number;
}

/**
 * Memory guard for surface lifecycle management
 */
export class RNHMemoryGuard {
  private surfaces: Map<number, SurfaceEntry> = new Map();
  private globalTimers: Set<number> = new Set();
  private globalListeners: Set<Object> = new Set();

  /**
   * Register a surface for tracking
   */
  registerSurface(surfaceId: number): void {
    if (this.surfaces.has(surfaceId)) {
      RNHLogger.warn(TAG, `Surface ${surfaceId} already registered`);
      return;
    }

    const entry: SurfaceEntry = {
      surfaceId,
      cleanupFns: new Set<CleanupFn>(),
      timers: new Set<number>(),
      listeners: new Set<Object>(),
      weakRefs: new Set<WeakRef<Object>>(),
      createdAt: Date.now(),
    };

    this.surfaces.set(surfaceId, entry);
    RNHLogger.info(TAG, `Surface ${surfaceId} registered`);
  }

  /**
   * Register a cleanup function for a surface
   */
  addCleanupFn(surfaceId: number, fn: CleanupFn): void {
    const entry = this.surfaces.get(surfaceId);
    if (!entry) {
      RNHLogger.warn(TAG, `Cannot add cleanup fn: surface ${surfaceId} not found`);
      return;
    }
    entry.cleanupFns.add(fn);
  }

  /**
   * Register a timer ID for a surface
   */
  trackTimer(surfaceId: number, timerId: number): void {
    const entry = this.surfaces.get(surfaceId);
    if (!entry) {
      // Track globally if no surface context
      this.globalTimers.add(timerId);
      return;
    }
    entry.timers.add(timerId);
  }

  /**
   * Untrack a timer (when it's cleared normally)
   */
  untrackTimer(surfaceId: number, timerId: number): void {
    const entry = this.surfaces.get(surfaceId);
    if (entry) {
      entry.timers.delete(timerId);
    }
    this.globalTimers.delete(timerId);
  }

  /**
   * Register an event listener reference for a surface
   */
  trackListener(surfaceId: number, listener: Object): void {
    const entry = this.surfaces.get(surfaceId);
    if (!entry) {
      this.globalListeners.add(listener);
      return;
    }
    entry.listeners.add(listener);
  }

  /**
   * Track a weak reference (for non-critical objects)
   */
  trackWeakRef(surfaceId: number, ref: WeakRef<Object>): void {
    const entry = this.surfaces.get(surfaceId);
    if (!entry) {
      RNHLogger.warn(TAG, `Cannot track weak ref: surface ${surfaceId} not found`);
      return;
    }
    entry.weakRefs.add(ref);
  }

  /**
   * Clean up all resources for a surface
   */
  cleanupSurface(surfaceId: number): void {
    const entry = this.surfaces.get(surfaceId);
    if (!entry) {
      RNHLogger.warn(TAG, `Cannot cleanup: surface ${surfaceId} not found`);
      return;
    }

    RNHLogger.info(TAG, `Cleaning up surface ${surfaceId}...`);

    // 1. Run all registered cleanup functions
    for (const fn of entry.cleanupFns) {
      try {
        fn();
      } catch (e) {
        RNHLogger.error(TAG, `Cleanup fn failed for surface ${surfaceId}: ${e}`);
      }
    }
    entry.cleanupFns.clear();

    // 2. Clear all timers
    for (const timerId of entry.timers) {
      try {
        clearTimeout(timerId);
        clearInterval(timerId);
      } catch (e) {
        RNHLogger.error(TAG, `Failed to clear timer ${timerId}: ${e}`);
      }
    }
    entry.timers.clear();

    // 3. Clear listener references
    entry.listeners.clear();

    // 4. Clear weak references
    entry.weakRefs.clear();

    // 5. Remove from tracking
    this.surfaces.delete(surfaceId);

    const duration = Date.now() - entry.createdAt;
    RNHLogger.info(TAG, `Surface ${surfaceId} cleaned up (lived ${duration}ms)`);
  }

  /**
   * Clean up all surfaces and global resources
   */
  cleanupAll(): void {
    RNHLogger.info(TAG, `Cleaning up all ${this.surfaces.size} surfaces...`);

    const surfaceIds: number[] = [];
    for (const id of this.surfaces.keys()) {
      surfaceIds.push(id);
    }

    for (const id of surfaceIds) {
      this.cleanupSurface(id);
    }

    // Clear global timers
    for (const timerId of this.globalTimers) {
      try {
        clearTimeout(timerId);
        clearInterval(timerId);
      } catch (e) {
        RNHLogger.error(TAG, `Failed to clear global timer ${timerId}: ${e}`);
      }
    }
    this.globalTimers.clear();

    // Clear global listeners
    this.globalListeners.clear();

    RNHLogger.info(TAG, 'All surfaces and global resources cleaned up');
  }

  /**
   * Get the number of registered surfaces
   */
  getSurfaceCount(): number {
    return this.surfaces.size;
  }

  /**
   * Get memory statistics for diagnostics
   */
  getStats(): MemoryGuardStats {
    let totalTimers = this.globalTimers.size;
    let totalListeners = this.globalListeners.size;
    let totalCleanupFns = 0;
    let totalWeakRefs = 0;

    for (const entry of this.surfaces.values()) {
      totalTimers += entry.timers.size;
      totalListeners += entry.listeners.size;
      totalCleanupFns += entry.cleanupFns.size;
      totalWeakRefs += entry.weakRefs.size;
    }

    const stats: MemoryGuardStats = {
      surfaces: this.surfaces.size,
      timers: totalTimers,
      listeners: totalListeners,
      cleanupFns: totalCleanupFns,
      weakRefs: totalWeakRefs,
    };
    return stats;
  }

  /**
   * Check if a surface is registered
   */
  hasSurface(surfaceId: number): boolean {
    return this.surfaces.has(surfaceId);
  }
}
