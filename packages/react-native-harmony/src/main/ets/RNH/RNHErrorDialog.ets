/**
 * RNOH-New: RNHErrorDialog - Error display dialog
 */

import { RNHLogger } from './RNHLogger';

const TAG = 'RNHErrorDialog';

/**
 * Error dialog component for displaying RN errors
 */
@Component
export struct RNHErrorDialog {
  @State private visible: boolean = false;
  @State private title: string = 'Error';
  @State private message: string = '';
  @State private stack: string = '';

  private onDismiss?: () => void;

  /**
   * Show an error dialog
   */
  show(title: string, message: string, stack?: string): void {
    this.title = title;
    this.message = message;
    this.stack = stack ?? '';
    this.visible = true;
    RNHLogger.error(TAG, `${title}: ${message}`);
  }

  /**
   * Hide the dialog
   */
  hide(): void {
    this.visible = false;
    this.onDismiss?.();
  }

  build() {
    if (this.visible) {
      Column() {
        // Error content
        Column() {
          Text(this.title)
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
            .fontColor(Color.Red)
            .margin({ bottom: 16 })

          Text(this.message)
            .fontSize(16)
            .fontColor(Color.Black)
            .margin({ bottom: 8 })

          if (this.stack) {
            Scroll() {
              Text(this.stack)
                .fontSize(12)
                .fontColor(Color.Gray)
                .fontFamily('monospace')
            }
            .height(200)
            .margin({ top: 16 })
          }
        }
        .width('80%')
        .padding(24)
        .backgroundColor(Color.White)
        .borderRadius(16)

        // Dismiss button
        Button('Dismiss')
          .width(120)
          .height(40)
          .margin({ top: 24 })
          .onClick(() => this.hide())
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)
      .backgroundColor('rgba(0, 0, 0, 0.5)')
    }
  }
}
