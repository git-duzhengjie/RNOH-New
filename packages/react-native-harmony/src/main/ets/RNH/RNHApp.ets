/**
 * RNOH-New: RNHApp - Main application component
 *
 * This is the top-level ArkUI component that hosts a React Native app.
 * It manages the RNH instance lifecycle and surfaces.
 */

import { RNHInstance } from './RNHInstance';
import { RNHLogger } from './RNHLogger';

const TAG = 'RNHApp';

/**
 * Configuration for RNHApp
 */
export interface RNHAppConfig {
  // App module name (must match AppRegistry.registerComponent)
  moduleName: string;

  // Initial props passed to the root component
  initialProps?: Record<string, Object>;

  // Bundle path (for production)
  bundlePath?: string;

  // Metro server URL (for development)
  metroUrl?: string;

  // Feature flags
  enableHermes?: boolean;
  enableFabric?: boolean;
  enableTurboModules?: boolean;

  // Debug
  enableInspector?: boolean;
  inspectorPort?: number;
}

/**
 * Main React Native application component.
 * Use this as the root of your ArkUI page.
 */
@Component
export struct RNHApp {
  @State private isInitialized: boolean = false;
  @State private error: string | null = null;

  private config: RNHAppConfig = { moduleName: 'App' };
  private instance: RNHInstance | null = null;

  aboutToAppear(): void {
    RNHLogger.info(TAG, `RNHApp aboutToAppear, module: ${this.config.moduleName}`);
    this.initialize();
  }

  aboutToDisappear(): void {
    RNHLogger.info(TAG, 'RNHApp aboutToDisappear');
    this.destroy();
  }

  private initialize(): void {
    try {
      RNHLogger.info(TAG, 'Initializing RNH instance...');
      this.instance = new RNHInstance({
        bundlePath: this.config.bundlePath ?? '',
        bundleUrl: this.config.metroUrl ?? '',
        enableHermes: this.config.enableHermes ?? true,
        enableFabric: this.config.enableFabric ?? true,
        enableTurboModules: this.config.enableTurboModules ?? true,
        enableBridgeless: true,
        enableInspector: this.config.enableInspector ?? false,
        enableRemoteDebugging: false,
        inspectorPort: this.config.inspectorPort ?? 9229,
        viewportWidth: 0,
        viewportHeight: 0,
        pixelRatio: 1.0,
      });

      this.isInitialized = true;
      RNHLogger.info(TAG, 'RNH instance initialized successfully');
    } catch (e) {
      const errMsg = `Failed to initialize RNH: ${e}`;
      RNHLogger.error(TAG, errMsg);
      this.error = errMsg;
    }
  }

  private destroy(): void {
    if (this.instance) {
      RNHLogger.info(TAG, 'Destroying RNH instance...');
      this.instance.destroy();
      this.instance = null;
      this.isInitialized = false;
    }
  }

  build() {
    if (this.error) {
      // Error state
      Column() {
        Text(`RNOH Error: ${this.error}`)
          .fontSize(16)
          .fontColor(Color.Red)
          .padding(20)
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)
    } else if (this.isInitialized) {
      // RN Surface placeholder
      // TODO: Replace with actual RNHSurface component
      Column() {
        Text(`RNH Surface: ${this.config.moduleName}`)
          .fontSize(16)
          .fontColor(Color.Black)
      }
      .width('100%')
      .height('100%')
    } else {
      // Loading state
      Column() {
        LoadingProgress()
          .width(48)
          .height(48)
        Text('Loading...')
          .fontSize(14)
          .margin({ top: 16 })
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)
    }
  }
}
