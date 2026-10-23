/**
 * RNOH-New: create-rnh-app 项目创建逻辑
 *
 * 负责创建项目目录、递归拷贝模板、替换占位符。
 */

const fs = require('node:fs');
const path = require('node:path');
const fse = require('fs-extra');

/**
 * 模板中特殊文件前缀
 * 以 _template 开头的文件不会被拷贝（如 _template_README.md）
 */
const TEMPLATE_SPECIAL_PREFIX = '_template';

/**
 * 模板文件名重命名前缀
 * 以 tpl- 开头的文件在拷贝时会去掉该前缀（如 tpl-package.json -> package.json）
 * 避免模板内的 package.json 等文件被 npm/编辑器误解
 */
const TEMPLATE_RENAME_PREFIX = 'tpl-';

/**
 * 创建项目目录
 *
 * 若目录已存在且非空，则抛出错误。
 *
 * @param {string} name 项目名
 * @returns {string} 项目目录绝对路径
 */
function createProjectDir(name) {
  const targetDir = path.resolve(process.cwd(), name);

  if (fs.existsSync(targetDir)) {
    const entries = fs.readdirSync(targetDir);
    if (entries.length > 0) {
      throw new Error(
        `目标目录已存在且非空: ${targetDir}\n请选择其他项目名或先清理该目录。`
      );
    }
  }

  fse.ensureDirSync(targetDir);
  return targetDir;
}

/**
 * 递归拷贝模板目录到目标目录
 *
 * 规则：
 * - 跳过以 _template 前缀开头的特殊文件
 * - 跳过 node_modules、.git 等目录
 * - 拷贝文件时进行占位符替换
 *
 * @param {string} templateDir 模板根目录
 * @param {string} targetDir 目标目录
 * @param {Object} variables 替换变量
 */
function copyTemplate(templateDir, targetDir, variables) {
  const skipDirs = new Set(['node_modules', '.git', '.hg', '.svn', 'build']);

  const walk = (currentSource, currentTarget) => {
    const entries = fs.readdirSync(currentSource, { withFileTypes: true });

    for (const entry of entries) {
      const sourcePath = path.join(currentSource, entry.name);
      const targetPath = path.join(currentTarget, entry.name);

      if (entry.isDirectory()) {
        // 跳过特殊目录
        if (skipDirs.has(entry.name)) {
          continue;
        }
        fse.ensureDirSync(targetPath);
        walk(sourcePath, targetPath);
      } else if (entry.isFile()) {
        // 跳过特殊文件
        if (entry.name.startsWith(TEMPLATE_SPECIAL_PREFIX)) {
          console.log(`  · 跳过模板特殊文件: ${entry.name}`);
          continue;
        }

        // 处理 tpl- 前缀重命名
        let targetFileName = entry.name;
        if (targetFileName.startsWith(TEMPLATE_RENAME_PREFIX)) {
          targetFileName = targetFileName.slice(TEMPLATE_RENAME_PREFIX.length);
          console.log(`  · 重命名: ${entry.name} -> ${targetFileName}`);
        }

        // 读取内容并替换变量
        let content = fs.readFileSync(sourcePath, 'utf8');
        content = replaceVariables(content, variables);

        const renamedTargetPath = path.join(currentTarget, targetFileName);
        fse.outputFileSync(renamedTargetPath, content);
      }
    }
  };

  walk(templateDir, targetDir);
}

/**
 * 替换文件内容中的占位符
 *
 * 支持 {{PROJECT_NAME}}、{{PROJECT_SLUG}}、{{PROJECT_VERSION}}、
 * {{BUNDLE_ID}}、{{PASCAL_CASE}}、{{SLUG_CASE}} 等。
 *
 * @param {string} content 原始内容
 * @param {Object} variables 替换变量
 * @returns {string} 替换后的内容
 */
function replaceVariables(content, variables) {
  let result = content;

  // 按变量替换，优先长变量名（避免部分匹配）
  const keys = Object.keys(variables).sort((a, b) => b.length - a.length);
  for (const key of keys) {
    const placeholder = `{{${key}}}`;
    if (result.includes(placeholder)) {
      result = result.split(placeholder).join(variables[key]);
    }
  }

  return result;
}

module.exports = {
  createProjectDir,
  copyTemplate,
  replaceVariables,
  TEMPLATE_SPECIAL_PREFIX,
  TEMPLATE_RENAME_PREFIX,
};