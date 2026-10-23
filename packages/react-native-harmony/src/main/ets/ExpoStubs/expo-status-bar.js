/**
 * RNOH-New: Expo StatusBar Stub（转发实现）
 *
 * 将 expo-status-bar 的 API 转发到 React Native 的 StatusBar 组件。
 *
 * 注意：
 * - 需要 react-native 已安装（RNOH-New 应用必有）
 * - 若 react-native 不可用，则安全降级为空操作
 */

'use strict';

let StatusBar = null;
try {
  StatusBar = require('react-native').StatusBar;
} catch {
  StatusBar = null; // react-native 不可用，降级为空操作
}

/**
 * 设置状态栏样式
 * @param {string} [style] 'auto' | 'inverted' | 'light' | 'dark'
 */
function setStatusBarStyle(style) {
  if (!StatusBar) return;
  const barStyle =
    style === 'light'
      ? 'light-content'
      : style === 'dark'
        ? 'dark-content'
        : 'default';
  StatusBar.setBarStyle(barStyle, true);
}

/**
 * 设置状态栏隐藏
 * @param {boolean} hidden 是否隐藏
 * @param {string} [animation] 'none' | 'slide' | 'fade'
 */
function setStatusBarHidden(hidden, animation) {
  if (!StatusBar) return;
  StatusBar.setHidden(hidden, animation === 'fade' ? 'fade' : 'none');
}

/**
 * 设置状态栏背景色（仅 Android/HarmonyOS）
 * @param {string} backgroundColor 颜色值
 * @param {boolean} [animated] 是否动画过渡
 */
function setStatusBarBackgroundColor(backgroundColor, animated) {
  if (!StatusBar) return;
  StatusBar.setBackgroundColor(backgroundColor, animated !== false);
}

/**
 * 设置状态栏网络活动指示器（占位兼容，RN 未直接支持则忽略）
 * @param {boolean} [visible] 是否可见
 */
function setStatusBarNetworkActivityIndicatorVisible(visible) {
  void visible;
  // RN StatusBar 未提供对应 API，静默忽略
}

/**
 * 获取是否启用隐藏切换动画（占位）
 * @returns {boolean} 恒为 true（兼容调用方）
 */
function isStatusBarHidden() {
  return false;
}

// 兼容 expo-status-bar 的组件导出（Stub 场景下无组件渲染，导出空组件标记）
const StatusBarComponent = null;

module.exports = {
  setStatusBarStyle,
  setStatusBarHidden,
  setStatusBarBackgroundColor,
  setStatusBarNetworkActivityIndicatorVisible,
  isStatusBarHidden,
  StatusBar: StatusBarComponent,
};
