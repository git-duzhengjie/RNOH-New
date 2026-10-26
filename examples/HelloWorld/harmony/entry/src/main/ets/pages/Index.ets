/**
 * HelloWorld Index Page
 *
 * 使用官方 RNOH 的 RNApp 组件加载 JS bundle。
 */

import {
  AnyJSBundleProvider,
  ResourceJSBundleProvider,
  RNApp,
  RNOHCoreContext,
} from '@rnoh-new/react-native-harmony';

@Entry
@Component
struct Index {
  @StorageLink('RNOHCoreContext') private rnohCoreContext: RNOHCoreContext | undefined = undefined;
  @State shouldShow: boolean = false;

  aboutToAppear(): void {
    this.shouldShow = true;
  }

  onBackPress(): boolean | undefined {
    this.rnohCoreContext!.dispatchBackPress();
    return true;
  }

  build() {
    Column() {
      if (this.rnohCoreContext && this.shouldShow) {
        RNApp({
          rnInstanceConfig: {
            name: 'main',
            createRNPackages: () => [],
            enableDebugger: false,
          },
          appKey: 'HelloWorld',
          initialProps: {},
          jsBundleProvider: new AnyJSBundleProvider([
            new ResourceJSBundleProvider(
              this.rnohCoreContext.uiAbilityContext.resourceManager,
              'bundle.harmony.js'
            ),
          ]),
        })
      }
    }
    .width('100%')
    .height('100%')
  }
}
