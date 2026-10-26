/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { UITurboModuleContext, RNOHLogger } from '../../RNOH/ts';
import { UITurboModule } from '../../RNOH/TurboModule';


export class ActionSheetManagerTurboModule extends UITurboModule {
  public static readonly NAME = 'ActionSheetManager' as const;

  private logger: RNOHLogger;

  constructor(ctx: UITurboModuleContext) {
    super(ctx);
    this.logger = ctx.logger.clone('ActionSheetManager');
  }

  protected getConstants() {
    return {};
  };

  protected showActionSheetWithOptions(
    options: {
      title?: string | undefined,
      message?: string | undefined,
      options: Array<string> | undefined,
      destructiveButtonIndices?: Array<number>,
      cancelButtonIndex?: number | undefined,
      anchor?: number | undefined,
      tintColor?: number | undefined,
      cancelButtonTintColor?: number | undefined,
      disabledButtonTintColor?: number | undefined,
      userInterfaceStyle?: string | undefined,
      disabledButtonIndices?: Array<number>,
    },
    callback: (buttonIndex: number) => void,
  ) {
    this.ctx.getUIContext()?.showActionSheet({
      title: options.title,
      message: options.message,
      sheets: (options.options ?? []).map((optionName, index) => {
        return {
          title: optionName, action: () => {
            callback(index);
          }
        };
      }),
    });
  };

  protected showShareActionSheetWithOptions(
    options: {
      message?: string | undefined,
      url?: string | undefined,
      subject?: string | undefined,
      anchor?: number | undefined,
      tintColor?: number | undefined,
      cancelButtonTintColor?: number | undefined,
      disabledButtonTintColor?: number | undefined,
      excludedActivityTypes?: Array<string> | undefined,
      userInterfaceStyle?: string | undefined,
    },
    failureCallback: (error: {
      domain: string,
      code: string,
      userInfo?: Object | undefined,
      message: string,
    }) => void,
    successCallback: (completed: boolean, activityType: string | undefined) => void,
  ) {
    this.logger.warn('showShareActionSheetWithOptions is not supported');
  };

  protected dismissActionSheet() {
    this.logger.warn('dismissActionSheet is not supported');
  };
}
