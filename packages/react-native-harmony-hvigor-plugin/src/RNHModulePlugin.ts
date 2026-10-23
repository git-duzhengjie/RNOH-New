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
import { ensureAutolinkingInCMake, ensureAutolinkingInETS } from './CMakeHelper';
import type {
  HvigorNode,
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
      console.log(
        `[RNHModulePlugin] Applying to module node: ${node.getName()}`
      );

      const lifecycle = node.getLifecycle();

      lifecycle.registerHook('beforeNodeBuild', async () => {
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

        if (!fs.existsSync(autolinkingCmake) || !fs.existsSync(packageProviderCpp)) {
          console.log('[RNHModulePlugin] Autolinking files missing, generating...');
          generateAutolinking({
            nodeModulesPath: options.nodeModulesPath,
            rnohHarmonyRoot: rnohModulePath,
            cwd: node.getNodePath(),
          });
        } else {
          console.log('[RNHModulePlugin] Autolinking files already exist.');
        }

        // b) 执行 codegen（若配置了）
        if (options.codegen) {
          runCodegen({
            projectRootPath: options.codegen.projectRootPath,
            rnohModulePath: options.codegen.rnohModulePath,
            cwd: node.getNodePath(),
          });
        }

        // c) 将 autolinking.cmake include 到当前模块的 CMakeLists.txt
        if (options.autolinking?.enabled !== false) {
          const modulePath = node.getNodePath();
          const cmakePath = path.join(modulePath, 'src', 'main', 'cpp', 'CMakeLists.txt');
          if (fs.existsSync(cmakePath)) {
            ensureAutolinkingInCMake(cmakePath, autolinkingCmake);
          } else {
            console.warn(
              `[RNHModulePlugin] CMakeLists.txt not found, skip CMake injection: ${cmakePath}`
            );
          }

          // 注入 ETS import
          const etsEntryPath = path.join(
            rnohModulePath,
            'src',
            'main',
            'ets',
            'index.ets'
          );
          const packageProviderEts = path.join(
            rnohModulePath,
            'src',
            'main',
            'ets',
            'Autolinking',
            'RNHPackageProvider.ets'
          );
          if (fs.existsSync(etsEntryPath) && fs.existsSync(packageProviderEts)) {
            ensureAutolinkingInETS(etsEntryPath, packageProviderEts);
          }
        }

        console.log('[RNHModulePlugin] beforeNodeBuild hook completed.');
      });

      lifecycle.registerHook('afterNodeBuild', async () => {
        console.log('[RNHModulePlugin] afterNodeBuild hook triggered.');
        // 打印编译产物路径
        const modulePath = node.getNodePath();
        const buildOutputDir = path.join(
          modulePath,
          'build',
          'default',
          'outputs',
          'default'
        );
        console.log(`[RNHModulePlugin] Build output directory: ${buildOutputDir}`);
        console.log(
          `[RNHModulePlugin] Module: ${node.getName()} built successfully.`
        );
      });
    },
  };
}

/**
 * 解析 @rnoh-new/react-native-harmony 模块根路径
 * @param options 模块级插件选项
 * @returns 模块根路径
 */
function resolveRnohModulePath(options: RNHModulePluginOptions): string {
  if (options.rnohModulePath) {
    return path.isAbsolute(options.rnohModulePath)
      ? options.rnohModulePath
      : path.resolve(process.cwd(), options.rnohModulePath);
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