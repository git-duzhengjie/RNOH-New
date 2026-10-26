/**
 * RNOH-New: TimingTurboModule - JS timer native module
 *
 * Implements React Native's Timing native module using setInterval/clearInterval.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';

const TAG = 'TimingTurboModule';

/**
 * Active timer entry
 */
interface TimerEntry {
  timerId: number;
  intervalId: number;
  duration: number;
  repeats: boolean;
  startTime: number;
}

/**
 * Timing TurboModule
 */
export class TimingTurboModule extends RNHTurboModule {
  private timers: Map<number, TimerEntry> = new Map();
  private sendIdleEvents: boolean = false;

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'Timing';
  }

  /**
   * Create and start a timer
   */
  createTimer(timerId: number, duration: number,
              jsSchedulingTime: number, repeats: boolean): void {
    RNHLogger.debug(TAG, `createTimer: ${timerId}, duration=${duration}, repeats=${repeats}`);

    if (this.timers.has(timerId)) {
      this.deleteTimer(timerId);
    }

    // RN schedules timers relative to JS scheduling time. Convert to delay.
    const now = Date.now();
    const schedulingDelay = Math.max(0, jsSchedulingTime - now);

    if (repeats) {
      const intervalId = setInterval(() => {
        this.emitTimerEvent(timerId);
      }, duration);

      // If there's an initial scheduling delay, offset the first fire.
      if (schedulingDelay > 0) {
        setTimeout(() => {
          this.emitTimerEvent(timerId);
        }, schedulingDelay);
      }

      this.timers.set(timerId, {
        timerId,
        intervalId,
        duration,
        repeats,
        startTime: now,
      });
    } else {
      const timeoutId = setTimeout(() => {
        this.emitTimerEvent(timerId);
        this.timers.delete(timerId);
      }, Math.max(duration, schedulingDelay));

      this.timers.set(timerId, {
        timerId,
        intervalId: timeoutId,
        duration,
        repeats,
        startTime: now,
      });
    }
  }

  /**
   * Delete a timer
   */
  deleteTimer(timerId: number): void {
    RNHLogger.debug(TAG, `deleteTimer: ${timerId}`);
    const entry = this.timers.get(timerId);
    if (entry) {
      if (entry.repeats) {
        clearInterval(entry.intervalId);
      } else {
        clearTimeout(entry.intervalId);
      }
      this.timers.delete(timerId);
    }
  }

  /**
   * Set whether to send idle events
   */
  setSendIdleEvents(sendIdleEvents: boolean): void {
    RNHLogger.debug(TAG, `setSendIdleEvents: ${sendIdleEvents}`);
    this.sendIdleEvents = sendIdleEvents;
  }

  /**
   * Emit timer event to JS
   */
  private emitTimerEvent(timerId: number): void {
    this.emitEvent('timer', JSON.stringify({ timerId }));

    if (this.sendIdleEvents) {
      this.emitEvent('idle', JSON.stringify({ timerId, timeRemaining: 0 }));
    }
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'TimingTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'TimingTurboModule invalidated');
    for (const entry of this.timers.values()) {
      if (entry.repeats) {
        clearInterval(entry.intervalId);
      } else {
        clearTimeout(entry.intervalId);
      }
    }
    this.timers.clear();
    super.invalidate();
  }
}
