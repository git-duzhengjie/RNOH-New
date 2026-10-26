/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/**
 * This file location relative to the package root shouldn't change to avoid introducing breaking changes.
 */
import { FatalRNOHError } from './RNOHError';
import { UITurboModuleContext } from './RNOHContext';
import { EtsUITurboModuleContext } from './EtsRNOHContext';
import { UITurboModule } from '.';


/**
 * @thread: MAIN
 */
export class EtsUITurboModule extends UITurboModule {
  protected ctx: EtsUITurboModuleContext;

  constructor(ctx: UITurboModuleContext) {
    super(ctx);
    if (!(ctx instanceof EtsUITurboModuleContext)) {
      throw new FatalRNOHError({
        whatHappened: 'Received context not compatible with EtsUITurboModule',
        howCanItBeFixed: []
      });
    }
    this.ctx = ctx as EtsUITurboModuleContext;
  }
}