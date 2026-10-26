/**
 * HelloWorld EntryAbility
 *
 * 基于官方 RNOH 的 RNAbility 承载 React Native 页面。
 */

import { RNAbility } from '@rnoh-new/react-native-harmony';

export default class EntryAbility extends RNAbility {
  override getPagePath(): string {
    return 'pages/Index';
  }
}
