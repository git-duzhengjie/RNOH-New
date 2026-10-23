/**
 * RNOH-New: create-rnh-app 工具函数
 *
 * 项目名校验、命名格式转换等通用工具。
 */

const RESERVED_NAMES = new Set([
  'node_modules',
  'favicon.ico',
  '.DS_Store',
  'test',
  'tests',
  'lib',
  'src',
  'dist',
  'build',
  'bin',
  'template',
  'examples',
  'docs',
]);

/**
 * 校验项目名是否合法
 *
 * 规则：
 * - 非空字符串
 * - 仅包含小写字母、数字、连字符、下划线
 * - 不以数字开头
 * - 非保留字
 *
 * @param {string} name 待校验的项目名
 * @returns {boolean} 是否合法
 */
function validateProjectName(name) {
  if (!name || typeof name !== 'string') {
    return false;
  }

  const trimmed = name.trim();
  if (!trimmed) {
    return false;
  }

  // 仅小写字母、数字、连字符、下划线
  if (!/^[a-z0-9\-_]+$/.test(trimmed)) {
    return false;
  }

  // 不以数字开头
  if (/^[0-9]/.test(trimmed)) {
    return false;
  }

  // 非保留字
  if (RESERVED_NAMES.has(trimmed)) {
    return false;
  }

  return true;
}

/**
 * 将项目名转为短横线（kebab-case）格式
 *
 * 例如：MyApp -> my-app, my_app -> my-app, my-app -> my-app
 *
 * @param {string} name 原始项目名
 * @returns {string} 短横线格式
 */
function toSlug(name) {
  return name
    .trim()
    .replace(/([a-z0-9])([A-Z])/g, '$1-$2') // 驼峰边界加连字符
    .replace(/[\s_]+/g, '-') // 空白与下划线转连字符
    .replace(/--+/g, '-') // 合并连续连字符
    .replace(/^-|-$/g, '') // 去掉首尾连字符
    .toLowerCase();
}

/**
 * 将项目名转为大驼峰（PascalCase）格式
 *
 * 例如：my-app -> MyApp, my_app -> MyApp
 *
 * @param {string} name 原始项目名
 * @returns {string} 大驼峰格式
 */
function toPascalCase(name) {
  const slug = toSlug(name);
  return slug
    .split('-')
    .filter(Boolean)
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join('');
}

/**
 * 生成 Android 风格 bundle id
 *
 * 例如：my-app -> com.rnohnew.my_app
 *
 * @param {string} name 原始项目名
 * @returns {string} bundle id
 */
function toBundleId(name) {
  const slug = toSlug(name);
  // bundle id 中连字符不合法，转为下划线
  const normalized = slug.replace(/-/g, '_');
  return `com.rnohnew.${normalized}`;
}

/**
 * 获取项目版本号（默认 0.1.0）
 *
 * @param {string|undefined} version 用户指定的版本号
 * @returns {string} 版本号
 */
function getProjectVersion(version) {
  return version || '0.1.0';
}

module.exports = {
  validateProjectName,
  toSlug,
  toPascalCase,
  toBundleId,
  getProjectVersion,
  RESERVED_NAMES,
};