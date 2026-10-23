/**
 * RNOH-New: Expo Haptics Stub（兜底实现）
 *
 * 当原生 expo-haptics 模块不可用时，
 * 提供空实现（返回已 resolve 的 Promise）。
 *
 * 所有 API 均兼容 expo-haptics 官方签名。
 */

'use strict';

// 兼容 expo-haptics 的冲击样式枚举
const ImpactFeedbackStyle = {
  Light: 'light',
  Medium: 'medium',
  Heavy: 'heavy',
  Soft: 'soft',
  Rigid: 'rigid',
};

// 兼容 expo-haptics 的通知类型枚举
const NotificationFeedbackType = {
  Success: 'success',
  Warning: 'warning',
  Error: 'error',
};

/**
 * 触发一次冲击反馈
 * @param {string} [style] 冲击样式
 * @returns {Promise<void>}
 */
function impactAsync(style) {
  void style;
  return Promise.resolve();
}

/**
 * 触发一次通知反馈
 * @param {string} [type] 通知类型
 * @returns {Promise<void>}
 */
function notificationAsync(type) {
  void type;
  return Promise.resolve();
}

/**
 * 触发一次选择反馈
 * @returns {Promise<void>}
 */
function selectionAsync() {
  return Promise.resolve();
}

module.exports = {
  impactAsync,
  notificationAsync,
  selectionAsync,
  ImpactFeedbackStyle,
  NotificationFeedbackType,
};
