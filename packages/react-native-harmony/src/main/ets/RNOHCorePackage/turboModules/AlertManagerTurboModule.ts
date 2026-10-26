/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { TurboModuleContext } from '../../RNOH/TurboModule';
import { TurboModule } from '../../RNOH/TurboModule';
import window from '@ohos.window';

type AlertOptions = {
  title: string;
  message?: string;
  primaryButton?: string;
  secondaryButton?: string;
  tertiaryButton?: string;
  cancelable: boolean;
};

export class AlertManagerTurboModule extends TurboModule {
  public static readonly NAME = 'AlertManager';

  constructor(protected ctx: TurboModuleContext) {
    super(ctx);

  }

  private constants = {
    buttonClicked: 'buttonClicked',
    dismissed: 'dismissed',
    primaryButton: 1,
    secondaryButton: 2,
    tertiaryButton: 3,
  };

  private parseButton(button?: string, buttonKey?: number, onAction?: (action: string, buttonKey?: number) => void) {
    if (button) {
      return ({
        value: button,
        action: () => {
          onAction?.(this.constants.buttonClicked, buttonKey);
          onAction = undefined;
        }
      });
    }
    return undefined;
  }

  getConstants() {
    return this.constants;
  }

  alert(options: AlertOptions, onError: (error: string) => void, onAction: (action: string, buttonKey?: number) => void) {
    this.ctx.rnInstance.getRNWindow().then((value) => {
      {
        const uiContext = value.getUIContext();

        const primaryButton = this.parseButton(options.primaryButton, this.constants.primaryButton, onAction);
        const secondaryButton = this.parseButton(options.secondaryButton, this.constants.secondaryButton, onAction);
        const tertiaryButton = this.parseButton(options.tertiaryButton, this.constants.tertiaryButton, onAction);
        const buttons = [primaryButton, secondaryButton, tertiaryButton];
        
        const alertParams = {
          title: options.title,
          message: options.message,
          autoCancel: options.cancelable,
          buttons: buttons.filter((button) => button !== undefined),
          cancel: () => {
            onAction(this.constants.dismissed);
          },
        };
        uiContext.showAlertDialog(alertParams);
      }
    }).catch(() => onError("Alert dialog couldn't be displayed."));
  }
}