# Expo 兼容性指南

本文档说明如何将现有 Expo 项目移植到鸿蒙（HarmonyOS）平台，基于 RNOH-New 桥接框架。

---

## 目录

- [快速开始](#快速开始)
- [步骤一：安装 @rnoh-new/react-native-harmony](#步骤一安装-rnoh-newreact-native-harmony)
- [步骤二：配置 Metro alias 到 ExpoStubs](#步骤二配置-metro-alias-到-expostubs)
- [支持的模块列表](#支持的模块列表)
- [已知限制](#已知限制)
- [示例项目](#示例项目)

---

## 快速开始

```bash
# 1. 在 Expo 项目根目录安装 RNOH-New
npm install @rnoh-new/react-native-harmony @rnoh-new/hvigor-plugin

# 2. 配置 metro.config.js（见下文）
# 3. 打包鸿蒙 bundle
npx rnh-new bundle \
  --platform harmony \
  --entry-file ./src/index.js \
  --bundle-output ./harmony/entry/src/main/resources/rawfile/bundle.harmony.js \
  --assets-dest ./harmony/entry/src/main/resources/rawfile/assets

# 4. 构建 HAP
cd harmony && hvigorw assembleHap
```

---

## 步骤一：安装 @rnoh-new/react-native-harmony

在项目根目录执行：

```bash
npm install @rnoh-new/react-native-harmony @rnoh-new/hvigor-plugin
```

安装完成后，RNOH-New 的 CLI 工具 `rnh-new` 即可使用：

```bash
npx rnh-new doctor   # 检查开发环境
npx rnh-new bundle   # 打包 JS bundle
```

---

## 步骤二：配置 Metro alias 到 ExpoStubs

Expo 的许多模块（如 `expo-secure-store`、`expo-crypto`）依赖原生代码，在鸿蒙平台上暂不可用。RNOH-New 提供了 **ExpoStubs** 兜底实现，通过 Metro 的 `resolver.alias` 配置，将这些模块的请求重定向到 stub 文件。

在 `metro.config.js` 中添加：

```javascript
const path = require('path');

const expoStubsDir = path.resolve(
  __dirname,
  'node_modules',
  '@rnoh-new',
  'react-native-harmony',
  'src',
  'main',
  'ets',
  'ExpoStubs'
);

const config = {
  resolver: {
    alias: {
      'expo-secure-store': path.resolve(expoStubsDir, 'expo-secure-store.js'),
      'expo-crypto': path.resolve(expoStubsDir, 'expo-crypto.js'),
      'expo-haptics': path.resolve(expoStubsDir, 'expo-haptics.js'),
      'expo-splash-screen': path.resolve(expoStubsDir, 'expo-splash-screen.js'),
    },
  },
};

module.exports = config;
```

> **提示**：在 monorepo（如 RNOH-New 自身）中，`expoStubsDir` 的路径可能需要调整为相对 workspace 根目录的路径。

---

## 支持的模块列表

| Expo 模块 | 支持状态 | 说明 |
|-----------|----------|------|
| `expo-constants` | ✅ 支持 | 通过 RNOH-New DeviceInfo TurboModule 提供原生数据 |
| `expo-file-system` | ✅ 支持 | 通过 RNOH-New FileSystem TurboModule 提供原生文件读写 |
| `expo-font` | ✅ 支持 | 通过 RNOH-New 资源系统加载字体 |
| `expo-linking` | ✅ 支持 | 通过 RNOH-New Linking TurboModule 提供 URL 跳转 |
| `expo-secure-store` | ⚠️ Stub | 基于 AsyncStorage 的降级实现，**不加密** |
| `expo-crypto` | ⚠️ Stub | 基于 WebCrypto 的实现，支持 SHA-1/256/384/512 |
| `expo-haptics` | ⚠️ Stub | 空实现（返回 resolved Promise），无实际震动 |
| `expo-splash-screen` | ⚠️ Stub | 空实现（返回 resolved Promise），无原生启动屏控制 |

### 各 stub 详细说明

#### expo-secure-store（Stub）

- **实现原理**：优先使用 `@react-native-async-storage/async-storage`；若无则使用 RN 内置 AsyncStorage；最终降级为内存 Map
- **兼容性**：完全兼容 `setItemAsync` / `getItemAsync` / `deleteItemAsync` API
- **安全警告**：数据不加密，**不应存储敏感凭据**（生产环境请等待原生模块支持）

#### expo-crypto（Stub）

- **实现原理**：基于 `globalThis.crypto`（WebCrypto）与 `TextEncoder`
- **支持算法**：SHA-1、SHA-256、SHA-384、SHA-512
- **兼容性**：兼容 `getRandomBytes` / `getRandomBytesAsync` / `digestStringAsync` API
- **限制**：`digestString`（同步版本）会抛出错误，请使用 `digestStringAsync`

#### expo-haptics（Stub）

- **实现原理**：空实现，所有方法返回 `Promise.resolve()`
- **兼容性**：兼容 `impactAsync` / `notificationAsync` / `selectionAsync` API
- **限制**：无实际震动反馈

#### expo-splash-screen（Stub）

- **实现原理**：空实现，所有方法返回 `Promise.resolve()`
- **兼容性**：兼容 `preventAutoHideAsync` / `hideAsync` / `setOptions` API
- **限制**：无原生启动屏控制能力

---

## 已知限制

以下 Expo 模块**暂不支持**在鸿蒙平台上使用：

| 模块 | 原因 | 替代方案 |
|------|------|----------|
| `expo-router` | 依赖 React Navigation 深度集成，暂不支持 | 使用 `@react-navigation/native` 直接配置 |
| `expo-camera` | 需要原生相机模块 | 使用鸿蒙原生相机 API 或等待 RNOH-New 支持 |
| `expo-location` | 需要原生定位模块 | 使用鸿蒙原生定位 API 或等待 RNOH-New 支持 |
| `expo-notifications` | 需要原生推送模块 | 使用鸿蒙原生推送 API 或等待 RNOH-New 支持 |
| `expo-media-library` | 需要原生媒体库模块 | 使用鸿蒙原生媒体 API 或等待 RNOH-New 支持 |
| `expo-av` | 需要原生音视频模块 | 使用鸿蒙原生 AVPlayer 或等待 RNOH-New 支持 |
| `expo-sqlite` | 需要原生 SQLite 模块 | 使用鸿蒙原生关系型数据库或等待 RNOH-New 支持 |
| `expo-barcode-generator` | 需要原生绘图模块 | 使用纯 JS 条码生成库（如 `jsbarcode`） |

> **注意**：任何依赖原生代码的 Expo 模块，在未提供 stub 或原生实现的情况下，都无法在鸿蒙平台上运行。

---

## 示例项目

完整的 Expo 兼容性示例请参见：

```
examples/ExpoDemo/
├── package.json          # 依赖声明（expo、expo-constants、expo-file-system、expo-asset、expo-secure-store）
├── metro.config.js       # Metro 配置（含 resolver.alias → ExpoStubs）
├── babel.config.js       # Babel 配置
├── tsconfig.json         # TypeScript 配置
└── src/
    ├── index.js          # RN 入口
    └── App.tsx           # 演示 4 个 Expo 模块的用法
```

### 运行示例

```bash
cd examples/ExpoDemo

# 安装依赖（在 monorepo 根目录执行 npm install 即可）
npm install

# 打包鸿蒙 bundle
npx rnh-new bundle \
  --platform harmony \
  --entry-file ./src/index.js \
  --bundle-output ./dist/bundle.harmony.js \
  --assets-dest ./dist/assets

# 或使用 package.json 中的脚本
npm run bundle:harmony
```

---

## 常见问题

### Q: 为什么我的 Expo 模块在鸿蒙上报错？

A: 请检查该模块是否在 [支持的模块列表](#支持的模块列表) 中。如果不在列表中，说明该模块暂不支持鸿蒙平台，需要：
1. 寻找纯 JS 替代方案
2. 等待 RNOH-New 提供原生实现
3. 自行编写 stub 或原生模块

### Q: 如何自行编写 Expo 模块 stub？

A: 参考 `packages/react-native-harmony/src/main/ets/ExpoStubs/` 目录下的现有 stub 实现，编写一个兼容目标模块 API 的 JS 文件，然后在 `metro.config.js` 的 `resolver.alias` 中注册即可。

### Q: Stub 实现和原生实现有什么区别？

A: Stub 是纯 JS 降级实现，功能可能受限（如 expo-secure-store 不加密）。原生实现通过 TurboModule 调用鸿蒙系统能力，功能完整。RNOH-New 会逐步将高频 stub 替换为原生实现。

---

## 反馈与贡献

如果在移植过程中遇到问题，欢迎提交 Issue：

- GitHub: https://github.com/git-duzhengjie/RNOH-New/issues
- 标签：`expo-compat`

---

*最后更新：2026-10-23*
