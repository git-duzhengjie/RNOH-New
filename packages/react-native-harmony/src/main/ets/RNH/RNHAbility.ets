/**
 * RNOH-New: RNHAbility - Base Ability for RN apps
 *
 * Extend this class to create a HarmonyOS Ability that hosts
 * a React Native application.
 */

import { AbilityConstant, UIAbility, Want } from '@kit.AbilityKit';
import { window } from '@kit.ArkUI';
import { RNHLogger } from './RNHLogger';

const TAG = 'RNHAbility';

/**
 * Base UIAbility for React Native apps on HarmonyOS.
 * Extend this class and override getPagePath() to specify your page.
 */
export class RNHAbility extends UIAbility {
  /**
   * Get the page path for the main RN page.
   * Override this to specify your app's main page.
   */
  getPagePath(): string {
    return 'pages/Index';
  }

  /**
   * Get the RNH worker script URL.
   * Override this if you use a custom worker.
   */
  getRNOHWorkerScriptUrl(): string {
    return 'entry/ets/workers/RNHWorker.ets';
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    RNHLogger.info(TAG, 'RNHAbility onCreate');
    super.onCreate(want, launchParam);
  }

  onDestroy(): void {
    RNHLogger.info(TAG, 'RNHAbility onDestroy');
    super.onDestroy();
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    RNHLogger.info(TAG, 'RNHAbility onWindowStageCreate');
    super.onWindowStageCreate(windowStage);

    windowStage.loadContent(this.getPagePath(), (err) => {
      if (err.code) {
        RNHLogger.error(TAG, `Failed to load content: ${JSON.stringify(err)}`);
        return;
      }
      RNHLogger.info(TAG, 'Content loaded successfully');
    });
  }

  onWindowStageDestroy(): void {
    RNHLogger.info(TAG, 'RNHAbility onWindowStageDestroy');
    super.onWindowStageDestroy();
  }

  onForeground(): void {
    RNHLogger.info(TAG, 'RNHAbility onForeground');
    super.onForeground();
  }

  onBackground(): void {
    RNHLogger.info(TAG, 'RNHAbility onBackground');
    super.onBackground();
  }
}
