/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { RNOHLogger } from './RNOHLogger';
import { displaySync } from '@kit.ArkGraphics2D';


export interface UIFpsMonitor {
  start: () => void;
  addOnFpsUpdateCallback: (callback: (fps: number) => void) => void;
  removeOnFpsUpdateCallback: (callback: (fps: number) => void) => void;
  stop: () => void;
}

/**
 * UI FPS Monitor using displaySync API
 *
 * Measures UI FPS via HarmonyOS displaySync. Keeps responsibility focused on
 * UI timing only; higher-level coordination is handled elsewhere.
 */
export class InternalUIFpsMonitor implements UIFpsMonitor {
  private logger: RNOHLogger;
  private isRunning: boolean = false;
  private displaySync: displaySync.DisplaySync;
  private frameInfoBuffer: Array<displaySync.IntervalInfo> = [];
  private onFpsUpdateCallbacks: Array<(fps: number) => void> = [];

  constructor(parentLogger: RNOHLogger) {
    this.logger = parentLogger.clone('UIFPSMonitor');
    this.displaySync = displaySync.create();
  }

  public addOnFpsUpdateCallback(callback: (fps: number) => void) {
    this.onFpsUpdateCallbacks.push(callback);
    if (!this.isRunning && this.onFpsUpdateCallbacks.length === 1) {
      this.start();
    }
  }

  public removeOnFpsUpdateCallback(callback: (fps: number) => void) {
    const index = this.onFpsUpdateCallbacks.indexOf(callback);
    if (index !== -1) {
      this.onFpsUpdateCallbacks.splice(index, 1);
      if (this.isRunning && this.onFpsUpdateCallbacks.length === 0) {
        this.stop();
      }
    }
  }

  public start() {
    if (this.isRunning) {
      this.logger.warn('UIFPSMonitor already running');
      return;
    }
    if (this.onFpsUpdateCallbacks.length === 0) {
      this.logger.warn('No callbacks registered, not starting UIFPSMonitor');
      return;
    }
    try {
      this.displaySync.on('frame', this.onFrame);
      this.displaySync.start();
      this.isRunning = true;
      this.logger.info('UIFPSMonitor started successfully');
    } catch (error) {
      this.logger.error(
        'Failed to start UIFPSMonitor: ',
        JSON.stringify(error, null, 2),
      );
    }
  }

  public stop() {
    if (!this.isRunning) {
      return;
    }
    try {
      this.displaySync.off('frame', this.onFrame);
      this.displaySync.stop();
      this.frameInfoBuffer = [];
      this.isRunning = false;
      this.logger.info('UIFPSMonitor stopped successfully');
    } catch (error) {
      this.logger.error(
        'Failed to stop UIFPSMonitor: ',
        JSON.stringify(error, null, 2),
      );
    }
  }

  private onFrame = (frameInfo: displaySync.IntervalInfo) => {
    this.frameInfoBuffer.push(frameInfo);
    // Calculate FPS using the last two frames
    if (this.frameInfoBuffer.length >= 2) {
      const timeDiff =
        this.frameInfoBuffer[1].timestamp - this.frameInfoBuffer[0].timestamp;
      const fps = 1000000000 / Number.parseInt(timeDiff.toString());
      this.onFpsUpdateCallbacks.forEach(callback => {
        try {
          callback(fps);
        } catch (error) {
          this.logger.error(
            'Error in FPS callback: ',
            JSON.stringify(error, null, 2),
          );
        }
      });
      // Keep only the most recent frame for next calculation
      this.frameInfoBuffer.shift();
    }
  };
}

/**
 * Noop implementation of UIFpsMonitor for release builds
 */
export class NoopUIFpsMonitor implements UIFpsMonitor {
  public addOnFpsUpdateCallback() {}
  public removeOnFpsUpdateCallback() {}
  public start() {}
  public stop() {}
}
