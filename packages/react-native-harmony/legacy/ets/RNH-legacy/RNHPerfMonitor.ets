/**
 * RNOH-New: 性能监控器（ArkTS 侧）
 *
 * 提供 JS FPS 监控、内存监控、与 C++ 侧数据汇总的统一接口。
 * 支持事件订阅：on('fps', callback)、on('memory', callback)。
 *
 * 说明：
 * - C++ 侧数据通过 NAPI 桥接获取（桥接层由 RNHNapiBridge 提供）
 * - 若 C++ 侧数据不可用，则仅返回 JS 侧统计
 */

import { RNHLogger } from './RNHLogger';

/**
 * 性能统计数据结构
 */
export interface PerfStats {
  /** JS 侧 FPS */
  jsFps: number;
  /** 原生侧 FPS（C++ 统计，不可用时为 0） */
  nativeFps: number;
  /** 内存统计 */
  memory: MemoryStats;
  /** 启动时间线（毫秒） */
  startup: StartupStats;
}

/**
 * 内存统计
 */
export interface MemoryStats {
  /** JS 堆使用量（字节） */
  jsHeapUsed: number;
  /** JS 堆总大小（字节） */
  jsHeapTotal: number;
  /** 原生堆已分配（字节） */
  nativeHeapAllocated: number;
  /** 原生堆总大小（字节） */
  nativeHeapSize: number;
  /** 进程 RSS（字节） */
  rss: number;
}

/**
 * 启动时间线
 */
export interface StartupStats {
  /** T0 → TI（JS 引擎初始化） */
  t0ToTI: number;
  /** TI → TTI（首屏可交互） */
  tiToTTI: number;
  /** T0 → TTI（总启动耗时） */
  t0ToTTI: number;
}

/**
 * 事件回调类型
 */
export type PerfCallback<T> = (data: T) => void;

/**
 * Native stats as returned by C++ bridge JSON string
 */
interface NativeStats {
  fps?: number;
  memory?: NativeMemoryStats;
  startup?: NativeStartupStats;
}

/**
 * Native 内存统计
 */
interface NativeMemoryStats {
  allocated?: number;
  heapSize?: number;
  rss?: number;
}

/**
 * Native 启动耗时统计
 */
interface NativeStartupStats {
  t0ToTI?: number;
  tiToTTI?: number;
  t0ToTTI?: number;
}

/**
 * JavaScript performance.memory-like structure
 */
interface JSPerformanceMemory {
  usedJSHeapSize?: number;
  totalJSHeapSize?: number;
}

/**
 * JavaScript performance-like structure
 */
interface JSPerformance {
  memory?: JSPerformanceMemory;
}

/**
 * JavaScript process.memoryUsage-like structure
 */
interface JSProcessMemoryUsage {
  used?: number;
  heapTotal?: number;
}

/**
 * JavaScript process-like structure
 */
interface JSProcess {
  memoryUsage: () => JSProcessMemoryUsage;
}

/**
 * Native bridge object shape
 */
interface NativePerfBridge {
  getStats: () => string;
}

/**
 * 全局对象引用持有者
 *
 * ArkTS 禁止直接把 globalThis 转成 Record 类型，
 * 因此通过一个显式声明为 ESObject 的容器间接访问可选全局能力。
 */
interface GlobalBridgeHolder {
  __RNHPerfMonitor__?: NativePerfBridge;
}

/**
 * 性能监控器（ArkTS 侧）
 *
 * 单例模式，通过 RNHPerfMonitor.getInstance() 获取。
 */
export class RNHPerfMonitor {
  private static instance: RNHPerfMonitor | null = null;

  // FPS 相关
  private jsFps_: number = 0;
  private frameCount_: number = 0;
  private fpsLastTime_: number = 0;
  private fpsTimer_: number | null = null;

  // 内存相关
  private memory_: MemoryStats = {
    jsHeapUsed: 0,
    jsHeapTotal: 0,
    nativeHeapAllocated: 0,
    nativeHeapSize: 0,
    rss: 0,
  };
  private memoryTimer_: number | null = null;

  // 事件订阅
  private fpsListeners_: PerfCallback<number>[] = [];
  private memoryListeners_: PerfCallback<MemoryStats>[] = [];

