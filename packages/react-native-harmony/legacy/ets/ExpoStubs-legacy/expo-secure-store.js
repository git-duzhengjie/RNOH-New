/**
 * RNOH-New: Expo SecureStore Stub（兜底实现）
 *
 * 当原生 expo-secure-store 模块不可用时，
 * 基于 AsyncStorage 提供简易的键值存储能力。
 *
 * 注意：
 * - 这是降级实现，数据不加密，不应存储敏感凭据
 * - 兼容 expo-secure-store 的 Promise API
 */

'use strict';

/**
 * 尝试加载 RN 的 AsyncStorage。
 * 支持不同版本的路径：
 * 1. @react-native-async-storage/async-storage
 * 2. react-native 内置
 * 3. 完全不可用时使用内存 Map 兜底
 */
let storage = null;
let usingFallback = false;

try {
  // 优先尝试社区版 AsyncStorage
  storage = require('@react-native-async-storage/async-storage');
} catch {
  try {
    // 尝试 react-native 内置（部分 RN 版本可用）
    const RN = require('react-native');
    if (RN.AsyncStorage) {
      storage = RN.AsyncStorage;
    }
  } catch {
    // 完全不可用：内存 Map 兜底（重启即失）
    usingFallback = true;
    const memoryStore = new Map();
    storage = {
      getItem: async (key) => memoryStore.get(key) ?? null,
      setItem: async (key, value) => memoryStore.set(key, String(value)),
      removeItem: async (key) => memoryStore.delete(key),
    };
  }
}

/**
 * 读取指定 key 的值
 *
 * @param {string} key 键名
 * @param {Object} [options] 选项（兼容 expo-secure-store API）
 * @returns {Promise<string|null>} 值，不存在返回 null
 */
function getItemAsync(key, options) {
  if (typeof key !== 'string' || key.length === 0) {
    return Promise.reject(new Error('getItemAsync: key 必须是非空字符串'));
  }
  void options; // 兼容占位
  return storage.getItem(key);
}

/**
 * 写入指定 key 的值
 *
 * @param {string} key 键名
 * @param {string} value 值
 * @param {Object} [options] 选项（兼容 expo-secure-store API）
 * @returns {Promise<void>}
 */
function setItemAsync(key, value, options) {
  if (typeof key !== 'string' || key.length === 0) {
    return Promise.reject(new Error('setItemAsync: key 必须是非空字符串'));
  }
  if (typeof value !== 'string') {
    return Promise.reject(new Error('setItemAsync: value 必须是字符串'));
  }
  void options; // 兼容占位
  return storage.setItem(key, value);
}

/**
 * 删除指定 key 的值
 *
 * @param {string} key 键名
 * @param {Object} [options] 选项（兼容 expo-secure-store API）
 * @returns {Promise<void>}
 */
function deleteItemAsync(key, options) {
  if (typeof key !== 'string' || key.length === 0) {
    return Promise.reject(new Error('deleteItemAsync: key 必须是非空字符串'));
  }
  void options; // 兼容占位
  return storage.removeItem(key);
}

module.exports = {
  getItemAsync,
  setItemAsync,
  deleteItemAsync,
  /** 是否使用了内存兜底（无 AsyncStorage 环境） */
  isUsingFallback: () => usingFallback,
};
