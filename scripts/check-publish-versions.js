#!/usr/bin/env node
/**
 * RNOH-New: 发布前版本一致性校验
 *
 * 在 publish 工作流真正 npm publish 之前，先做三道校验：
 *   1. 三个发布包的版本号必须一致（monorepo lockstep 发布）
 *   2. tag 触发时，tag 名去掉 v 前缀必须与包版本号一致（如 v0.1.0 -> 0.1.0）
 *   3. 该版本若已存在于 npm registry，提前失败并给出明确提示
 *
 * 环境变量：
 *   GITHUB_REF       由 GitHub Actions 注入，如 refs/tags/v0.1.0
 *   SKIP_REGISTRY_CHECK=1  跳过第 3 步（离线环境使用）
 *
 * 退出码：0 表示校验通过，1 表示校验失败。
 */
'use strict';

const fs = require('node:fs');
const path = require('node:path');

const root = path.resolve(__dirname, '..');

const PACKAGE_DIRS = [
  'packages/react-native-harmony',
  'packages/react-native-harmony-hvigor-plugin',
  'packages/create-rnh-app',
];

const REGISTRY = process.env.NPM_REGISTRY || 'https://registry.npmjs.org';

let failed = false;

function fail(message) {
  console.error(`❌ ${message}`);
  failed = true;
}

function info(message) {
  console.log(`  ✓ ${message}`);
}

function warn(message) {
  console.log(`  ⚠ ${message}`);
}

function readPackages() {
  return PACKAGE_DIRS.map((dir) => {
    const file = path.join(root, dir, 'package.json');
    if (!fs.existsSync(file)) {
      fail(`缺少 package.json: ${dir}`);
      return null;
    }
    const json = JSON.parse(fs.readFileSync(file, 'utf8'));
    return { dir, name: json.name, version: json.version, isPrivate: json.private === true };
  }).filter(Boolean);
}

/** 校验 1：三个包版本号一致 */
function checkVersionLockstep(packages) {
  const versions = [...new Set(packages.map((p) => p.version))];
  if (versions.length === 1) {
    info(`版本号一致: ${versions[0]}`);
    return versions[0];
  }
  fail(`三个包版本号不一致: ${packages.map((p) => `${p.name}@${p.version}`).join(', ')}`);
  return versions[0];
}

/** 校验 2：tag 名（去 v 前缀）与包版本号一致 */
function checkTagMatchesVersion(version) {
  const ref = process.env.GITHUB_REF || '';

  if (!ref.startsWith('refs/tags/')) {
    warn(`非 tag 触发（GITHUB_REF=${ref || '未设置'}），跳过 tag 与版本一致性校验`);
    return;
  }

  const tag = ref.replace('refs/tags/', '');
  const expected = tag.replace(/^v/, '');

  if (expected === version) {
    info(`tag 与版本号一致: ${tag} -> ${version}`);
    return;
  }

  fail(`tag ${tag} 与包版本号 ${version} 不一致（期望 tag 为 v${version}）`);
}

/** 校验 3：版本是否已存在于 npm registry */
async function checkAlreadyPublished(packages, version) {
  if (process.env.SKIP_REGISTRY_CHECK === '1') {
    warn('已设置 SKIP_REGISTRY_CHECK=1，跳过 registry 存在性检查');
    return;
  }

  const published = [];

  for (const pkg of packages) {
    const url = `${REGISTRY}/${pkg.name.replace('/', '%2F')}/${version}`;
    try {
      const response = await fetch(url, { method: 'GET' });
      if (response.status === 200) {
        published.push(pkg.name);
      } else if (response.status !== 404) {
        warn(`${pkg.name} 查询返回 ${response.status}，请手动确认`);
      }
    } catch (error) {
      warn(`${pkg.name} 查询失败（${error.message}），跳过存在性检查`);
    }
  }

  if (published.length === 0) {
    info(`npm registry 上不存在 ${version}，可以发布`);
    return;
  }

  fail(`以下包已存在 ${version} 版本，请先升级版本号: ${published.join(', ')}`);
}

async function main() {
  console.log('▶ 发布前版本校验');

  const packages = readPackages();
  if (packages.length !== PACKAGE_DIRS.length) {
    console.error('\n❌ 版本校验失败：包信息读取不完整');
    process.exit(1);
  }

  for (const pkg of packages) {
    console.log(`   · ${pkg.name}@${pkg.version}`);
    if (pkg.isPrivate === true) {
      fail(`${pkg.name} 标记为 private，无法发布`);
    }
  }

  const version = checkVersionLockstep(packages);
  checkTagMatchesVersion(version);
  await checkAlreadyPublished(packages, version);

  if (failed) {
    console.error('\n❌ 版本校验失败，已中止发布');
    process.exit(1);
  }

  console.log('\n✅ 版本校验通过');
}

main().catch((error) => {
  console.error(`❌ 版本校验异常: ${error.stack || error.message}`);
  process.exit(1);
});
