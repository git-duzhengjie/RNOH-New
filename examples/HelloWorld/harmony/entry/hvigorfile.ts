import { hapTasks } from '@ohos/hvigor-ohos-plugin';
import { createRNHModulePlugin } from '@rnoh-new/hvigor-plugin';

export default {
  system: hapTasks,
  plugins: [
    createRNHModulePlugin({
      nodeModulesPath: '../../node_modules',
      rnohModulePath: '../../packages/react-native-harmony',
    }),
  ],
};
