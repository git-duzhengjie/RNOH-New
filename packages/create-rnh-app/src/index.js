/**
 * RNOH-New: create-rnh-app 主流程
 *
 * 解析项目名 → 校验 → 交互式提问 → 拷贝模板 → 变量替换
 * → 执行 npm install → git init → 打印成功提示。
 */

const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const chalk = require('chalk');

const { createProjectDir, copyTemplate } = require('./createProject');
const { askProjectName, askTemplate, askSkipInstall } = require('./prompts');
const {
  validateProjectName,
  toSlug,
  toPascalCase,
  toBundleId,
  getProjectVersion,
} = require('./utils');

/**
 * 获取内置模板目录列表
 *
 * @param {string} templatesRoot 模板根目录
 * @returns {string[]} 模板名列表
 */
function getAvailableTemplates(templatesRoot) {
  if (!fs.existsSync(templatesRoot)) {
    return ['default'];
  }

  const entries = fs.readdirSync(templatesRoot, { withFileTypes: true });
  const templates = entries
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name);

  return templates.length > 0 ? templates : ['default'];
}

/**
 * 解析模板目录
 *
 * @param {string} templateName 模板名
 * @returns {string} 模板目录绝对路径
 */
function resolveTemplateDir(templateName) {
  const templatesRoot = path.join(__dirname, '..', 'template');
  return path.join(templatesRoot, templateName);
}

/**
 * 执行 npm install
 *
 * @param {string} projectDir 项目目录
 */
function runNpmInstall(projectDir) {
  console.log(chalk.cyan('\n📦 正在安装依赖...'));
  const result = spawnSync('npm', ['install'], {
    cwd: projectDir,
    stdio: 'inherit',
    shell: process.platform === 'win32',
  });

  if (result.status !== 0) {
    console.warn(chalk.yellow('⚠️  npm install 未成功，可稍后手动执行 npm install。'));
    return;
  }
  console.log(chalk.green('✅ 依赖安装完成。'));
}

/**
 * 初始化 git 仓库
 *
 * @param {string} projectDir 项目目录
 */
function initGit(projectDir) {
  console.log(chalk.cyan('\n🔧 正在初始化 Git 仓库...'));
  const result = spawnSync('git', ['init'], {
    cwd: projectDir,
    stdio: 'inherit',
    shell: process.platform === 'win32',
  });

  if (result.status !== 0) {
    console.warn(chalk.yellow('⚠️  git init 未成功，可稍后手动执行 git init。'));
    return;
  }
  console.log(chalk.green('✅ Git 仓库初始化完成。'));
}

/**
 * 打印成功提示和后续步骤
 *
 * @param {string} projectDir 项目目录
 * @param {string} projectName 项目名
 */
function printSuccess(projectDir, projectName) {
  console.log('');
  console.log(chalk.green.bold('🎉 项目创建成功！'));
  console.log('');
  console.log(chalk.bold('下一步：'));
  console.log(`  cd ${projectName}`);
  console.log('  # 打开 DevEco Studio 导入 harmony 目录');
  console.log('  # 或者继续开发 React Native 逻辑：');
  console.log('  npx react-native start');
  console.log('');
  console.log(chalk.dim(`项目目录: ${projectDir}`));
}

/**
 * 主流程入口
 *
 * @param {Object} options CLI 解析出的选项
 * @param {string} [options.projectName] 项目名
 * @param {string} [options.template] 模板名
 * @param {boolean} [options.skipInstall] 是否跳过依赖安装
 * @param {boolean} [options.git] 是否初始化 git
 * @param {boolean} [options.typescript] 是否使用 TypeScript
 */
async function main(options) {
  const templatesRoot = path.join(__dirname, '..', 'template');
  const availableTemplates = getAvailableTemplates(templatesRoot);

  // 1. 项目名：命令行未提供时交互式提问
  let projectName = options.projectName;
  if (!projectName) {
    projectName = await askProjectName('my-rnoh-app');
  }

  // 校验合法性
  if (!validateProjectName(projectName)) {
    throw new Error(
      `项目名不合法: ${projectName}\n` +
        '仅允许小写字母、数字、连字符、下划线，且不以数字开头。'
    );
  }

  // 2. 模板：命令行未提供时交互式提问
  let template = options.template || 'default';
  if (!availableTemplates.includes(template)) {
    template = await askTemplate(template, availableTemplates);
  }

  // 3. 是否跳过安装：命令行未指定时交互式提问
  let skipInstall = options.skipInstall;
  if (skipInstall === undefined) {
    skipInstall = await askSkipInstall(false);
  }

  // 4. 生成变量
  const variables = {
    PROJECT_NAME: projectName,
    PROJECT_SLUG: toSlug(projectName),
    PASCAL_CASE: toPascalCase(projectName),
    SLUG_CASE: toSlug(projectName),
    PROJECT_VERSION: getProjectVersion(),
    BUNDLE_ID: toBundleId(projectName),
    TYPESCRIPT: options.typescript !== false ? 'true' : 'false',
  };

  console.log('');
  console.log(chalk.cyan('🚀 正在创建项目...'));
  console.log(chalk.dim(`  项目名: ${variables.PROJECT_NAME}`));
  console.log(chalk.dim(`  模板: ${template}`));
  console.log(chalk.dim(`  Bundle ID: ${variables.BUNDLE_ID}`));

  // 5. 创建项目目录
  const projectDir = createProjectDir(projectName);
  const templateDir = resolveTemplateDir(template);

  if (!fs.existsSync(templateDir)) {
    throw new Error(`模板目录不存在: ${templateDir}`);
  }

  // 6. 拷贝模板并替换变量
  copyTemplate(templateDir, projectDir, variables);
  console.log(chalk.green('✅ 模板文件已生成。'));

  // 7. 执行 npm install（除非 --skip-install）
  if (!skipInstall) {
    runNpmInstall(projectDir);
  } else {
    console.log(chalk.dim('⏭️  已跳过依赖安装（--skip-install）。'));
  }

  // 8. 初始化 git（除非 --no-git）
  if (options.git !== false) {
    initGit(projectDir);
  } else {
    console.log(chalk.dim('⏭️  已跳过 git init（--no-git）。'));
  }

  // 9. 打印成功提示
  printSuccess(projectDir, projectName);
}

module.exports = {
  main,
  getAvailableTemplates,
  resolveTemplateDir,
};