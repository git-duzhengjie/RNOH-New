/**
 * RNOH-New: Autolinking 代码生成执行器
 *
 * 调用 @rnoh-new/react-native-harmony 的 autolinking 脚本，
 * 生成 C++ 和 ETS 侧的包注册表。
 */

import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';

/**
 * Autolinking 执行选项
 */
export interface GenerateAutolinkingOptions {
  /**
   * node_modules 目录路径
   */
  nodeModulesPath: string;

  /**
   * @rnoh-new/react-native-harmony 包根目录
   * 默认从 node_modulesPath 下解析
   */
  rnohHarmonyRoot?: string;

  /**
   * 工作目录
   */
  cwd?: string;
}

/**
 * 运行 autolinking 代码生成
 *
 * 调用 `node scripts/generate-autolinking.js --nodeModules <path>`
 * 生成 C++ 与 ETS 注册表文件。
 *
 * @param options 执行选项
 * @throws 当生成失败时抛出 Error
 */
export function generateAutolinking(options: GenerateAutolinkingOptions): void {
  const {
    nodeModulesPath,
    rnohHarmonyRoot,
    cwd = process.cwd(),
  } = options;

  // 确定 @rnoh-new/react-native-harmony 包根目录
  const resolvedRnohRoot =
    rnohHarmonyRoot ?? resolveRnohHarmonyRoot(nodeModulesPath);

  if (!fs.existsSync(resolvedRnohRoot)) {
    throw new Error(
      `[RNHProjectPlugin] @rnoh-new/react-native-harmony not found at ${resolvedRnohRoot}`
    );
  }

  const scriptPath = path.join(
    resolvedRnohRoot,
    'scripts',
    'generate-autolinking.js'
  );

  if (!fs.existsSync(scriptPath)) {
    throw new Error(
      `[RNHProjectPlugin] Autolinking script not found: ${scriptPath}`
    );
  }

  console.log(`[RNHProjectPlugin] Running autolinking...`);
  console.log(`  script: ${scriptPath}`);
  console.log(`  nodeModules: ${nodeModulesPath}`);

  const args = [
    scriptPath,
    `--nodeModules=${nodeModulesPath}`,
  ];

  const result = spawnSync('node', args, {
    cwd,
    stdio: 'inherit',
    shell: false,
    env: process.env,
  });

  if (result.error) {
    throw new Error(
      `[RNHProjectPlugin] Autolinking spawn failed: ${result.error.message}`
    );
  }

  if (result.status !== 0) {
    throw new Error(
      `[RNHProjectPlugin] Autolinking exited with code ${result.status ?? 'unknown'}`
    );
  }

  // 验证生成文件存在
  const expectedFiles = [
    path.join(resolvedRnohRoot, 'src', 'main', 'cpp', 'RNHAutolinking', 'PackageProvider.h'),
    path.join(resolvedRnohRoot, 'src', 'main', 'cpp', 'RNHAutolinking', 'PackageProvider.cpp'),
    path.join(resolvedRnohRoot, 'src', 'main', 'cpp', 'RNHAutolinking', 'autolinking.cmake'),
    path.join(resolvedRnohRoot, 'src', 'main', 'ets', 'Autolinking', 'RNHPackageProvider.ets'),
  ];

  for (const file of expectedFiles) {
    if (!fs.existsSync(file)) {
      throw new Error(
        `[RNHProjectPlugin] Expected autolinking output missing: ${file}`
      );
    }
  }

  console.log(`[RNHProjectPlugin] Autolinking completed successfully.`);
}

/**
 * 解析 @rnoh-new/react-native-harmony 包根目录
 * @param nodeModulesPath node_modules 路径
 * @returns 包根目录路径
 */
function resolveRnohHarmonyRoot(nodeModulesPath: string): string {
  const candidates = [
    path.join(nodeModulesPath, '@rnoh-new', 'react-native-harmony'),
    path.join(nodeModulesPath, 'react-native-harmony'),
  ];

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  // 默认返回 scoped package 路径
  return candidates[0];
}
