/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import ConfigurationConstant from '@ohos.app.ability.ConfigurationConstant';
import type EnvironmentCallback from '@ohos.app.ability.EnvironmentCallback';
import { EtsWorkerTurboModuleContext } from '../../RNOH';
import { AnyThreadTurboModuleContext } from '../../RNOH/RNOHContext';
import { AnyThreadTurboModule, RNOHLogger, UITurboModuleContext } from '../../RNOH/ts';
import { UITaskRunnable } from '../../RNOH/UITaskRunner';

@Sendable
class ChangeColorSchemeRunnableParams {
  public colorMode: number;

  constructor(colorMode: ConfigurationConstant.ColorMode) {
    this.colorMode = colorMode;
  }
}

@Sendable
class ChangeColorSchemeRunnable implements UITaskRunnable<ChangeColorSchemeRunnableParams> {
  run(uiTurboModuleContext: UITurboModuleContext, params: ChangeColorSchemeRunnableParams): void {
    uiTurboModuleContext.uiAbilityContext.getApplicationContext().setColorMode(params.colorMode);
  }
}

type JSColorScheme = 'light' | 'dark' | 'unspecified';

export class AppearanceTurboModule extends AnyThreadTurboModule {
  public static readonly NAME = 'Appearance';
  private lastColorScheme: JSColorScheme = 'unspecified';
  private logger: RNOHLogger;
  private envCallbackID: number | undefined = undefined;

  constructor(ctx: AnyThreadTurboModuleContext) {
    super(ctx);
    this.logger = ctx.logger.clone('AppearanceTurboModule');
    this.logger.info('constructor');
    this.setup();
  }

  private colorModeToJSColorScheme(colorMode: ConfigurationConstant.ColorMode): JSColorScheme {
    if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT) {
      return 'light';
    } else if (colorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK) {
      return 'dark';
    } else {
      return 'unspecified';
    }
  }

  getColorScheme(): JSColorScheme {
    return this.lastColorScheme;
  }

  setup(): void {
    const initialColorMode =
      this.ctx.uiAbilityContext.config.colorMode ?? ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET;
    this.lastColorScheme = this.colorModeToJSColorScheme(initialColorMode);

    const applicationContext = this.ctx.uiAbilityContext.getApplicationContext();

    let envCallback: EnvironmentCallback = {
      onConfigurationUpdated: (config) => {
        const colorMode = config.colorMode ?? ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET;
        this.lastColorScheme = this.colorModeToJSColorScheme(colorMode);

        this.ctx.rnInstance.emitDeviceEvent('appearanceChanged', {
          colorScheme: this.lastColorScheme
        });
      },

      onMemoryLevel: () => {
        //we need this empty callback, otherwise it won't compile
      }
    };

    this.envCallbackID = applicationContext.on('environment', envCallback);
  }

  setColorScheme(colorScheme: string): void {
    let colorMode: ConfigurationConstant.ColorMode;
    if (colorScheme === 'light') {
      colorMode = ConfigurationConstant.ColorMode.COLOR_MODE_LIGHT;
    } else if (colorScheme === 'dark') {
      colorMode = ConfigurationConstant.ColorMode.COLOR_MODE_DARK;
    } else {
      // null / undefined / 'unspecified' → clear app override and follow system
      colorMode = ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET;
    }
    if (this.ctx instanceof EtsWorkerTurboModuleContext) {
      this.ctx.runOnUIThread(new ChangeColorSchemeRunnable(), new ChangeColorSchemeRunnableParams(colorMode));
    } else if (this.ctx instanceof UITurboModuleContext) {
      this.ctx.uiAbilityContext.getApplicationContext().setColorMode(colorMode);
    }
  };

  public addListener(eventName: string) {
    // NOOP
  }

  public removeListeners(count: number) {
    // NOOP
  }

  public __onDestroy__(): void {
    super.__onDestroy__();
    if (this.envCallbackID !== undefined) {
      const applicationContext = this.ctx.uiAbilityContext.getApplicationContext();
      applicationContext.off('environment', this.envCallbackID);
      this.envCallbackID = undefined;
    }
  }
}
