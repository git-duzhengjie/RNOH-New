/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import window from '@ohos.window';
import { DevToolsController } from './DevToolsController';
import { RNOHLogger } from './RNOHLogger';
import { ShakeDetector } from './ShakeDetector';
import { RNOHError } from './RNOHError';
import type common from '@ohos.app.ability.common';


interface AlertDialogButtonOptions {
  value: string,
  action: () => void,
}

enum DialogButtonDirection {
  AUTO, HORIZONTAL, VERTICAL,
}

/**
 * @actor RNOH_APP
 *
 * The DevMenu interface provides a way to interact with the developer menu in an RN application.
 * It allows adding custom menu items and displaying the menu.
 */
export interface DevMenu {
  /**
   * Adds a new custom item to the developer menu.
   * @param title - The display text of the menu item.
   */
  addMenuItem(title: string): void;

  /**
   * Displays the developer menu to the user.
   */
  show(): void;
}

/**
 * @internal
 */
export interface SettingDialog {
  visible: boolean;
  createDevSettingDialog(uiAbilityContext: common.UIAbilityContext, logger: RNOHLogger): void
  open(onClose?: () => void): void
  close(): void
}

/**
 * @internal
 */
export class InternalDevMenu implements DevMenu {
  private devMenuDialogVisible: boolean = false;
  private devMenuButtons: AlertDialogButtonOptions[] = [];
  private logger: RNOHLogger;
  protected shakeDetector: ShakeDetector | null = null;

  /**
   * @internal
   */
  constructor(
    private devToolsController: DevToolsController,
    private settingDialog: SettingDialog,
    private uiAbilityContext: common.UIAbilityContext,
    logger: RNOHLogger
  ) {
    this.logger = logger.clone('DevMenu');
    const localLogger = this.logger.clone('constructor');
    this.createDevMenuDefaultButtons();
    try {
      this.shakeDetector = ShakeDetector.from(this.show.bind(this));
    } catch (err) {
      if (err instanceof RNOHError) {
        localLogger.error(new RNOHError({
          whatHappened: 'Shake to open Dev Menu is disabled. ' + err.getMessage(),
          howCanItBeFixed: err.getSuggestions()
        }));
      } else {
        throw err;
      }
    }
  }

  addMenuItem(title: string): void {
    this.devMenuButtons.push({
      value: title,
      action: () => {
        this.devToolsController.emitDidPressMenuItem(title);
        this.hideDevMenuDialog();
      }
    });
  }

  private createDevMenuDefaultButtons() {
    this.devMenuButtons.push({
      value: 'Reload',
      action: () => {
        this.devToolsController.reload(undefined);
        this.hideDevMenuDialog();
      },
    });
    this.devMenuButtons.push({
      value: 'Toggle Element Inspector',
      action: () => {
        this.devToolsController.toggleElementInspector();
        this.hideDevMenuDialog();
      },
    });
    this.devMenuButtons.push({
      value: 'Open DevTools',
      action: () => {
        this.devToolsController.openDebugger();
        this.hideDevMenuDialog();
      }
    });
    this.devMenuButtons.push({
      value: 'Toggle Perf Monitor',
      action: () => {
        this.devToolsController.togglePerfMonitor();
        this.hideDevMenuDialog();
      }
    });
    this.devMenuButtons.push({
      value: 'Settings',
      action: () => {
        this.settingDialog.open(() => {
          this.show();
        });
        this.hideDevMenuDialog();
      }
    });
  }

  public show() {
    if (!this.devMenuDialogVisible && !this.settingDialog.visible) {
      this.logger.debug('DevMenu opened');
      this.showDevMenuDialog();
    }
  }

  private showDevMenuDialog() {
    window.getLastWindow(this.uiAbilityContext).then((value) => {
      {
        const uiContext = value.getUIContext();
        const dialogParams = {
          title: 'React Native Dev Menu',
          message: '',
          buttons: this.devMenuButtons,
          buttonDirection: DialogButtonDirection.VERTICAL,
          cancel: this.hideDevMenuDialog.bind(this),
        };
        uiContext.showAlertDialog(dialogParams);
        this.devMenuDialogVisible = true;
        this.devToolsController.emitDevMenuShown();
        this.settingDialog.createDevSettingDialog(this.uiAbilityContext, this.logger);
      }
    }).catch(() => {
      this.logger.error("DevMenu dialog couldn't be displayed.");
      this.hideDevMenuDialog();
    });
  }

  private hideDevMenuDialog() {
    this.devMenuDialogVisible = false;
    this.shakeDetector?.resetDetector();
  }

  onDestroy() {
    this.shakeDetector?.onDestroy();
  }
}

/**
 * @internal
 */
export class NoopDevMenu implements DevMenu {
  addMenuItem(title: string): void {
  }

  show(): void {
  }
}