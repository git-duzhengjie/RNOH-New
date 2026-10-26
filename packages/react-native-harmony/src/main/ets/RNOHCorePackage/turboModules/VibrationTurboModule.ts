/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import type { UITurboModuleContext } from '../../RNOH/TurboModule';
import { UITurboModule } from '../../RNOH/TurboModule';
import { RNOHLogger } from '../../RNOH/RNOHLogger';
import { VibrationController } from '../../RNOH/VibrationController';

export class VibrationTurboModule extends UITurboModule {
  public static readonly NAME = 'Vibration';

  private logger: RNOHLogger;
  private vibrationController: VibrationController;

  constructor(protected ctx: UITurboModuleContext) {
    super(ctx);
    this.logger = this.ctx.logger.clone('VibrationTurboModule');
    this.vibrationController = new VibrationController(this.logger);
  }

  public vibrate(pattern: number) {
    this.vibrationController.vibrate(pattern);
  };

  public vibrateByPattern(pattern: Array<number>, repeat: number) {
    this.vibrationController.vibrateByPattern(pattern, repeat);
  };

  public cancel() {
    this.vibrationController.cancel();
  }

  __onDestroy__() {
    this.vibrationController.onDestroy();
  }
}