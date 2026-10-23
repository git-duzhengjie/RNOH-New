/**
 * ExpoDemo - Babel configuration
 */

const path = require('path');

module.exports = {
  presets: [
    [path.resolve(__dirname, '..', '..', 'node_modules', 'babel-preset-expo', 'node_modules', '@react-native', 'babel-preset'), {
      // React Native preset defaults
    }],
  ],
  plugins: [],
};
