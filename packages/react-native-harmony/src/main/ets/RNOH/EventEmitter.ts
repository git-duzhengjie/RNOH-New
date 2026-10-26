/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

export type EventEmitterOptions = {
  debounceInMs?: number;
};

export class EventEmitter<TArgsByEventType extends Record<string, unknown[]>> {
  private listenersByEventType: Partial<{
    [K in keyof TArgsByEventType]: Set<{ listener: (...args: TArgsByEventType[K]) => void; debounceInMs?: number }>;
  }> = {};
  private lastTriggerTimeByListener: Map<(...args: unknown[]) => void, number> = new Map();
  private timerIdByListener: Map<(...args: unknown[]) => void, ReturnType<typeof setTimeout>> = new Map();

  public subscribe<TEventType extends keyof TArgsByEventType>(
    eventType: TEventType,
    listener: (...args: TArgsByEventType[TEventType]) => void,
    options?: EventEmitterOptions
  ): () => void {
    if (!this.listenersByEventType[eventType]) {
      this.listenersByEventType[eventType] = new Set();
    }
    this.listenersByEventType[eventType]!.add({ listener, debounceInMs: options?.debounceInMs });

    return () => {
      this.listenersByEventType[eventType]!.forEach(l => {
        if (l.listener === listener) {
          this.listenersByEventType[eventType]!.delete(l);
        }
      });
      this.lastTriggerTimeByListener.delete(listener);
      if (this.timerIdByListener.has(listener)) {
        clearTimeout(this.timerIdByListener.get(listener));
        this.timerIdByListener.delete(listener);
      }
    };
  }

  public emit<TEventType extends keyof TArgsByEventType>(
    eventType: TEventType,
    ...args: TArgsByEventType[TEventType]
  ): void {
    this.listenersByEventType[eventType]?.forEach(({ listener, debounceInMs }) => {
      if (debounceInMs === undefined) {
        listener(...args);
        return;
      }

      const currentTime = Date.now();
      if (!this.lastTriggerTimeByListener.get(listener) ||
        currentTime - this.lastTriggerTimeByListener.get(listener)! > debounceInMs) {
        if (this.timerIdByListener.get(listener)) {
          clearTimeout(this.timerIdByListener.get(listener));
          this.timerIdByListener.delete(listener);
        }
        listener(...args);
        this.lastTriggerTimeByListener.set(listener, currentTime);
      } else {
        if (this.timerIdByListener.get(listener)) {
          clearTimeout(this.timerIdByListener.get(listener));
        }
        this.timerIdByListener.set(
          listener,
          setTimeout(() => {
            listener(...args);
            this.lastTriggerTimeByListener.set(listener, Date.now());
            this.timerIdByListener.delete(listener);
          }, debounceInMs)
        );
      }
    });
  }
}
