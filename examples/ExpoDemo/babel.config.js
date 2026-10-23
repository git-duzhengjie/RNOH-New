/**
 * ExpoDemo - Babel configuration
 *
 * Uses babel-preset-expo (Expo official preset) which internally loads
 * @react-native/babel-preset with the Hermes parser (Flow syntax support).
 */

const path = require('path');

module.exports = {
  presets: [
    [path.resolve(__dirname, '..', '..', 'node_modules', 'babel-preset-expo'), {}],
  ],
  plugins: [],
};
