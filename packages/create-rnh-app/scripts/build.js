#!/usr/bin/env node
/**
 * RNOH-New: create-rnh-app 构建脚本
 *
 * 本包为纯 JavaScript CLI，无需编译。构建阶段做三件事：
 *   1. 校验目录结构（bin 入口、src、template 是否齐备）
 *   2. 对所有发布的 .js 文件做语法校验（先按 CJS，失败再按 ESM）
 *   3. 校验 template/default 模板的关键文件存在
 *
 * 任一校验失败则以非 0 退出码终止，供 CI 使用。
 */
'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');

const packageRoot = path.resolve(__dirname, '..');
const checkDirs = ['bin', 'src'];
const requiredTemplateFiles = [
  // 模板中刻意以 tpl- 前缀命名，拷插时再重命名为 package.json
  'tpl-package.json',
  'src/App.tsx',
  'harmony/hvigorfile.ts',
];

let failed = 0;

function fail(message) {
  console.error(`❌ ${message}`);
  failed += 1;
}

function collectJsFiles(dir) {
  const absolute = path.join(packageRoot, dir);
  if (!fs.existsSync(absolute)) {
    fail(`缺少目录: ${dir}`);
    return [];
  }

  const result = [];
  for (const entry of fs.readdirSync(absolute, { withFileTypes: true })) {
    const child = path.join(absolute, entry.name);
    if (entry.isDirectory()) {
      result.push(...collectJsFiles(path.join(dir, entry.name)));
    } else if (entry.isFile() && /\.(js|mjs|cjs)$/.test(entry.name)) {
      result.push(child);
    }
  }
  return result;
}

function checkSyntax(file) {
  const relative = path.relative(packageRoot, file);

  const cjs = spawnSync(process.execPath, ['--check', file], { encoding: 'utf8' });
  if (cjs.status === 0) {
    return true;
  }

  const esm = spawnSync(process.execPath, ['--input-type=module', '--check'], {
    input: fs.readFileSync(file, 'utf8'),
    encoding: 'utf8',
  });
  if (esm.status === 0) {
    return true;
  }

  console.error(`❌ 语法错误: ${relative}`);
  console.error((cjs.stderr || esm.stderr || '').split('\n').slice(0, 5).join('\n'));
  failed += 1;
  return false;
}

function main() {
  console.log('▶ create-rnh-app 构建校验');

  // 1. CLI 入口
  const binEntry = path.join(packageRoot, 'bin', 'index.js');
  if (fs.existsSync(binEntry)) {
    console.log('  ✓ bin 入口存在: bin/index.js');
  } else {
    fail('缺少 bin 入口: bin/index.js');
  }

  // 2. 语法校验
  const files = checkDirs.flatMap(collectJsFiles);
  let checked = 0;
  for (const file of files) {
    if (checkSyntax(file)) {
      checked += 1;
    }
  }
  console.log(`  ✓ 语法校验通过: ${checked}/${files.length} 个文件`);

  // 3. 模板完整性
  const templateRoot = path.join(packageRoot, 'template', 'default');
  if (!fs.existsSync(templateRoot)) {
    fail('缺少模板目录: template/default');
  } else {
    for (const relative of requiredTemplateFiles) {
      if (fs.existsSync(path.join(templateRoot, relative))) {
        console.log(`  ✓ 模板文件存在: template/default/${relative}`);
      } else {
        fail(`缺少模板文件: template/default/${relative}`);
      }
    }
  }

  if (failed > 0) {
    console.error(`\n❌ 构建校验失败，共 ${failed} 项问题`);
    process.exit(1);
  }

  console.log('\n✅ create-rnh-app 构建校验通过');
}

main();
