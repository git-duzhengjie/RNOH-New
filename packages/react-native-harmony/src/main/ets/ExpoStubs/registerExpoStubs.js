/**
 * RNOH-New: Expo Stubs 注册器
 *
 * 提供 registerExpoStubs() 与 metro resolver.alias 配置片段，
 * 用于在缺少原生 Expo 实现时将 expo-* 模块映射到本地 stub。
 *
 * 用法（ExpoDemo 的 metro.config.js）：
 *   const { getExpoStubsAlias } = require('react-native-harmony/ExpoStubs/registerExpoStubs');
 *   ...
 *   module.exports = {
 *     resolver: {
 *       alias: {
 *         ...getExpoStubsAlias(),
 *       },
 *     },
 *   };
 */

'use strict';

const path = require('path');

/**
 * 获取 ExpoStubs 目录绝对路径
 * @returns {string} 目录路径
 */
function getStubsDir() {
  return __dirname;
}

/**
 * 获取 Metro resolver.alias 配置片段
 *
 * 将 expo-secure-store / expo-haptics / expo-crypto /
 * expo-splash-screen / expo-status-bar 映射到本地 stub。
 *
 * @returns {Object} alias 映射对象
 */
function getExpoStubsAlias() {
  const stubsDir = getStubsDir();
  return {
    'expo-secure-store': path.join(stubsDir, 'expo-secure-store.js'),
    'expo-haptics': path.join(stubsDir, 'expo-haptics.js'),
    'expo-crypto': path.join(stubsDir, 'expo-crypto.js'),
    'expo-splash-screen': path.join(stubsDir, 'expo-splash-screen.js'),
    'expo-status-bar': path.join(stubsDir, 'expo-status-bar.js'),
  };
}

/**
 * 注册 Expo Stubs（应用运行时钩子）
 *
 * 将 stub 模块注册到 require 缓存/全局，便于在无法改
 * metro.config.js 的场景下直接注入。返回是否注册成功。
 *
 * @returns {boolean} 是否成功注册
 */
function registerExpoStubs() {
  try {
    const Module = require('module');
    const originalResolve = Module._resolveFilename;

    const aliases = getExpoStubsAlias();

    // 包装模块解析，将 expo-* 模块重定向到 stub
    Module._resolveFilename = function (request, ...args) {
      if (aliases[request]) {
        return aliases[request];
      }
      return originalResolve.call(this, request, ...args);
    };

    return true;
  } catch {
    // 环境不支持（如非 Node 运行时），静默失败
    return false;
  }
}

module.exports = {
  registerExpoStubs,
  getExpoStubsAlias,
  getStubsDir,
};

/**
 * Metro resolver.alias 配置片段示例：
 *
 * ```js
 * // ExpoDemo/metro.config.js
 * const path = require('path');
 * const {
 *   getExpoStubsAlias,
 * } = require('react-native-harmony/ExpoStubs/registerExpoStubs');
 *
 * module.exports = {
 *   resolver: {
 *     alias: {
 *       ...getExpoStubsAlias(),
 *       // 可按需覆盖： 'expo-secure-store': '/abs/path/to/real/impl.js',
 *     },
 *   },
 * };
 * ```
 */
