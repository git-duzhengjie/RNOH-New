/**
 * ExpoDemo - Metro configuration for RNOH-New
 *
 * Built on @expo/metro-config (synchronous getDefaultConfig) which already
 * includes the React Native defaults. Configured for an npm-workspaces
 * monorepo: projectRoot is this example, the monorepo root (with hoisted
 * node_modules) is added to watchFolders + nodeModulesPaths.
 *
 * Adds:
 *  - `.harmony.js` source extension
 *  - Expo module aliases → RNOH-New ExpoStubs (HarmonyOS fallback)
 */

const path = require('path');
const { getDefaultConfig } = require('@expo/metro-config');

const projectRoot = __dirname;
const monorepoRoot = path.resolve(projectRoot, '..', '..');

const expoStubsDir = path.resolve(
  monorepoRoot,
  'packages',
  'react-native-harmony',
  'src',
  'main',
  'ets',
  'ExpoStubs'
);

const config = getDefaultConfig(projectRoot);

// Resolve `.harmony.js` variants before plain `.js`
config.resolver.sourceExts = ['harmony.js', ...config.resolver.sourceExts];

// Resolve hoisted dependencies from the monorepo root node_modules
config.resolver.nodeModulesPaths = [
  path.resolve(projectRoot, 'node_modules'),
  path.resolve(monorepoRoot, 'node_modules'),
];

// Watch the monorepo root so hoisted node_modules are available
config.watchFolders = [monorepoRoot];

// Expo module aliases → RNOH-New HarmonyOS stubs
config.resolver.alias = {
  'expo-secure-store': path.resolve(expoStubsDir, 'expo-secure-store.js'),
  'expo-crypto': path.resolve(expoStubsDir, 'expo-crypto.js'),
  'expo-haptics': path.resolve(expoStubsDir, 'expo-haptics.js'),
  'expo-splash-screen': path.resolve(expoStubsDir, 'expo-splash-screen.js'),
};

module.exports = config;
