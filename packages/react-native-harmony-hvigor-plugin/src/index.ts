/**
 * RNOH-New: Hvigor 插件入口
 *
 * 导出项目级插件、模块级插件以及配套类型与工具函数。
 */

export { createRNHProjectPlugin } from './RNHProjectPlugin';
export { createRNHModulePlugin } from './RNHModulePlugin';
export { runMetroBundler } from './MetroBundler';
export { generateAutolinking } from './AutolinkingRunner';
export { runCodegen } from './CodegenTask';
export {
  ensureAutolinkingInCMake,
  ensureAutolinkingInETS,
  prependImport,
} from './CMakeHelper';

export type {
  RNHProjectPluginOptions,
  RNHModulePluginOptions,
  MetroBundlerOptions,
  HvigorNode,
  HvigorLifecycle,
  HvigorPlugin,
} from './types';

export type { CodegenOptions, PackageCodegenConfig } from './CodegenTask';
export type { GenerateAutolinkingOptions } from './AutolinkingRunner';
export type { RunMetroBundlerOptions } from './MetroBundler';