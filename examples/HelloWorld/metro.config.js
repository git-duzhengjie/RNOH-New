/**
 * HelloWorld - Metro configuration for RNOH-New
 */

const path = require('path');

/**
 * Metro configuration
 * https://facebook.github.io/metro/docs/configuration
 */
const config = {
  resolver: {
    // RN 0.88 的 CLI 会校验 platform 是否在 resolver.platforms 白名单中，
    // 需显式声明 out-of-tree 平台 harmony
    platforms: ['harmony', 'ios', 'android', 'windows', 'web'],
    // Allow resolving `.harmony.js` variants before `.js`
    sourceExts: ['harmony.js', 'js', 'jsx', 'ts', 'tsx', 'json', 'cjs', 'mjs'],
    assetExts: ['png', 'jpg', 'jpeg', 'gif', 'webp', 'svg', 'ttf', 'otf', 'woff', 'woff2', 'mp3', 'mp4'],
    // Ensure RNOH-New packages are resolved from the workspace
    // 注意：显式设置会覆盖 Metro 默认的祖先目录查找，
    // 必须包含 monorepo 根 node_modules（react-native 被 hoist 到这里）
    nodeModulesPaths: [
      path.resolve(__dirname, 'node_modules'),
      path.resolve(__dirname, '..', '..', 'node_modules'),
      path.resolve(__dirname, '..', '..', 'packages'),
    ],
    extraNodeModules: {
      '@rnoh-new/react-native-harmony': path.resolve(__dirname, '..', '..', 'packages', 'react-native-harmony'),
      '@rnoh-new/hvigor-plugin': path.resolve(__dirname, '..', '..', 'packages', 'react-native-harmony-hvigor-plugin'),
    },
  },
  transformer: {
    // Asset plugins can be added here if needed
    assetPlugins: [],
    // Babel transformer for TypeScript / HarmonyOS specific transforms
    babelTransformerPath: require.resolve('@react-native/metro-babel-transformer'),
  },
  server: {
    // Port for Metro bundler
    port: 8081,
    // Enhance console output with RN-specific errors
    enhanceMiddleware: (middleware) => middleware,
  },
  // Watch additional folders for monorepo workspace
  watchFolders: [
    path.resolve(__dirname),
    // RN 0.88 的 react-native 等依赖被 hoist 到 monorepo 根，
    // 必须纳入 watchFolders 否则 Metro 无法访问
    path.resolve(__dirname, '..', '..', 'node_modules'),
    path.resolve(__dirname, '..', '..', 'packages'),
  ],
};

module.exports = config;
