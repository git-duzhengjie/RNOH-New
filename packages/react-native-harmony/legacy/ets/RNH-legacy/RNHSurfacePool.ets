/**
 * RNOH-New: RNHSurfacePool - Surface object reuse pool
 *
 * Maintains a pool of reusable surface configuration objects to reduce
 * the overhead of frequent create/destroy cycles.
 *
 * Note: Since RNHSurface is a @Component struct (ArkUI component),
 * this pool manages surface configuration and ID reuse rather than
 * component instances themselves. The pool stores surface metadata
 * that can be re-acquired quickly, avoiding re-registration overhead.
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHSurfacePool';

/**
 * Default maximum pool size
 */
const DEFAULT_MAX_POOL_SIZE: number = 3;

/**
 * Surface pool statistics
 */
export interface SurfacePoolStats {
  poolSize: number;
  maxPoolSize: number;
  active: number;
  inactive: number;
  totalAcquired: number;
  totalReleased: number;
  totalReused: number;
  reuseRate: number;
}

/**
 * Pooled surface metadata
 */
export interface PooledSurface {
  surfaceId: number;
  moduleName: string;
  width: number;
  height: number;
  pixelRatio: number;
  isActive: boolean;
  lastUsedAt: number;
  createdAt: number;
  reuseCount: number;
}

/**
 * Surface configuration for acquire()
 */
export interface SurfaceAcquireConfig {
  moduleName: string;
  width?: number;
  height?: number;
  pixelRatio?: number;
}

/**
 * Surface pool for reusing surface configurations
 */
export class RNHSurfacePool {
  private pool: PooledSurface[] = [];
  private maxPoolSize: number = DEFAULT_MAX_POOL_SIZE;
  private nextSurfaceId: number = 1;
  private totalAcquired: number = 0;
  private totalReleased: number = 0;
  private totalReused: number = 0;

  constructor(maxPoolSize: number = DEFAULT_MAX_POOL_SIZE) {
    this.maxPoolSize = maxPoolSize;
  }

  /**
   * Acquire a surface from the pool or create a new one.
   * Returns the pooled surface metadata.
   */
  acquire(config: SurfaceAcquireConfig): PooledSurface {
    this.totalAcquired++;

    // Try to find a matching inactive surface in the pool
    for (let i = 0; i < this.pool.length; i++) {
      const surface = this.pool[i];
      if (!surface.isActive && surface.moduleName === config.moduleName) {
        surface.isActive = true;
        surface.lastUsedAt = Date.now();
        surface.reuseCount++;
        this.totalReused++;

        // Update dimensions if provided
        if (config.width !== undefined) {
          surface.width = config.width;
        }
        if (config.height !== undefined) {
          surface.height = config.height;
        }
        if (config.pixelRatio !== undefined) {
          surface.pixelRatio = config.pixelRatio;
        }

        RNHLogger.debug(
          TAG,
          `Reusing surface ${surface.surfaceId} (module: ${config.moduleName}, reuse: ${surface.reuseCount})`
        );
        return surface;
      }
    }

    // No matching surface in pool; create a new one
    const surfaceId = this.nextSurfaceId++;
    const newSurface: PooledSurface = {
      surfaceId,
      moduleName: config.moduleName,
      width: config.width ?? 0,
      height: config.height ?? 0,
      pixelRatio: config.pixelRatio ?? 1.0,
      isActive: true,
      lastUsedAt: Date.now(),
      createdAt: Date.now(),
      reuseCount: 0,
    };

    // Add to pool if there's space
    if (this.pool.length < this.maxPoolSize) {
      this.pool.push(newSurface);
    } else {
      // Pool is full; evict the oldest inactive surface, or the oldest active one
      this.evictOldest();
      if (this.pool.length < this.maxPoolSize) {
        this.pool.push(newSurface);
      }
    }

    RNHLogger.debug(
      TAG,
      `Created new surface ${surfaceId} (module: ${config.moduleName}, pool size: ${this.pool.length}/${this.maxPoolSize})`
    );
    return newSurface;
  }

