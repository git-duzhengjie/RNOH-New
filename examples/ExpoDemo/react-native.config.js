/**
 * ExpoDemo - React Native CLI configuration
 *
 * Registers the community CLI plugin for bundle/start commands.
 */

const path = require('path');

// Resolve community-cli-plugin from monorepo root node_modules
// Note: Directly require dist/index.js to bypass Node 24 exports field issues
const pluginPath = path.resolve(__dirname, '..', '..', 'node_modules', '@react-native', 'community-cli-plugin', 'dist', 'index.js');
const plugin = require(pluginPath);

module.exports = {
  commands: [
    plugin.bundleCommand,
    plugin.startCommand,
  ],
};
