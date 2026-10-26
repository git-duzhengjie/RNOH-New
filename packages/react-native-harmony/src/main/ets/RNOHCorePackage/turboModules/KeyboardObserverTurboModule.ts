/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import window from '@ohos.window';
import type { TurboModuleContext } from '../../RNOH/TurboModule';
import { TurboModule } from '../../RNOH/TurboModule';

export class KeyboardObserverTurboModule extends TurboModule {
  public static readonly NAME = 'KeyboardObserver';

  private cleanUpCallbacks: (() => void)[] = [];
  private keyboardHeight: number;

  constructor(protected ctx: TurboModuleContext) {
    super(ctx);
    this.subscribeListeners();
  }

  private createKeyboardEvent({
    screenX,
    startScreenY,
    endScreenY,
    startHeight,
    endHeight,
    width,
  }: {
    screenX: number;
    startScreenY: number;
    endScreenY: number;
    startHeight: number;
    endHeight: number;
    width: number;
  }) {
    /*
    * KeyboardAvoidingView calculates the relativeKeyboardHeight based on the height of the component received
    * from onLayout, which in some cases can be the whole screen. Yoga uses floats for layout, which leads to
    * rounding errors. Passing the below numbers without any rounding (JS numbers are double precision)
    * can lead to relativeKeyboardHeight being calculated as non-zero when the keyboard is hidden,
    * leading to an infinite loop of re-renders and jittering.
    */
    return {
      easing: 'keyboard',
      duration: 0,
      endCoordinates: {
        screenX: Math.fround(px2vp(screenX)),
        screenY: Math.fround(px2vp(endScreenY)),
        height: Math.fround(px2vp(endHeight)),
        width: Math.fround(px2vp(width)),
      },
      startCoordinates: {
        screenX: Math.fround(px2vp(screenX)),
        screenY: Math.fround(px2vp(startScreenY)),
        height: Math.fround(px2vp(startHeight)),
        width: Math.fround(px2vp(width)),
      }
    };
  }

  private async subscribeListeners() {
    const windowInstance = await this.ctx.rnInstance.getRNWindow();
    // using inputMethodEngine.on('keyboardShow') and .on('keyboardHide') would be preferable, but it doesn't work at the time of writing.
    const onWindowKeyboardHeightChange = async (keyboardHeight) => {
      if (keyboardHeight > 0) {
        const windowRect = windowInstance.getWindowProperties().windowRect;
        const keyboardAvoidArea = windowInstance.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD).bottomRect;
        this.keyboardHeight = keyboardAvoidArea.height;
        this.ctx.rnInstance.emitDeviceEvent('keyboardDidShow',
          this.createKeyboardEvent({
            screenX: keyboardAvoidArea.left,
            startScreenY: windowRect.height,
            endScreenY: keyboardAvoidArea.top,
            startHeight: 0,
            endHeight: keyboardHeight,
            width: windowRect.width,
          }));
        this.ctx.rnInstance.postMessageToCpp('KEYBOARD_VISIBLE', { keyboardHeight: px2vp(keyboardAvoidArea.height) });
      } else {
        const windowRect = windowInstance.getWindowProperties().windowRect;
        const keyboardAvoidArea = windowInstance.getWindowAvoidArea(window.AvoidAreaType.TYPE_KEYBOARD).bottomRect;
        this.ctx.rnInstance.emitDeviceEvent('keyboardDidHide',
          this.createKeyboardEvent({
            screenX: keyboardAvoidArea.left,
            startScreenY: windowRect.height - this.keyboardHeight,
            endScreenY: windowRect.height,
            startHeight: this.keyboardHeight,
            endHeight: 0,
            width: windowRect.width,
          }));
        this.ctx.rnInstance.postMessageToCpp('KEYBOARD_HIDDEN', {});
      }
    };

    windowInstance.on('keyboardHeightChange', onWindowKeyboardHeightChange);
    this.cleanUpCallbacks.push(() => {
      windowInstance.off('keyboardHeightChange', onWindowKeyboardHeightChange);
    });
  }

  __onDestroy__() {
    super.__onDestroy__();
    this.cleanUpCallbacks.forEach(cb => cb());
    this.cleanUpCallbacks = [];
  }
}

