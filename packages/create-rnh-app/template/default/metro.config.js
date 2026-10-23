/**
 * {{PROJECT_NAME}} - Metro configuration for RNOH-New
 */

const path = require('path');

/**
 * Metro configuration
 * https://facebook.github.io/metro/docs/configuration
 */
const config = {
  resolver: {
    // Allow resolving `.harmony.js` variants before `.js`
    sourceExts: ['harmony.js', 'js', 'jsx', 'ts', 'tsx', 'json', 'cjs', 'mjs'],
    assetExts: ['png', 'jpg', 'jpeg', 'gif', 'webp', 'svg', 'ttf', 'otf', 'woff', 'woff2', 'mp3', 'mp4'],
    // Ensure RNOH-New packages are resolved from the workspace
    nodeModulesPaths: [
      path.resolve(__dirname, 'node_modules'),
    ],
    extraNodeModules: {
      '@rnoh-new/react-native-harmony': path.resolve(__dirname, 'node_modules', '@rnoh-new', 'react-native-harmony'),
      '@rnoh-new/hvigor-plugin': path.resolve(__dirname, 'node_modules', '@rnoh-new', 'hvigor-plugin'),
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
  ],
};

module.exports = config;
