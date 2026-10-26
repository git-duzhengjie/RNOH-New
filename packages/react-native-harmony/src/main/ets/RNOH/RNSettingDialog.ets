/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { preferences } from '@kit.ArkData';
import { BusinessError } from '@kit.BasicServicesKit';
import type common from '@ohos.app.ability.common';
import { ComponentContent, PromptAction, window } from '@kit.ArkUI';
import { RNOHLogger } from './RNOHLogger';

class SettingsPageParams {
  public uiAbilityContext: common.UIAbilityContext;
  public offsetY: number;
  public onClose: () => void;

  constructor(uiAbilityContext: common.UIAbilityContext, offsetY: number, onClose: () => void) {
    this.uiAbilityContext = uiAbilityContext;
    this.offsetY = offsetY;
    this.onClose = onClose;
  }
}

const GUTTER = 16;
const PreferencesKey = 'devHostAndPortAddress';

@Component
@CustomDialog
struct DevSetHostAndPortContent {
  @State address: string = '';
  uiAbilityContext: common.UIAbilityContext | null = null;
  settingsPreferences: preferences.Preferences | null = null;
  controller?: CustomDialogController;

  private cancel() {
    this.controller?.close();
  }

  private confirm() {
    this.settingsPreferences?.put(PreferencesKey, this.address).then(() => {
      this.settingsPreferences?.flush();
    }).catch((err: BusinessError) => {
      console.error(`Failed to put value of '${PreferencesKey}'. code = ${err.code}, message = ${err.message}`);
    }).finally(() => {
      this.cancel();
    });
  }

  private reset() {
    this.settingsPreferences?.deleteSync(PreferencesKey);
    this.settingsPreferences?.flush();
    this.address = '';
  }

  aboutToAppear() {
    this.settingsPreferences?.get(PreferencesKey, '', (err: BusinessError, val: preferences.ValueType) => {
      if (!err) {
        this.address = val.toString();
      }
    });
  }

  build() {
    Column() {
      Text('Metro Server Address (LAN)')
        .width('100%')
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .padding({ top: 10, bottom: 20 })
      Row() {
        TextInput({ text: this.address, placeholder: 'IP:Port' })
          .placeholderColor('#888')
          .borderRadius(4)
          .onChange((value: string) => {
            this.address = value;
          });
      }
      Row() {
        Button('Confirm')
          .buttonStyle(ButtonStyleMode.TEXTUAL)
          .onClick(() => {
            this.confirm();
          });
        Divider().vertical(true).height(22).color('#888').margin({ left: 8, right: 8 });
        Button('Reset')
          .buttonStyle(ButtonStyleMode.TEXTUAL)
          .onClick(() => {
            this.reset();
          });
        Divider().vertical(true).height(22).color('#888').margin({ left: 8, right: 8 });
        Button('Cancel')
          .buttonStyle(ButtonStyleMode.TEXTUAL)
          .onClick(() => {
            this.cancel();
          });
      }
      .width('100%')
      .height(64)
      .justifyContent(FlexAlign.SpaceAround)
    }
    .width('94%')
    .backgroundColor(Color.White)
    .borderRadius(4)
    .padding(10)
  }
}

@Component
struct SettingsClickItem {
  title: string = '';
  description: string = '';
  click: () => void = () => {};
  private clickBackgroundColor: ResourceColor = Color.White;
  @Styles normalStyle() {
    .backgroundColor(Color.White)
  }
  @Styles pressedStyle() {
    .backgroundColor('rgba(224,224,224, 0.5)')
  }
  build() {
    Flex({ direction: FlexDirection.Column }) {
      Column() {
        Text(this.title)
          .width('100%')
          .fontWeight(FontWeight.Bold)
          .textAlign(TextAlign.Start)
        Text(this.description)
          .width('100%')
          .textAlign(TextAlign.Start)
          .fontColor('#888')
          .fontSize(12)
          .margin({
            top: GUTTER / 2
          });
      }
      .backgroundColor(this.clickBackgroundColor)
      .padding(GUTTER)
      .borderColor(Color.Gray)
      .stateStyles({
        normal: this.normalStyle,
        pressed: this.pressedStyle,
      })
      .onClick(() => {
        this.click();
      });
      Divider().width('100%').color('#888');
    }
  }
}

