#!/usr/bin/env node
/**
 * RNOH-New: create-rnh-app CLI 入口
 *
 * 使用 commander 解析命令行参数并调用主流程。
 */

const { Command } = require('commander');
const chalk = require('chalk');
const { main } = require('../src/index');
const pkg = require('../package.json');

const program = new Command();

program
  .name('create-rnh-app')
  .description('创建 React Native for HarmonyOS (RNOH-New) 应用脚手架')
  .version(pkg.version)
  .argument('[project-name]', '项目名称')
  .option('-t, --template <name>', '模板名称', 'default')
  .option('--skip-install', '跳过依赖安装')
  .option('--no-git', '不初始化 Git 仓库')
  .option('--typescript', '使用 TypeScript（默认开启）', true)
  .option('--no-typescript', '不使用 TypeScript')
  .action(async (projectName, options) => {
    try {
      await main({
        projectName,
        template: options.template,
        skipInstall: options.skipInstall || false,
        git: options.git,
        typescript: options.typescript,
      });
    } catch (error) {
      console.error(chalk.red(`\n❌ 创建失败: ${error.message}`));
      process.exit(1);
    }
  });

program.parse(process.argv);