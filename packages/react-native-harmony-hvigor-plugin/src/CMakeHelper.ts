/**
 * RNOH-New: CMake / ETS 文件注入辅助
 *
 * 负责将 autolinking 产生的 include / import 注入到
 * 当前模块的 CMakeLists.txt 与 ETS 入口文件中。
 * 修改前自动备份为 .bak。
 */

import fs from 'node:fs';
import path from 'node:path';

/**
 * 将 autolinking.cmake include 到目标 CMakeLists.txt
 *
 * 若目标文件尚未 include autolinking.cmake，则自动追加。
 * 修改前备份原文件为 <name>.bak。
 *
 * @param cmakePath 当前模块的 CMakeLists.txt 路径
 * @param autolinkingCmakePath autolinking.cmake 路径（绝对或相对）
 * @returns 是否发生了修改
 */
export function ensureAutolinkingInCMake(
  cmakePath: string,
  autolinkingCmakePath: string
): boolean {
  if (!fs.existsSync(cmakePath)) {
    throw new Error(`[RNHModulePlugin] CMakeLists.txt not found: ${cmakePath}`);
  }

  const content = fs.readFileSync(cmakePath, 'utf8');

  // 计算相对 include 路径
  const cmakeDir = path.dirname(cmakePath);
  const relPath = path.relative(cmakeDir, autolinkingCmakePath);
  const normalized = relPath.replace(/\\/g, '/');
  const includeLine = `include(${normalized})`;

  // 已 include 则跳过
  if (content.includes(normalized) || content.includes('autolinking.cmake')) {
    console.log(`[RNHModulePlugin] CMakeLists.txt already includes autolinking.`);
    return false;
  }

  // 备份
  const backupPath = cmakePath + '.bak';
  fs.copyFileSync(cmakePath, backupPath);

  // 追加 include（并同时展开 autolink_libraries 宏占位）
  const appendBlock = `\n# --- RNOH-New Autolinking (auto-injected) ---\n${includeLine}\nautolink_libraries(\${TARGET_NAME})\n`;

  fs.appendFileSync(cmakePath, appendBlock, 'utf8');
  console.log(
    `[RNHModulePlugin] Injected autolinking into ${cmakePath} (backup: ${backupPath})`
  );
  return true;
}

/**
 * 将 PackageProvider import 注入到目标 ETS 入口文件
 *
 * 若目标文件尚未 import，自动追加。修改前备份原文件为 .bak。
 *
 * @param etsEntryPath 当前模块的 ETS 入口文件路径
 * @param packageProviderPath PackageProvider 文件路径
 * @returns 是否发生了修改
 */
export function ensureAutolinkingInETS(
  etsEntryPath: string,
  packageProviderPath: string
): boolean {
  if (!fs.existsSync(etsEntryPath)) {
    throw new Error(`[RNHModulePlugin] ETS entry not found: ${etsEntryPath}`);
  }

  const content = fs.readFileSync(etsEntryPath, 'utf8');

  // 计算相对 import 路径（ETS 使用相对路径，不带扩展名）
  const etsDir = path.dirname(etsEntryPath);
  let relPath = path.relative(etsDir, packageProviderPath);
  relPath = relPath.replace(/\\/g, '/');
  // 去掉 .ets 扩展名
  relPath = relPath.replace(/\.ets$/, '');
  // 确保以 ./ 开头
  if (!relPath.startsWith('.')) {
    relPath = './' + relPath;
  }

  const importLine = `import { createAutolinkedPackages } from '${relPath}';`;

  // 已注入则跳过
  if (content.includes('createAutolinkedPackages')) {
    console.log(`[RNHModulePlugin] ETS entry already imports autolinking.`);
    return false;
  }

  // 备份
  const backupPath = etsEntryPath + '.bak';
  fs.copyFileSync(etsEntryPath, backupPath);

  // 在文件头部注入 import
  const injected = `${importLine}\n${content}`;

  fs.writeFileSync(etsEntryPath, injected, 'utf8');
  console.log(
    `[RNHModulePlugin] Injected autolinking import into ${etsEntryPath} (backup: ${backupPath})`
  );
  return true;
}

/**
 * 将 import 行追加到文件头部（通用辅助）
 *
 * @param filePath 目标文件路径
 * @param importLine 要追加的 import 行
 * @returns 是否发生修改
 */
export function prependImport(filePath: string, importLine: string): boolean {
  if (!fs.existsSync(filePath)) {
    throw new Error(`[RNHModulePlugin] File not found: ${filePath}`);
  }

  const content = fs.readFileSync(filePath, 'utf8');
  if (content.includes(importLine)) {
    return false;
  }

  const backupPath = filePath + '.bak';
  fs.copyFileSync(filePath, backupPath);

  fs.writeFileSync(filePath, `${importLine}\n${content}`, 'utf8');
  return true;
}