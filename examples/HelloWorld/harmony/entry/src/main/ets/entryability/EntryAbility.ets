/**
 * HelloWorld EntryAbility
 *
 * Extends RNHAbility to host the React Native app.
 */

import { RNHAbility } from '@rnoh-new/react-native-harmony';

export default class EntryAbility extends RNHAbility {
  override getPagePath(): string {
    return 'pages/Index';
  }
}
