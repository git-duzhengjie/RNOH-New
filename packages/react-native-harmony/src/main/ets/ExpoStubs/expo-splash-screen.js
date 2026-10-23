/**
 * RNOH-New: Expo SplashScreen Stub（兜底实现）
 *
 * 当原生 expo-splash-screen 模块不可用时，
 * 提供空实现（返回已 resolve 的 Promise）。
 *
 * 注意：兜底环境下没有原生启动屏控制能力，
 * 这些调用会被安全忽略。
 */

'use strict';

/**
 * 阻止启动屏自动隐藏
 * @returns {Promise<void>}
 */
function preventAutoHideAsync() {
  return Promise.resolve();
}

/**
 * 隐藏启动屏
 * @returns {Promise<void>}
 */
function hideAsync() {
  return Promise.resolve();
}

/**
 * 设置启动屏配置（兼容性占位）
 * @param {Object} [options] 配置项
 * @returns {Promise<void>}
 */
function setOptions(options) {
  void options;
  return Promise.resolve();
}

module.exports = {
  preventAutoHideAsync,
  hideAsync,
  setOptions,
};
