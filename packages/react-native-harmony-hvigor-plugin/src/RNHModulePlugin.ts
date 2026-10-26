/**
 * RNOH-New: 模块级 Hvigor 插件
 *
 * 在模块构建前后自动完成：
 * 1. 检查 autolinking 生成文件是否存在，不存在则生成
 * 2. 将 autolinking.cmake include 到当前模块的 CMakeLists.txt
 * 3. 打印编译产物路径
 */

import fs from 'node:fs';
import path from 'node:path';
import { generateAutolinking } from './AutolinkingRunner';
import { runCodegen } from './CodegenTask';
import { ensureAutolinkingInCMake } from './CMakeHelper';
import type {
  HvigorNode,
  HvigorNodeCompat,
  HvigorLifecycleCompat,
  HvigorPlugin,
  RNHModulePluginOptions,
} from './types';

/**
 * 创建 RNOH-New 模块级 hvigor 插件
 *
 * @param options 模块级插件配置
 * @returns hvigor 插件对象
 */
export function createRNHModulePlugin(
  options: RNHModulePluginOptions
): HvigorPlugin {
  return {
    pluginId: 'rnoh-new-module-plugin',
    apply(node: HvigorNode): void {
      const compatNode = node as unknown as HvigorNodeCompat;
      const nodeName = resolveNodeName(compatNode);
      const nodePath = resolveNodePath(compatNode);

      console.log(`[RNHModulePlugin] Applying to module node: ${nodeName}`);

      const lifecycle = resolveLifecycle(compatNode);
      if (!lifecycle) {
        console.warn(
          '[RNHModulePlugin] Unable to resolve hvigor lifecycle, plugin skipped.'
        );
        return;
      }

      registerBeforeBuild(lifecycle, async () => {
        console.log('[RNHModulePlugin] beforeNodeBuild hook triggered.');

        // a) 检查 autolinking 生成文件，不存在则生成
        const rnohModulePath = resolveRnohModulePath(options);
        const autolinkingDir = path.join(
          rnohModulePath,
          'src',
          'main',
          'cpp',
          'RNHAutolinking'
        );
        const autolinkingCmake = path.join(autolinkingDir, 'autolinking.cmake');
        const packageProviderCpp = path.join(
          autolinkingDir,
          'PackageProvider.cpp'
        );

        if (
          !fs.existsSync(autolinkingCmake) ||
          !fs.existsSync(packageProviderCpp)
        ) {
          console.log(
            '[RNHModulePlugin] Autolinking files missing, generating...'
          );
          generateAutolinking({
            nodeModulesPath: options.nodeModulesPath,
            rnohHarmonyRoot: rnohModulePath,
            cwd: nodePath,
          });
        } else {
          console.log('[RNHModulePlugin] Autolinking files already exist.');
        }

        // b) 将 autolinking.cmake include 到当前模块的 CMakeLists.txt
        const cmakePath = path.join(nodePath, 'src', 'main', 'cpp',
          'CMakeLists.txt');
        if (fs.existsSync(cmakePath)) {
          ensureAutolinkingInCMake(cmakePath, autolinkingCmake);
        } else {
          console.log(
            `[RNHModulePlugin] CMakeLists.txt not found, skipping: ${cmakePath}`
          );
        }

        // c) 可选：执行 codegen
        if (options.codegen) {
          console.log('[RNHModulePlugin] Running codegen...');
          runCodegen({
            projectRootPath: options.codegen.projectRootPath,
            rnohModulePath: options.codegen.rnohModulePath,
          });
        }
      });

      registerAfterBuild(lifecycle, async () => {
        console.log(
          `[RNHModulePlugin] Module ${nodeName} built successfully.`
        );
        console.log(`[RNHModulePlugin] Module path: ${nodePath}`);
      });
    },
  };
}

/**
 * 解析 @rnoh-new/react-native-harmony 模块路径
 * @param options 插件选项
 * @returns 模块绝对路径
 */
function resolveRnohModulePath(options: RNHModulePluginOptions): string {
  if (options.rnohModulePath) {
    return options.rnohModulePath;
  }

  // 从 node_modules 解析
  const candidates = [
    path.join(options.nodeModulesPath, '@rnoh-new', 'react-native-harmony'),
    path.join(options.nodeModulesPath, 'react-native-harmony'),
  ];

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  // 默认返回 scoped package 路径
  return candidates[0];
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
    '[RNHModulePlugin] No supported beforeNodeBuild registration API found.'
  );
}

/**
 * 注册 afterNodeBuild 回调（兼容两种 hvigor API 形态）
 * @param lifecycle 生命周期对象
 * @param callback 构建后回调
 */
function registerAfterBuild(
  lifecycle: HvigorLifecycleCompat,
  callback: () => Promise<void>
): void {
  if (typeof lifecycle.afterNodeBuild === 'function') {
    lifecycle.afterNodeBuild(callback);
    return;
  }
  if (typeof lifecycle.registerHook === 'function') {
    lifecycle.registerHook('afterNodeBuild', callback);
    return;
  }
  console.warn(
    '[RNHModulePlugin] No supported afterNodeBuild registration API found.'
  );
}
