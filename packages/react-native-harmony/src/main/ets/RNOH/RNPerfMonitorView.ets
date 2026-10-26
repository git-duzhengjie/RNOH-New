/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { InternalRNOHCoreContext, RNOHCoreContext } from './RNOHContext';
import { SafeAreaInsets } from './SafeAreaInsetsProvider';
import { PerfMonitorController, DEFAULT_PERFORMANCE_METRICS} from './PerfMonitorController';
import { RNInstance } from './RNInstance';
import type { RNInstancePerformanceMetricsSnapshot } from './NapiBridge.ts';

const METRICS_POLLING_INTERVAL_MS = 500;

const PADDING = 4;
const MARGIN = 8;
const FONT_SIZE = 12;
const FONT_COLOR = Color.White;

@Component
export struct RNPerfMonitorView {
  @Require public ctx: RNOHCoreContext;
  @Require public rnInstance: RNInstance;
  @State private visible: boolean = false;
  @State private showDetails: boolean = false;
  @State private jsFps: number = 0;
  @State private uiFps: number = 0;
  @State private performanceMetrics: RNInstancePerformanceMetricsSnapshot = DEFAULT_PERFORMANCE_METRICS;
  @State private appPss: number = 0;
  @State private componentsCount: number = 0;
  @State private jsRuntimeHeapUsage: number = 0;
  @State private safeAreaInsets: SafeAreaInsets | undefined = undefined;
  private cleanUpCallbacks: (() => void)[] = [];
  private metricsPollingInterval: number | null = null;
  private perfMonitorController!: PerfMonitorController;


  aboutToAppear() {
    this.perfMonitorController = new PerfMonitorController(this.ctx.logger, this.rnInstance, (this.ctx as InternalRNOHCoreContext).uiFpsMonitor);
    this.setupSafeAreaInsets();
    this.setupPerfMonitorSubscriptions();
  }

  private setupSafeAreaInsets() {
    if (this.ctx!.safeAreaInsetsProvider !== undefined) {
      this.safeAreaInsets = this.ctx!.safeAreaInsetsProvider.safeAreaInsets;
      this.cleanUpCallbacks.push(
        this.ctx!.safeAreaInsetsProvider.eventEmitter.subscribe(
          'SAFE_AREA_INSETS_CHANGE',
          (insets) => {
            this.safeAreaInsets = insets;
          }));
    } else {
      this.safeAreaInsets = { top: 0, bottom: 0, left: 0, right: 0 }
    }
  }

  private setupPerfMonitorSubscriptions() {
    const emitter = this.ctx!.devToolsController.eventEmitter;

    this.cleanUpCallbacks.push(emitter.subscribe('TOGGLE_PERF_MONITOR', () => {
      this.handleVisibilityToggle();
    }));
  }

  aboutToDisappear() {
    this.stopMetricsPolling();
    this.perfMonitorController.stop();
    this.cleanUpCallbacks.forEach(cb => cb());
  }

  private startMetricsPolling() {
    if (this.metricsPollingInterval !== null) {
      return; //  Already polling, prevent duplicate intervals
    }

    this.metricsPollingInterval = setInterval(() => {
      const metrics = this.perfMonitorController.getFpsMetrics();
      this.uiFps = Math.round(metrics.uiFps);
      this.jsFps = Math.round(metrics.jsFps);
      this.appPss = Math.round(this.perfMonitorController.getAppPss() / 1000); //  convert to MB
      this.componentsCount = this.perfMonitorController.getComponentsCount();
      this.jsRuntimeHeapUsage = this.perfMonitorController.getJSRuntimeHeapUsage() / 1000000; //  convert to MB
    }, METRICS_POLLING_INTERVAL_MS);
  }

  private stopMetricsPolling(): void {
    if (this.metricsPollingInterval !== null) {
      clearInterval(this.metricsPollingInterval);
      this.metricsPollingInterval = null;
    }
  }

  private handleVisibilityToggle(): void {
    this.visible = !this.visible;
    
    if (this.visible) {
      this.perfMonitorController.start();
      this.startMetricsPolling();
      this.performanceMetrics = this.perfMonitorController.getPerformanceMetrics();
    } else {
      this.stopMetricsPolling();
      this.perfMonitorController.stop();
    }
  }


  build() {
    if (this.safeAreaInsets && this.visible) {
      Column() {
        // Always visible FPS metrics
        Row() {
          PrimaryMetric({ name: 'JS', value: this.jsFps.toString() });
          PrimaryMetric({ name: 'UI', value: this.uiFps.toString() });
          PrimaryMetric({
            name: 'JS VM',
            value: this.jsRuntimeHeapUsage.toFixed(2),
            suffix: 'MB',
            minWidth: 60
          });
          PrimaryMetric({ name: 'PSS', value: this.appPss.toString(), suffix: 'MB' });
          PrimaryMetric({ name: 'Views', value: this.componentsCount.toString() });
        }

        if (this.showDetails) {
          // Toggled onClick
          Column() {
            Divider()
              .color(FONT_COLOR)
              .opacity(0.3)
              .margin(MARGIN)
            ForEach(Object.keys(this.performanceMetrics), (name: string) => {
              SecondaryMetric({ name, value: this.performanceMetrics[name] });
            }, (name: string) => name);
          }
          .alignItems(HorizontalAlign.Start)
          .margin({ left: PADDING })
          .constraintSize({ maxWidth: 200 })
        }
      }
      .backgroundColor(Color.Black)
      .opacity(0.8)
      .borderRadius(8)
      .borderColor(FONT_COLOR)
      .borderWidth(1)
      .padding(PADDING)
      .hitTestBehavior(HitTestMode.Transparent)
      .position({
        right: MARGIN,
        top: (this.safeAreaInsets?.top ?? 0) + MARGIN
      })
      .zIndex(1000)
      .onClick(() => {
        this.showDetails = !this.showDetails;
      });
    }
  }
}

@Component
struct PrimaryMetric {
  @Prop name: string;
  @Prop value: string;
  @Prop minWidth: number = 32;
  @Prop suffix: string = '';


  build() {
    Column() {
      Text(this.name)
        .fontColor(FONT_COLOR)
        .fontWeight(FontWeight.Bold)
        .fontSize(FONT_SIZE)

      Text(`${this.value} ${this.suffix}`)
        .fontColor(FONT_COLOR)
        .fontSize(FONT_SIZE)
    }.padding(PADDING).constraintSize(({minWidth: this.minWidth}));
  }
}

@Component
struct SecondaryMetric {
  @Prop name: string;
  @Prop value: number;

  private formatValue(name: string, value?: number): string {
    if(value === undefined) {
      return 'N/A';
    }
    let suffix = 'ms';
    if(name.toLowerCase().includes('size')){
      suffix = 'B';
    }else if (name.toLowerCase().includes('count')){
      suffix = '';
    }
    const formatted = Number.isInteger(value) ? value : value.toFixed(2);
    return `${formatted} ${suffix}`;
  }

  // EXAMPLE_VALUE -> ExampleValue
  private formatName(name: string): string {
    return name.split('_')
      .map(word => word[0].toUpperCase() +
      word.slice(1).toLowerCase())
      .join('')
  }

  build() {
    Text(`${this.formatName(this.name)}: ${this.formatValue(this.name, this.value)}`)
      .fontColor(FONT_COLOR)
      .fontSize(FONT_SIZE)
  }
}