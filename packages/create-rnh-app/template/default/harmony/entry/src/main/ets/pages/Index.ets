/**
 * {{PROJECT_NAME}} Index Page
 *
 * Top-level ArkUI page that hosts the RNHApp component.
 */

import { RNHApp } from '@rnoh-new/react-native-harmony';

@Entry
@Component
struct Index {
  build() {
    RNHApp({
      config: {
        moduleName: '{{PROJECT_NAME}}',
        bundlePath: '',
        metroUrl: '',
        enableHermes: true,
        enableFabric: true,
        enableTurboModules: true,
      }
    })
      .width('100%')
      .height('100%')
  }
}
