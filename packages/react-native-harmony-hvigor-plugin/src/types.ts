/**
 * RNOH-New Hvigor Plugin 类型定义
 *
 * 本文件声明插件选项与 Hvigor 相关类型的最小接口。
 * 由于 hvigor 未实际安装，这里使用本地 interface 占位，
 * 避免依赖完整的 @ohos/hvigor 类型。
 */

/**
 * Hvigor 生命周期对象的最小接口
 * 用于注册 beforeNodeBuild 等构建钩子。
 */
export interface HvigorLifecycle {
  /**
   * 注册指定生命周期的回调钩子
   * @param hookName 钩子名称
   * @param callback 回调函数
   */
  registerHook(
    hookName: 'beforeNodeBuild' | 'afterNodeBuild' | 'beforeNodeEvaluate',
    callback: () => Promise<void> | void
  ): void;
}

/**
 * Hvigor 生命周期对象的真实 API 形态（兼容处理用）
 *
 * 真实 hvigor 提供的是形如 lifecycle.beforeNodeBuild(cb) 的方法，
 * 而不是 registerHook(name, cb)。两者都兼容处理。
 */
export interface HvigorLifecycleCompat extends Partial<HvigorLifecycle> {
  beforeNodeBuild?(callback: () => Promise<void> | void): void;
  afterNodeBuild?(callback: () => Promise<void> | void): void;
}

/**
 * Hvigor 节点的最小接口
 * 项目级插件通过该节点获取生命周期与节点信息。
 */
export interface HvigorNode {
  /**
   * 获取节点的生命周期对象
   */
  getLifecycle(): HvigorLifecycle;

  /**
   * 获取节点名称
   */
  getName(): string;

  /**
   * 获取节点路径
   */
  getNodePath(): string;
}

/**
 * Hvigor 节点的真实 API 形态（兼容处理用）
 *
 * 真实 hvigor 的节点提供 getNodeName() / getNodePath() / getLifecycle()，
 * 部分版本提供 getName()。这里全部声明为可选，运行时按可用性依次尝试。
 */
export interface HvigorNodeCompat {
  getLifecycle?(): HvigorLifecycleCompat;
  getName?(): string;
  getNodeName?(): string;
  getNodePath?(): string;
  getNodeDir?(): string;
}

/**
 * Hvigor 插件对象的标准结构
 */
export interface HvigorPlugin {
  /**
   * 插件唯一标识
   */
  pluginId: string;

  /**
   * 插件应用函数
   * @param node 当前 hvigor 节点
   */
  apply(node: HvigorNode): void;
}

/**
 * Metro 打包配置
 */
export interface MetroBundlerOptions {
  /**
   * Metro 配置文件路径
   */
  config: string;

  /**
   * bundle 输出路径
   */
  bundleOutput: string;

  /**
   * 静态资源输出目录
   */
  assetsDest: string;

  /**
   * 入口文件路径，默认 index.js
   */
  entryFile?: string;
}

/**
 * 项目级插件选项
 */
export interface RNHProjectPluginOptions {
  /**
   * node_modules 目录路径
   * 用于 autolinking 扫描
   */
  nodeModulesPath: string;

  /**
   * Metro 打包配置
   */
  bundler: MetroBundlerOptions;

  /**
   * 环境变量文件路径
   * 默认 harmony/.env
   */
  envFile?: string;

  /**
   * 构建模式：debug 或 release
   * 默认 debug
   */
  buildMode?: 'debug' | 'release';
}

/**
 * 模块级插件选项
 */
export interface RNHModulePluginOptions {
  /**
   * node_modules 目录路径
   */
  nodeModulesPath: string;

  /**
   * @rnoh-new/react-native-harmony 模块路径
   */
  rnohModulePath?: string;

  /**
   * Metro 相关配置（可选，占位）
   */
  metro?: unknown;

  /**
   * Codegen 配置
   */
  codegen?: {
    projectRootPath: string;
    rnohModulePath: string;
  };

  /**
   * Autolinking 配置
   */
  autolinking?: {
    enabled: boolean;
    outputDir: string;
  };
}
