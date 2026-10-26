/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { TurboModuleContext } from '../../RNOH/TurboModule';
import { TurboModule } from '../../RNOH/TurboModule';
import { DisplayMetrics } from '../../RNOH/types';
import { display } from '@kit.ArkUI';

export class DeviceInfoTurboModule extends TurboModule {
  public static readonly NAME = 'DeviceInfo';

  static async create(ctx: TurboModuleContext) {
    const initialDisplayMetrics = ctx.getDisplayMetrics();
    return new DeviceInfoTurboModule(ctx, initialDisplayMetrics);
  }

  private displayMetrics?: DisplayMetrics = null;
  private previousOrientation?: display.Orientation = null;
  private cleanUpCallbacks?: (() => void)[] = [];
  private readonly windowSizeChangeDebounceTimeInMs: number = 500;

  constructor(protected ctx: TurboModuleContext, initialDisplayMetrics: DisplayMetrics) {
    super(ctx);
    this.displayMetrics = initialDisplayMetrics;
    const displayChangeCallback = (id) => {
      const displayInstance = this.ctx._rnohCoreContextDeps.rnDisplayProvider();
      if (id === displayInstance.id && this.previousOrientation !== displayInstance?.orientation) {
        this.emitOrientationChanged(displayInstance?.orientation);
        this.previousOrientation = displayInstance?.orientation;
      }
    };
    display.on('change', displayChangeCallback);
    this.cleanUpCallbacks.push(() => display.off('change', displayChangeCallback));
    const updateDisplayMetrics = () => {
      this.displayMetrics = this.ctx.getDisplayMetrics();
      this.ctx.rnInstance.emitDeviceEvent('didUpdateDimensions', this.displayMetrics);
      this.ctx.rnInstance.postMessageToCpp('CONFIGURATION_UPDATE', this.displayMetrics);
    };
    this.cleanUpCallbacks.push(
      this.ctx.rnInstance.subscribeToLifecycleEvents('CONFIGURATION_UPDATE', updateDisplayMetrics)
    );
    this.cleanUpCallbacks.push(
      this.ctx.rnInstance.subscribeToLifecycleEvents('WINDOW_SIZE_CHANGE', updateDisplayMetrics,
        { debounceInMs: this.windowSizeChangeDebounceTimeInMs })
    );
  }

  __onDestroy__() {
    super.__onDestroy__();
    this.cleanUpCallbacks.forEach(cb => cb());
  }

  getConstants() {
    if (!this.displayMetrics) {
      this.ctx.logger.error('DeviceInfoTurboModule::getConstants: JS Display Metrics not set');
    }
    return {
      Dimensions: {
        windowPhysicalPixels: this.displayMetrics.windowPhysicalPixels,
        screenPhysicalPixels: this.displayMetrics.screenPhysicalPixels,
      }
    };
  }

  private emitOrientationChanged(orientation: display.Orientation) {
    let name: string;
    let rotationDegrees: number;
    let isLandscape: boolean = false;

    switch (orientation) {
      case display.Orientation.PORTRAIT:
        name = 'portrait-primary';
        rotationDegrees = 0.0;
        break;
      case display.Orientation.LANDSCAPE:
        name = 'landscape-primary';
        rotationDegrees = -90.0;
        isLandscape = true;
        break;
      case display.Orientation.PORTRAIT_INVERTED:
        name = 'portrait-secondary';
        rotationDegrees = 180.0;
        break;
      case display.Orientation.LANDSCAPE_INVERTED:
        name = 'landscape-secondary';
        rotationDegrees = 90.0;
        isLandscape = true;
        break;
    }

    const payload = {
      name,
      rotationDegrees,
      isLandscape,
    };

    this.ctx.rnInstance.emitDeviceEvent('namedOrientationDidChange', payload);
  }
}