  // C++ 侧桥接（通过 NAPI 注入的全局对象）
  private nativePerf_: NativePerfBridge | null = null;

  private constructor() {
    // 尝试获取 C++ 侧性能监控桥接
    try {
      // 由 RNHNapiBridge 注入到全局对象
      const holder = globalThis as GlobalBridgeHolder;
      const globalThisBridge = holder.__RNHPerfMonitor__;
      if (globalThisBridge) {
        const bridge = globalThisBridge as NativePerfBridge;
        if (typeof bridge.getStats === 'function') {
          this.nativePerf_ = bridge;
        }
      }
    } catch (e) {
      RNHLogger.warn('RNHPerfMonitor: C++ 侧桥接不可用', e);
    }
  }

  /**
   * 获取单例
   */
  static getInstance(): RNHPerfMonitor {
    if (!RNHPerfMonitor.instance) {
      RNHPerfMonitor.instance = new RNHPerfMonitor();
    }
    return RNHPerfMonitor.instance;
  }

  // ---------- FPS ----------

  /**
   * 启动 JS FPS 监控
   * @param intervalMs 统计间隔（毫秒），默认 1000
   */
  startFpsMonitor(intervalMs: number = 1000): void {
    if (this.fpsTimer_ !== null) {
      return;
    }
    this.frameCount_ = 0;
    this.fpsLastTime_ = Date.now();

    const tick = (): void => {
      this.frameCount_++;
    };

    // ArkTS 环境不提供 requestAnimationFrame，按 16ms 间隔模拟 60fps 采样
    setInterval(tick, 16);

    this.fpsTimer_ = setInterval(() => {
      const now = Date.now();
      const elapsed = (now - this.fpsLastTime_) / 1000;
      if (elapsed > 0) {
        this.jsFps_ = this.frameCount_ / elapsed;
        this.fpsLastTime_ = now;
        this.frameCount_ = 0;

        // 通知订阅者
        for (const listener of this.fpsListeners_) {
          try {
            listener(this.jsFps_);
          } catch (e) {
            RNHLogger.error('RNHPerfMonitor: fps listener error', e);
          }
        }
      }
    }, intervalMs);
  }

  /**
   * 停止 JS FPS 监控
   */
  stopFpsMonitor(): void {
    if (this.fpsTimer_ !== null) {
      clearInterval(this.fpsTimer_);
      this.fpsTimer_ = null;
    }
  }

  /**
   * 获取当前 JS FPS
   */
  getJsFps(): number {
    return this.jsFps_;
  }

  // ---------- 内存 ----------

  /**
   * 启动内存监控
   * @param intervalMs 统计间隔（毫秒），默认 2000
   */
  startMemoryMonitor(intervalMs: number = 2000): void {
    if (this.memoryTimer_ !== null) {
      return;
    }

    this.updateMemoryStats();
    this.memoryTimer_ = setInterval(() => {
      this.updateMemoryStats();

      // 通知订阅者
      for (const listener of this.memoryListeners_) {
        try {
          listener(this.memory_);
        } catch (e) {
          RNHLogger.error('RNHPerfMonitor: memory listener error', e);
        }
      }
    }, intervalMs);
  }

  /**
   * 停止内存监控
   */
  stopMemoryMonitor(): void {
    if (this.memoryTimer_ !== null) {
      clearInterval(this.memoryTimer_);
      this.memoryTimer_ = null;
    }
  }

  /**
   * 获取当前内存统计
   */
  getMemoryStats(): MemoryStats {
    const stats: MemoryStats = {
      jsHeapUsed: this.memory_.jsHeapUsed,
      jsHeapTotal: this.memory_.jsHeapTotal,
      nativeHeapAllocated: this.memory_.nativeHeapAllocated,
      nativeHeapSize: this.memory_.nativeHeapSize,
      rss: this.memory_.rss,
    };
    return stats;
  }

