/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { InternalDevToolsController } from './DevToolsController';
import { RNOHContext } from './RNOHContext';

/**
 * @actor RNOH_APP
 *
 * A custom dialog component shown when the debugger is paused.
 * It listens to events from native C++ and displays a message dialog with a resume button.
 */
@Component
export struct RNPauseInDebuggerDialog {
  /**
   * The RNOH execution context injected into the component.
   * Provides access to runtime information such as safe area insets, RN instance, and event subscriptions.
   */
  @Require public ctx!: RNOHContext;

  /**
   * The message content displayed inside the pause dialog.
   * This state is updated when "OPEN_DEBUGGER_PAUSED_DIALOG" is received.
   */
  @State private message: string = '';

  /**
   * Internal controller used to open or close the custom dialog.
   */
  private controller!: CustomDialogController;

  /**
   * Reference to the callback unsubscribing from the OPEN_DEBUGGER_PAUSED_DIALOG event.
   */
  private openCallback!: () => void;

  /**
   * Reference to the callback unsubscribing from the CLOSE_DEBUGGER_PAUSED_DIALOG event.
   */
  private closeCallback!: () => void;

  /**
   * Lifecycle method called before the component appears.
   * Sets up the dialog controller and subscribes to C++ debugger events.
   */
  aboutToAppear(): void {
    const insets = this.ctx.safeAreaInsetsProvider.safeAreaInsets;
    this.controller = new CustomDialogController({
      builder: PauseInDebuggerCustomDialog({
        message: this.message,
        onResume: () => {
          this.ctx._rnohCoreContextDeps.napiBridge.resumeDebugger(this.ctx.rnInstance.getId());
        }
      }),
      autoCancel: false,
      alignment: DialogAlignment.Top,
      customStyle: true,
      width: '100%',
      height: '100%',
      offset: { dx: 0, dy: insets ? insets.top : 0 }
    });

    const internalDevToolsController = this.ctx.devToolsController as InternalDevToolsController;
    this.openCallback = internalDevToolsController.inspectorEventEmitter.subscribe(
      'OPEN_DEBUGGER_PAUSED_DIALOG', payload => {
      this.controller.open();
      this.message = payload.message;
    });

    this.closeCallback = internalDevToolsController.inspectorEventEmitter.subscribe(
      'CLOSE_DEBUGGER_PAUSED_DIALOG', () => {
      this.controller.close();
    });
  }

  /**
   * Lifecycle method called when the component disappears.
   * Unsubscribes from all debugger-related events to avoid memory leaks.
   */
  aboutToDisappear() {
    this.openCallback();
    this.closeCallback();
  }

  /**
   * Empty render method since UI is fully managed by the controller.
   */
  build() {}
}

@CustomDialog
struct PauseInDebuggerCustomDialog {
  @Require public message!: string;
  @Require public onResume!: () => void;
  public controller: CustomDialogController;

  build() {
      Stack() {
        Button({type: ButtonType.Normal}) {
          Row() {
            Text(this.message + ' ▶️')
              .fontColor('rgb(94, 94, 94)')
              .fontSize(16)
              .fontWeight(FontWeight.Bold)
              .textAlign(TextAlign.Center)
              .margin(8)
          }
        }
        .padding({ left: 17, right: 11 })
        .backgroundColor(Color.Transparent)
        .borderRadius(12)
        .onClick(() => this.onResume())
      }
      .alignContent(Alignment.Center)
      .borderRadius(12)
      .backgroundColor('rgb(255,255,193)')
      .borderWidth(2)
      .borderColor('rgb(181, 181, 158)')
  }
}







