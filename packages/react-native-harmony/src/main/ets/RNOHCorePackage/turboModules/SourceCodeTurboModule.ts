/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import {AnyThreadTurboModule} from '../../RNOH/TurboModule';
import {AnyThreadTurboModuleContext} from '../../RNOH/RNOHContext';

export class SourceCodeTurboModule extends AnyThreadTurboModule {
  public static readonly NAME = 'SourceCode';

  constructor(protected ctx: AnyThreadTurboModuleContext) {
    super(ctx);
  }

  getConstants(): {scriptURL: string | null} {
    return {scriptURL: this.ctx.rnInstance.getInitialBundleUrl() ?? ''};
  }
}
