/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { RNInstance, RNComponentContext, CustomComponentBuilder } from './RNOH';
import { RNSurface, SurfaceConfig2 } from './RNSurface';

/**
 * Dialog component that renders React Native's LogBox UI for displaying logs,
 * warnings and errors. Creates a dedicated RN surface with "LogBox" app key.
 * @actor RNOH_APP
 */
@CustomDialog
export struct LogBoxDialog {
  public ctx!: RNComponentContext;
  public initialProps: Record<string, string> = {}
  /**
   * @deprecated: do not use it (latestRNOHVersion: 0.72.39)
   */
  public controller: CustomDialogController;
  /**
   * @deprecated: do not use it (latestRNOHVersion: 0.72.39)
   */
  public rnInstance!: RNInstance;
  /**
   * @deprecated: do not use it (latestRNOHVersion 0.72.39)
   */
  @BuilderParam public buildCustomComponent!: CustomComponentBuilder;

  build() {
    RNSurface({
      ctx: this.ctx!,
      surfaceConfig: {
        initialProps: this.initialProps ?? {},
        appKey: 'LogBox',
      },
      buildCustomComponent: this.buildCustomComponent,
    });
  }
}