@Component
struct SettingsPageDialog {
  uiAbilityContext: common.UIAbilityContext | null = null;
  offsetY: number = 0;
  onClose: () => void = () => {}
  settingsPreferences : preferences.Preferences | null = null;
  setHostAndPortDialogController: CustomDialogController | null = new CustomDialogController({
    builder: DevSetHostAndPortContent({ uiAbilityContext: this.uiAbilityContext, settingsPreferences: this.settingsPreferences }),
    cancel: ()=> {},
    autoCancel: false,
    customStyle: true,
  });

  aboutToAppear() {
    preferences.getPreferences(this.uiAbilityContext, 'devSettings', (err: BusinessError, val: preferences.Preferences) => {
      if (!err) {
        this.settingsPreferences = val;
      }
    });
  }

  aboutToDisappear() {
    this.setHostAndPortDialogController = null;
  }

  build() {
    Column() {
      Column() {
        Stack() {
          Text('Settings')
            .width('100%')
            .textAlign(TextAlign.Center)
            .fontSize(20)
            .fontWeight(FontWeight.Bold)
          Button('Close')
            .buttonStyle(ButtonStyleMode.TEXTUAL)
            .stateEffect(true)
            .fontColor('#888')
            .position({
              right: 0
            })
            .onClick(() => {
              this.onClose();
            });
        }
        SettingsClickItem({
          title: 'Connect to Metro Server on Local Network',
          description: 'If you\'re loading the bundle via \`new MetroJSBundleProvider()\`, you can click here to set the Metro server address within your local network. After that, terminate the app and relaunch it to connect to the server.',
          click:() => {
            if (this.setHostAndPortDialogController !== null) {
              this.setHostAndPortDialogController.open();
            }
          }
        });
      }
    }.backgroundColor(Color.White).width('100%').padding({ top: this.offsetY });
  }
}

@Builder
function SettingsPageDialogBuilder(Params: SettingsPageParams) {
  SettingsPageDialog({ uiAbilityContext: Params.uiAbilityContext, offsetY: Params.offsetY, onClose: Params.onClose });
}

export class RNSettingDialog {
  private contentNode: ComponentContent<Object> | undefined = undefined;
  private promptAction: PromptAction | undefined = undefined;
  private offsetY: number = 0;
  private onClose?: () => void;
  public visible: boolean = false;

  constructor() {}

  public createDevSettingDialog(uiAbilityContext: common.UIAbilityContext, logger: RNOHLogger) {
    const _logger = logger.clone('RNSettingDialog');
    window.getLastWindow(uiAbilityContext).then((value) => {
      const uiContext = value.getUIContext();
      const avoidAreaType = window.AvoidAreaType.TYPE_SYSTEM;
      const avoidArea = value.getWindowAvoidArea(avoidAreaType);
      const avoidAreaHeight = avoidArea.topRect.height;
      this.offsetY = uiContext.px2vp(avoidAreaHeight);
      this.promptAction = uiContext.getPromptAction();
      this.contentNode = new ComponentContent(uiContext, wrapBuilder(SettingsPageDialogBuilder), new SettingsPageParams(uiAbilityContext, this.offsetY, () => {
        this.close();
      }));
    }).catch(() => {
      _logger.error("RNSettingDialog couldn't be displayed.");
    });
  }

  public open(onClose?: () => void) {
    this.promptAction?.openCustomDialog(this.contentNode, {
      isModal: true,
      onWillDismiss: () => {
        this.close();
      },
      autoCancel: false,
      keyboardAvoidMode: KeyboardAvoidMode.NONE,
    });
    this.visible = true;
    this.onClose = onClose;
  }

  public close() {
    this.promptAction?.closeCustomDialog(this.contentNode);
    this.visible = false;
    this.onClose?.();
  }
}
