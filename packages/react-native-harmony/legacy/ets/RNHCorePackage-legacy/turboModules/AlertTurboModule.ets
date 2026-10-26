/**
 * RNOH-New: AlertTurboModule - Provides AlertManager native module
 *
 * Implements React Native's AlertManager interface using @ohos.promptAction.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import promptAction from '@ohos.promptAction';

const TAG = 'AlertTurboModule';

/**
 * Alert button configuration from JS
 */
interface AlertButton {
  id: string;
  text: string;
  style?: 'default' | 'cancel' | 'destructive';
}

/**
 * Alert options
 */
interface AlertOptions {
  title?: string;
  message?: string;
  buttons?: AlertButton[];
}

/**
 * Dialog button option for promptAction.showDialog.
 * The 'color' property is required by the underlying ArkUI Button type.
 */
interface DialogButtonOption {
  text: string;
  color: string;
}

/**
 * Options passed to promptAction.showDialog.
 * Declared explicitly to satisfy arkts-no-obj-literals-as-types.
 */
interface DialogShowOptions {
  title: string;
  message: string;
  buttons: DialogButtonOption[];
}

/**
 * Dialog result
 */
interface DialogResult {
  index: number;
}

/**
 * AlertManager TurboModule
 */
export class AlertTurboModule extends RNHTurboModule {
  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'AlertManager';
  }

  /**
   * Show a native alert dialog
   */
  alert(options: AlertOptions): void {
    const title = options.title ?? '';
    const message = options.message ?? '';
    const buttons = options.buttons ?? [];

    RNHLogger.info(TAG, `Showing alert: ${title}`);

    // Map RN alert buttons to showDialog buttons
    const dialogButtons: DialogButtonOption[] = [];
    for (const button of buttons) {
      const color = button.style === 'destructive' ? '#ff3b30' : '#317aff';
      const dialogButton: DialogButtonOption = {
        text: button.text,
        color,
      };
      dialogButtons.push(dialogButton);
    }

    // If no buttons are provided, add a default OK button
    if (dialogButtons.length === 0) {
      const okButton: DialogButtonOption = {
        text: 'OK',
        color: '#317aff',
      };
      dialogButtons.push(okButton);
    }

    const dialogOptions: DialogShowOptions = {
      title,
      message,
      buttons: dialogButtons,
    };

    promptAction.showDialog(dialogOptions)
      .then((data: DialogResult) => {
        const index = data.index;
        if (index >= 0 && index < buttons.length) {
          this.handleButtonClick(buttons[index].id);
        } else {
          // Treat dismiss outside or back as cancel
          const cancelButton = this.findCancelButton(buttons);
          if (cancelButton) {
            this.handleButtonClick(cancelButton.id);
          }
        }
      })
      .catch((err: Error) => {
        RNHLogger.error(TAG, `Alert failed: ${err}`);
      });
  }

  /**
   * Show a native prompt dialog
   */
  prompt(title: string, message: string, callback: (result: string) => void): void {
    RNHLogger.info(TAG, `Showing prompt: ${title}`);
    const okButton: DialogButtonOption = {
      text: 'OK',
      color: '#317aff',
    };
    const dialogOptions: DialogShowOptions = {
      title,
      message,
      buttons: [okButton],
    };
    promptAction.showDialog(dialogOptions)
      .then((data: DialogResult) => {
        callback(data.index.toString());
      })
      .catch((err: Error) => {
        RNHLogger.error(TAG, `Prompt failed: ${err}`);
        callback('-1');
      });
  }

  /**
   * Handle button click by emitting event to JS
   */
  private handleButtonClick(buttonId: string): void {
    RNHLogger.info(TAG, `Alert button clicked: ${buttonId}`);
    this.emitEvent('onAlertAction', JSON.stringify({ id: buttonId }));
  }

  /**
   * Find the cancel-style button in the list
   */
  private findCancelButton(buttons: AlertButton[]): AlertButton | null {
    for (const button of buttons) {
      if (button.style === 'cancel') {
        return button;
      }
    }
    return null;
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'AlertTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'AlertTurboModule invalidated');
    super.invalidate();
  }
}
