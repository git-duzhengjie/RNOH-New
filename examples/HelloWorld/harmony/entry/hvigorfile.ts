import { harTasks } from '@ohos/hvigor-ohos-plugin';
import { createRNHModulePlugin } from '@rnoh-new/hvigor-plugin';

export default {
  system: harTasks,
  plugins: [
    createRNHModulePlugin({
      // Entry module configuration
    })
  ],
};
