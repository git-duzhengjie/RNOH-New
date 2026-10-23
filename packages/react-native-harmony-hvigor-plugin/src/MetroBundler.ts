/**
 * RNOH-New: Metro Bundle 打包器
 *
 * 负责调用 Metro 打包 React Native JS bundle。
 * 优先使用 @react-native-community/cli 的 bundle 命令，
 * 保证与社区标准一致。
 */

import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import type { MetroBundlerOptions } from './types';

/**
 * Metro 打包器选项
 */
export interface RunMetroBundlerOptions extends MetroBundlerOptions {
  /**
   * 构建模式
   */
  buildMode?: 'debug' | 'release';

  /**
   * 工作目录
   */
  cwd?: string;
}

/**
 * 运行 Metro Bundle 打包
 *
 * 使用 `npx react-native bundle` 命令行方式执行，
 * 生成 platform=harmony 的 bundle 文件。
 *
 * @param options 打包配置选项
 * @throws 当打包失败时抛出 Error
 */
export async function runMetroBundler(
  options: RunMetroBundlerOptions
): Promise<void> {
  const {
    config,
    bundleOutput,
    assetsDest,
    entryFile = 'index.js',
    buildMode = 'debug',
    cwd = process.cwd(),
  } = options;

  // 确保输出目录存在
  ensureDirectory(path.dirname(bundleOutput));
  ensureDirectory(assetsDest);

  const dev = buildMode === 'debug';

  console.log(`[RNHProjectPlugin] Running Metro bundler...`);
  console.log(`  mode: ${buildMode}`);
  console.log(`  entry: ${entryFile}`);
  console.log(`  output: ${bundleOutput}`);
  console.log(`  assetsDest: ${assetsDest}`);

  const args = [
    'react-native',
    'bundle',
    '--platform',
    'harmony',
    '--dev',
    dev ? 'true' : 'false',
    '--entry-file',
    entryFile,
    '--bundle-output',
    bundleOutput,
    '--assets-dest',
    assetsDest,
    '--config',
    config,
  ];

  const result = spawnSync('npx', args, {
    cwd,
    stdio: 'inherit',
    shell: true,
    env: {
      ...process.env,
      // 强制使用 harmony 平台
      RCT_METRO_PLATFORMS: 'harmony',
    },
  });

  if (result.error) {
    throw new Error(
      `[RNHProjectPlugin] Metro bundler spawn failed: ${result.error.message}`
    );
  }

  if (result.status !== 0) {
    throw new Error(
      `[RNHProjectPlugin] Metro bundler exited with code ${result.status ?? 'unknown'}`
    );
  }

  if (!fs.existsSync(bundleOutput)) {
    throw new Error(
      `[RNHProjectPlugin] Bundle output not found: ${bundleOutput}`
    );
  }

  console.log(`[RNHProjectPlugin] Bundle generated successfully: ${bundleOutput}`);
}

/**
 * 确保目录存在
 * @param dirPath 目录路径
 */
function ensureDirectory(dirPath: string): void {
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
  }
}
