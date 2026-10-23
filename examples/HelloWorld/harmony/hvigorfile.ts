import { appTasks } from '@ohos/hvigor-ohos-plugin';
import { createRNHProjectPlugin } from '@rnoh-new/hvigor-plugin';

export default {
  system: appTasks,
  plugins: [
    createRNHProjectPlugin({
      nodeModulesPath: '../node_modules',
      bundler: {
        config: '../metro.config.js',
        bundleOutput: './entry/src/main/resources/rawfile/bundle.harmony.js',
        assetsDest: './entry/src/main/resources/rawfile/assets',
      }
    })
  ],
};
