/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { UIFpsMonitor } from './UIFpsMonitor';
import { RNOHLogger } from './RNOHLogger';
import type { RNInstancePerformanceMetricsSnapshot } from './NapiBridge.ts';
import type { RNInstance } from './RNInstance';
import { RNInstanceImpl } from './RNInstance';
import { hidebug } from '@kit.PerformanceAnalysisKit';

export interface FpsMetrics {
  uiFps: number;
  jsFps: number;
}

export const DEFAULT_PERFORMANCE_METRICS: RNInstancePerformanceMetricsSnapshot = {
  BUNDLE_SIZE: 0,
  SCRIPT_DOWNLOAD: 0,
  SCRIPT_EXECUTION: 0,
  REACT_INSTANCE_INIT: 0,
  TURBO_MODULE_SETUP: 0,
  INIT_REACT_RUNTIME: 0,
  ROOT_VIEW_TTI: 0, 
  TURBO_MODULE_MAIN_THREAD_COUNT: 0,
  APP_STARTUP: 0
}

export class PerfMonitorController {
  private uiFpsMonitor: UIFpsMonitor;
  private rnInstance: RNInstanceImpl;
  private uiFps: number = 0;
  private jsFps: number = 0;
  private logger: RNOHLogger;
  private isStarted: boolean = false;
  private uiFpsCallback: (fps :number) => void;

  constructor(logger: RNOHLogger, rnInstance: RNInstance, uiFpsMonitor: UIFpsMonitor) {
    this.uiFpsMonitor = uiFpsMonitor;
    this.rnInstance = rnInstance as RNInstanceImpl;
    this.logger = logger.clone('PerfMonitorController');
    this.uiFpsCallback = (uiFps: number) => {
      this.uiFps = uiFps;
    };
  }


  public start(): void {
    // Guard against duplicate starts
    if (this.isStarted) {
      return;
    }
    
    this.uiFpsMonitor.addOnFpsUpdateCallback(this.uiFpsCallback);
    this.isStarted = true;

    try {
      this.rnInstance.startJSFpsMonitor((fps: number) => {
        this.jsFps = fps;
      });
    } catch (error) {
      this.logger.error(`Failed to start JS FPS monitor for instance ${this.rnInstance.getId()}: ${JSON.stringify(error, null, 2)}`);
    }
  }

  public stop(): void {
    // Guard against duplicate stops
    if (!this.isStarted) {
      return;
    }

    this.uiFpsMonitor.removeOnFpsUpdateCallback(this.uiFpsCallback);
    this.isStarted = false;

    try {
    this.rnInstance.stopJsFpsMonitor();
    } catch (error) {
      this.logger.error(`Failed to stop JS FPS monitor for instance ${this.rnInstance.getId()}: ${JSON.stringify(error, null, 2)}`);
    }
  }

  public getAppPss(): number{
    try {
      return Number(hidebug.getPss());
    } catch (error) {
      this.logger.error(`Failed to get appPss: ${error}`);
      return 0;
    }
  }

  public getFpsMetrics(): FpsMetrics {
    return { uiFps: this.uiFps, jsFps: this.jsFps }
  }

  public getComponentsCount(): number {
    try {
     return this.rnInstance.getComponentInstancesCount();
    } catch (error) {
      this.logger.error(`Failed to get component instance count: ${error}`);
      return 0;
    }
  }

  public getJSRuntimeHeapUsage(): number {
    try {
      return this.rnInstance.getJSRuntimeHeapUsage();
    } catch (error) {
      this.logger.error(`Failed to get runtime heap usage: ${error}`);
      return 0;
    }
  }

  public getPerformanceMetrics(): RNInstancePerformanceMetricsSnapshot {
    try {
      const snapshot = this.rnInstance.getPerformanceMetricsSnapshot();
      return snapshot ?? DEFAULT_PERFORMANCE_METRICS;
    }catch(error){
      this.logger.error(`Failed to get performance metrics: ${error}`);
      return DEFAULT_PERFORMANCE_METRICS;
    }
  }

}