  /**
   * Release a surface back to the pool.
   * The surface is marked inactive and available for reuse.
   */
  release(surfaceId: number): void {
    this.totalReleased++;

    const surface = this.findSurface(surfaceId);
    if (!surface) {
      RNHLogger.warn(TAG, `Cannot release surface ${surfaceId}: not found in pool`);
      return;
    }

    if (!surface.isActive) {
      RNHLogger.warn(TAG, `Surface ${surfaceId} already released`);
      return;
    }

    surface.isActive = false;
    surface.lastUsedAt = Date.now();

    RNHLogger.debug(
      TAG,
      `Surface ${surfaceId} released (module: ${surface.moduleName})`
    );
  }

  /**
   * Find a surface by ID
   */
  findSurface(surfaceId: number): PooledSurface | null {
    for (const surface of this.pool) {
      if (surface.surfaceId === surfaceId) {
        return surface;
      }
    }
    return null;
  }

  /**
   * Evict the oldest inactive surface, or the oldest surface if all are active
   */
  private evictOldest(): void {
    if (this.pool.length === 0) {
      return;
    }

    // First, try to evict the oldest inactive surface
    let oldestIndex = -1;
    let oldestTime = Infinity;

    for (let i = 0; i < this.pool.length; i++) {
      if (!this.pool[i].isActive && this.pool[i].lastUsedAt < oldestTime) {
        oldestIndex = i;
        oldestTime = this.pool[i].lastUsedAt;
      }
    }

    if (oldestIndex >= 0) {
      const evicted = this.pool.splice(oldestIndex, 1)[0];
      RNHLogger.debug(
        TAG,
        `Evicted inactive surface ${evicted.surfaceId} (module: ${evicted.moduleName})`
      );
      return;
    }

    // All surfaces are active; evict the oldest one
    let oldestActiveIndex = 0;
    let oldestActiveTime = this.pool[0].lastUsedAt;

    for (let i = 1; i < this.pool.length; i++) {
      if (this.pool[i].lastUsedAt < oldestActiveTime) {
        oldestActiveIndex = i;
        oldestActiveTime = this.pool[i].lastUsedAt;
      }
    }

    const evicted = this.pool.splice(oldestActiveIndex, 1)[0];
    RNHLogger.warn(
      TAG,
      `Evicted active surface ${evicted.surfaceId} (module: ${evicted.moduleName}) - pool full`
    );
  }

  /**
   * Set the maximum pool size
   */
  setMaxPoolSize(size: number): void {
    this.maxPoolSize = size;

    // If shrinking, evict excess inactive surfaces
    while (this.pool.length > this.maxPoolSize) {
      this.evictOldest();
    }

    RNHLogger.info(TAG, `Max pool size set to ${size}`);
  }

  /**
   * Get pool statistics
   */
  getStats(): SurfacePoolStats {
    let activeCount = 0;
    let inactiveCount = 0;
    for (const surface of this.pool) {
      if (surface.isActive) {
        activeCount++;
      } else {
        inactiveCount++;
      }
    }

    const stats: SurfacePoolStats = {
      poolSize: this.pool.length,
      maxPoolSize: this.maxPoolSize,
      active: activeCount,
      inactive: inactiveCount,
      totalAcquired: this.totalAcquired,
      totalReleased: this.totalReleased,
      totalReused: this.totalReused,
      reuseRate: this.totalAcquired > 0 ? this.totalReused / this.totalAcquired : 0,
    };
    return stats;
  }

  /**
   * Clear the pool and reset all counters
   */
  clear(): void {
    this.pool = [];
    this.nextSurfaceId = 1;
    this.totalAcquired = 0;
    this.totalReleased = 0;
    this.totalReused = 0;
    RNHLogger.info(TAG, 'Surface pool cleared');
  }

  /**
   * Destroy the pool
   */
  destroy(): void {
    const stats = this.getStats();
    this.pool = [];
    RNHLogger.info(
      TAG,
      `Surface pool destroyed (reuse rate: ${(stats.reuseRate * 100).toFixed(1)}%)`
    );
  }
}
