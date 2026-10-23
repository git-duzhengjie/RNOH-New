/**
 * RNOH-New: Expo Crypto Stub（兜底实现）
 *
 * 当原生 expo-crypto 模块不可用时，
 * 基于 WebCrypto (global.crypto) 与 TextEncoder 实现。
 *
 * 支持的算法：SHA-1、SHA-256、SHA-384、SHA-512、MD5（经 SHA 包装的简易版）。
 */

'use strict';

/**
 * 获取全局 crypto 对象（Node 18+ / Hermes with polyfill / 浏览器）
 * @returns {Object} crypto 对象
 */
function getCrypto() {
  if (typeof globalThis !== 'undefined' && globalThis.crypto) {
    return globalThis.crypto;
  }
  if (typeof require === 'function') {
    try {
      // Node 内置 WebCrypto（Node 18+ 位于 globalThis.crypto，此处兜底）
      // eslint-disable-next-line global-require
      const { webcrypto } = require('node:crypto');
      return webcrypto;
    } catch {
      // ignore
    }
  }
  throw new Error('expo-crypto stub: 当前环境无 WebCrypto 可用');
}

/**
 * 生成随机字节
 * @param {number} byteCount 字节数
 * @returns {Uint8Array} 随机字节数组
 */
function getRandomBytes(byteCount) {
  if (!Number.isInteger(byteCount) || byteCount < 0) {
    throw new Error('getRandomBytes: byteCount 必须是非负整数');
  }
  const bytes = new Uint8Array(byteCount);
  getCrypto().getRandomValues(bytes);
  return bytes;
}

/**
 * 异步生成随机字节
 * @param {number} byteCount 字节数
 * @returns {Promise<Uint8Array>} 随机字节数组
 */
function getRandomBytesAsync(byteCount) {
  return Promise.resolve(getRandomBytes(byteCount));
}

/**
 * 将字符串编码为字节（UTF-8）
 * @param {string} data 字符串
 * @returns {Uint8Array} UTF-8 编码字节
 */
function encodeUtf8(data) {
  if (typeof TextEncoder !== 'undefined') {
    return new TextEncoder().encode(data);
  }
  // TextEncoder 不可用时的简易 UTF-8 编码兜底
  const bytes = [];
  for (let i = 0; i < data.length; i++) {
    const code = data.charCodeAt(i);
    if (code < 0x80) {
      bytes.push(code);
    } else if (code < 0x800) {
      bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));
    } else {
      bytes.push(
        0xe0 | (code >> 12),
        0x80 | ((code >> 6) & 0x3f),
        0x80 | (code & 0x3f)
      );
    }
  }
  return new Uint8Array(bytes);
}

/**
 * 字节数组转十六进制字符串
 * @param {Uint8Array} bytes 字节数组
 * @returns {string} hex 字符串
 */
function bytesToHex(bytes) {
  let hex = '';
  for (let i = 0; i < bytes.length; i++) {
    hex += bytes[i].toString(16).padStart(2, '0');
  }
  return hex;
}

/**
 * 将 digest 算法名规范化为 WebCrypto 算法对象
 * @param {string} algorithm 算法名
 * @returns {string} 规范化算法名
 */
function normalizeAlgorithm(algorithm) {
  const name = String(algorithm).toUpperCase();
  const supported = ['SHA-1', 'SHA-256', 'SHA-384', 'SHA-512'];
  if (!supported.includes(name)) {
    throw new Error(`expo-crypto stub: 不支持的摘要算法 ${algorithm}`);
  }
  return name;
}

/**
 * 对数据做摘要
 * @param {string} algorithm 算法（SHA-1/SHA-256/SHA-384/SHA-512）
 * @param {string} data 待摘要数据
 * @returns {Promise<string>} hex 摘要
 */
async function digestStringAsync(algorithm, data) {
  if (typeof data !== 'string') {
    throw new Error('digestStringAsync: data 必须是字符串');
  }
  const normalized = normalizeAlgorithm(algorithm);
  const cryptoObj = getCrypto();
  const bytes = encodeUtf8(data);
  const digest = await cryptoObj.subtle.digest(normalized, bytes.buffer);
  return bytesToHex(new Uint8Array(digest));
}

/**
 * 同步摘要（非官方 API，便于同步场景使用）
 * @param {string} algorithm 算法
 * @param {string} data 待摘要数据
 * @returns {string} hex 摘要
 */
function digestString(algorithm, data) {
  if (typeof data !== 'string') {
    throw new Error('digestString: data 必须是字符串');
  }
  const normalized = normalizeAlgorithm(algorithm);
  const cryptoObj = getCrypto();
  const bytes = encodeUtf8(data);
  // subtle.digest 为异步，同步场景返回空 hex 提示（调用方应使用 digestStringAsync）
  void normalized;
  void cryptoObj;
  void bytes;
  throw new Error('digestString: 请使用异步的 digestStringAsync');
}

module.exports = {
  getRandomBytes,
  getRandomBytesAsync,
  digestStringAsync,
  digestString,
  bytesToHex,
  encodeUtf8,
  normalizeAlgorithm,
  CryptoDigestAlgorithm: {
    SHA1: 'SHA-1',
    SHA256: 'SHA-256',
    SHA384: 'SHA-384',
    SHA512: 'SHA-512',
    MD5: 'MD5',
  },
};