  /**
   * 更新内存统计（从 JS 引擎与 C++ 侧汇总）
   */
  private updateMemoryStats(): void {
    // JS 堆：ArkTS 侧不直接探测 performance / process，固定为 0，由 C++ 侧统计
    let jsHeapUsed = 0;
    let jsHeapTotal = 0;

    // C++ 侧内存
    let nativeHeapAllocated = 0;
    let nativeHeapSize = 0;
    let rss = 0;

    if (this.nativePerf_ && typeof this.nativePerf_.getStats === 'function') {
      try {
        const nativeStats = JSON.parse(this.nativePerf_.getStats()) as NativeStats;
        if (nativeStats.memory) {
          nativeHeapAllocated = nativeStats.memory.allocated ?? 0;
          nativeHeapSize = nativeStats.memory.heapSize ?? 0;
          rss = nativeStats.memory.rss ?? 0;
        }
      } catch (e) {
        RNHLogger.warn('RNHPerfMonitor: 获取原生内存统计失败', e);
      }
    }

    const stats: MemoryStats = {
      jsHeapUsed,
      jsHeapTotal,
      nativeHeapAllocated,
      nativeHeapSize,
      rss,
    };
    this.memory_ = stats;
  }

  // ---------- 启动时间线 ----------

  /**
   * 获取启动时间线统计
   */
  getStartupStats(): StartupStats {
    let t0ToTI = 0;
    let tiToTTI = 0;
    let t0ToTTI = 0;

    if (this.nativePerf_ && typeof this.nativePerf_.getStats === 'function') {
      try {
        const nativeStats = JSON.parse(this.nativePerf_.getStats()) as NativeStats;
        if (nativeStats.startup) {
          t0ToTI = nativeStats.startup.t0ToTI ?? 0;
          tiToTTI = nativeStats.startup.tiToTTI ?? 0;
          t0ToTTI = nativeStats.startup.t0ToTTI ?? 0;
        }
      } catch (e) {
        RNHLogger.warn('RNHPerfMonitor: 获取原生启动统计失败', e);
      }
    }

    const stats: StartupStats = { t0ToTI, tiToTTI, t0ToTTI };
    return stats;
  }

  // ---------- 统一接口 ----------

  /**
   * 获取汇总的性能统计数据
   */
  getPerfStats(): PerfStats {
    let nativeFps = 0;
    if (this.nativePerf_ && typeof this.nativePerf_.getStats === 'function') {
      try {
        const nativeStats = JSON.parse(this.nativePerf_.getStats()) as NativeStats;
        nativeFps = nativeStats.fps ?? 0;
      } catch {
        nativeFps = 0;
      }
    }

    const startupStats = this.getStartupStats();
    const memoryStats = this.getMemoryStats();

    const stats: PerfStats = {
      jsFps: this.jsFps_,
      nativeFps,
      memory: memoryStats,
      startup: startupStats,
    };
    return stats;
  }

  // ---------- 事件订阅 ----------

  /**
   * 订阅 FPS 事件
   * @param event 'fps'
   * @param callback 回调函数，参数为当前 FPS
   */
  on(event: 'fps', callback: PerfCallback<number>): void;

  /**
   * 订阅内存事件
   * @param event 'memory'
   * @param callback 回调函数，参数为当前内存统计
   */
  on(event: 'memory', callback: PerfCallback<MemoryStats>): void;

  on(event: string, callback: PerfCallback<number> | PerfCallback<MemoryStats>): void {
    if (event === 'fps') {
      this.fpsListeners_.push(callback as PerfCallback<number>);
    } else if (event === 'memory') {
      this.memoryListeners_.push(callback as PerfCallback<MemoryStats>);
    }
  }

  /**
   * 取消订阅 FPS 事件
   */
  off(event: 'fps', callback: PerfCallback<number>): void;

  /**
   * 取消订阅内存事件
   */
  off(event: 'memory', callback: PerfCallback<MemoryStats>): void;

  off(event: string, callback: PerfCallback<number> | PerfCallback<MemoryStats>): void {
    if (event === 'fps') {
      this.fpsListeners_ = this.fpsListeners_.filter((l: PerfCallback<number>) => l !== callback);
    } else if (event === 'memory') {
      this.memoryListeners_ = this.memoryListeners_.filter((l: PerfCallback<MemoryStats>) => l !== callback);
    }
  }

  /**
   * 销毁监控器，停止所有定时器
   */
  destroy(): void {
    this.stopFpsMonitor();
    this.stopMemoryMonitor();
    this.fpsListeners_ = [];
    this.memoryListeners_ = [];
  }
}
