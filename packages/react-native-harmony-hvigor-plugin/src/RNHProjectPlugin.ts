/**
 * RNOH-New: 项目级 Hvigor 插件
 *
 * 在应用构建前自动完成：
 * 1. 读取并注入 .env 环境变量
 * 2. 调用 Metro 打包 JS bundle
 * 3. 调用 autolinking 代码生成
 */

import fs from 'node:fs';
import path from 'node:path';
import { runMetroBundler } from './MetroBundler';
import { generateAutolinking } from './AutolinkingRunner';
import type {
  HvigorNode,
  HvigorNodeCompat,
  HvigorLifecycleCompat,
  HvigorPlugin,
  RNHProjectPluginOptions,
} from './types';

/**
 * 创建 RNOH-New 项目级 hvigor 插件
 *
 * @param options 项目级插件配置
 * @returns hvigor 插件对象
 */
export function createRNHProjectPlugin(
  options: RNHProjectPluginOptions
): HvigorPlugin {
  const resolvedOptions = resolveOptions(options);

  return {
    pluginId: 'rnoh-new-project-plugin',
    apply(node: HvigorNode): void {
      const compatNode = node as unknown as HvigorNodeCompat;
      const nodeName = resolveNodeName(compatNode);
      const nodePath = resolveNodePath(compatNode);

      console.log(`[RNHProjectPlugin] Applying to project node: ${nodeName}`);

      const lifecycle = resolveLifecycle(compatNode);
      if (!lifecycle) {
        console.warn(
          '[RNHProjectPlugin] Unable to resolve hvigor lifecycle, plugin skipped.'
        );
        return;
      }

      registerBeforeBuild(lifecycle, async () => {
        console.log('[RNHProjectPlugin] beforeNodeBuild hook triggered.');

        // a) 读取 .env 文件并注入 process.env
        loadEnvFile(resolvedOptions.envFile);

        // b) 调用 Metro 打包 JS bundle
        await runMetroBundler({
          config: resolvedOptions.bundler.config,
          bundleOutput: resolvedOptions.bundler.bundleOutput,
          assetsDest: resolvedOptions.bundler.assetsDest,
          entryFile: resolvedOptions.bundler.entryFile,
          buildMode: resolvedOptions.buildMode,
          cwd: nodePath,
        });

        // c) 调用 autolinking 代码生成
        generateAutolinking({
          nodeModulesPath: resolvedOptions.nodeModulesPath,
          cwd: nodePath,
        });

        console.log('[RNHProjectPlugin] beforeNodeBuild hook completed.');
      });
    },
  };
}

/**
 * 补全默认选项
 * @param options 用户传入选项
 * @returns 完整选项
 */
function resolveOptions(
  options: RNHProjectPluginOptions
): Required<RNHProjectPluginOptions> {
  return {
    nodeModulesPath: options.nodeModulesPath,
    bundler: options.bundler,
    envFile: options.envFile ?? 'harmony/.env',
    buildMode: options.buildMode ?? 'debug',
  };
}

/**
 * 解析节点名称（兼容不同 hvigor 版本）
 * @param node 兼容节点对象
 * @returns 节点名称
 */
function resolveNodeName(node: HvigorNodeCompat): string {
  if (typeof node.getName === 'function') {
    return node.getName();
  }
  if (typeof node.getNodeName === 'function') {
    return node.getNodeName();
  }
  return 'unknown-node';
}

/**
 * 解析节点路径（兼容不同 hvigor 版本）
 * @param node 兼容节点对象
 * @returns 节点绝对路径
 */
function resolveNodePath(node: HvigorNodeCompat): string {
  if (typeof node.getNodePath === 'function') {
    return node.getNodePath();
  }
  if (typeof node.getNodeDir === 'function') {
    return node.getNodeDir();
  }
  return process.cwd();
}

/**
 * 解析生命周期对象（兼容不同 hvigor 版本）
 * @param node 兼容节点对象
 * @returns 生命周期对象或 undefined
 */
function resolveLifecycle(
  node: HvigorNodeCompat
): HvigorLifecycleCompat | undefined {
  if (typeof node.getLifecycle === 'function') {
    return node.getLifecycle();
  }
  return undefined;
}

/**
 * 注册 beforeNodeBuild 回调（兼容两种 hvigor API 形态）
 *
 * - 新版本：lifecycle.beforeNodeBuild(cb)
 * - 旧/自定义声明：lifecycle.registerHook('beforeNodeBuild', cb)
 *
 * @param lifecycle 生命周期对象
 * @param callback 构建前回调
 */
function registerBeforeBuild(
  lifecycle: HvigorLifecycleCompat,
  callback: () => Promise<void>
): void {
  if (typeof lifecycle.beforeNodeBuild === 'function') {
    lifecycle.beforeNodeBuild(callback);
    return;
  }
  if (typeof lifecycle.registerHook === 'function') {
    lifecycle.registerHook('beforeNodeBuild', callback);
    return;
  }
  console.warn(
    '[RNHProjectPlugin] No supported beforeNodeBuild registration API found.'
  );
}

/**
 * 读取 .env 文件并注入 process.env
 *
 * 支持 KEY=VALUE 格式，忽略空行和 # 注释行。
 * 不会覆盖已存在的环境变量。
 *
 * @param envFilePath .env 文件路径
 */
function loadEnvFile(envFilePath: string): void {
  const absolutePath = path.isAbsolute(envFilePath)
    ? envFilePath
    : path.resolve(process.cwd(), envFilePath);

  console.log(`[RNHProjectPlugin] Loading env file: ${absolutePath}`);

  if (!fs.existsSync(absolutePath)) {
    console.log(
      `[RNHProjectPlugin] Env file not found, skipping: ${absolutePath}`
    );
    return;
  }

  const content = fs.readFileSync(absolutePath, 'utf8');
  const lines = content.split(/\r?\n/);

  let injectedCount = 0;
  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) {
      continue;
    }

    const equalIndex = trimmed.indexOf('=');
    if (equalIndex === -1) {
      continue;
    }

    const key = trimmed.substring(0, equalIndex).trim();
    const value = trimmed.substring(equalIndex + 1).trim();

    // 不覆盖已存在的环境变量
    if (process.env[key] === undefined) {
      process.env[key] = value;
      injectedCount++;
    }
  }

  console.log(
    `[RNHProjectPlugin] Injected ${injectedCount} env variable(s) from ${absolutePath}`
  );
}
