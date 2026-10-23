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
 * Alert button configuration
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
 * Prompt callback type
 */
type PromptCallback = (result: string) => void;

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

    // Map RN alert buttons to AlertDialog buttons
    // AlertDialog supports primary/secondary buttons and optionally cancel
    const primaryButton = buttons[0];
    const secondaryButton = buttons[1];

    const dialogOptions: promptAction.AlertDialogParam = {
      title: title,
      message: message,
      primaryButton: primaryButton ? {
        value: primaryButton.text,
        action: () => {
          this.handleButtonClick(primaryButton.id);
        }
      } : undefined,
      secondaryButton: secondaryButton ? {
        value: secondaryButton.text,
        action: () => {
          this.handleButtonClick(secondaryButton.id);
        }
      } : undefined,
      onWillDismiss: (dismissInfo: promptAction.DismissDialogAction) => {
        if (dismissInfo.dismissAction === promptAction.DismissAction.PRESS_BACK) {
          // Treat back button as cancel
          const cancelButton = buttons.find((b) => b.style === 'cancel');
          if (cancelButton) {
            this.handleButtonClick(cancelButton.id);
          }
        }
      }
    };

    promptAction.showAlert(dialogOptions);
  }

  /**
   * Show a native prompt dialog
   */
  prompt(title: string, message: string, callback: PromptCallback): void {
    RNHLogger.info(TAG, `Showing prompt: ${title}`);
    promptAction.showDialog({
      title: title,
      message: message,
      buttons: [
        {
          text: 'OK',
          color: '#317aff',
        }
      ]
    }).then((data: promptAction.ShowDialogSuccessResponse) => {
      callback(data.index.toString());
    }).catch((err: Error) => {
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

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'AlertTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'AlertTurboModule invalidated');
    super.invalidate();
  }
}
