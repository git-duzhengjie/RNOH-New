/**
 * ExpoDemo - Metro configuration for RNOH-New
 */

const path = require('path');

/**
 * ExpoStubs directory: expo module stubs for HarmonyOS fallback.
 */
const expoStubsDir = path.resolve(
  __dirname,
  '..',
  '..',
  'packages',
  'react-native-harmony',
  'src',
  'main',
  'ets',
  'ExpoStubs'
);

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
      path.resolve(__dirname, '..', '..', 'packages'),
    ],
    extraNodeModules: {
      '@rnoh-new/react-native-harmony': path.resolve(__dirname, '..', '..', 'packages', 'react-native-harmony'),
      '@rnoh-new/hvigor-plugin': path.resolve(__dirname, '..', '..', 'packages', 'react-native-harmony-hvigor-plugin'),
    },
    // Expo module aliases → RNOH-New stubs (HarmonyOS fallback)
    alias: {
      'expo-secure-store': path.resolve(expoStubsDir, 'expo-secure-store.js'),
      'expo-crypto': path.resolve(expoStubsDir, 'expo-crypto.js'),
      'expo-haptics': path.resolve(expoStubsDir, 'expo-haptics.js'),
      'expo-splash-screen': path.resolve(expoStubsDir, 'expo-splash-screen.js'),
    },
  },
  transformer: {
    // Asset plugins can be added here if needed
    assetPlugins: [],
    // Babel transformer for TypeScript / HarmonyOS specific transforms
    // Resolve from community-cli-plugin's nested node_modules
    babelTransformerPath: require.resolve(path.resolve(__dirname, '..', '..', 'node_modules', '@react-native', 'community-cli-plugin', 'node_modules', 'metro-babel-transformer')),
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
    path.resolve(__dirname, '..', '..', 'packages'),
  ],
};

module.exports = config;
