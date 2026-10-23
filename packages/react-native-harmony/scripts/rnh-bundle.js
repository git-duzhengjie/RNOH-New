/**
 * RNOH-New: rnh-bundle.js
 *
 * Bundles the React Native app for HarmonyOS using Metro.
 */

import path from 'path';
import { spawnSync } from 'child_process';
import chalk from 'chalk';

/**
 * Resolve the React Native CLI bundle script path.
 * Prefers the local project copy, falls back to @react-native-community/cli.
 */
function resolveReactNativeCli() {
  const localCli = path.resolve(process.cwd(), 'node_modules', 'react-native', 'cli.js');
  const communityCli = path.resolve(process.cwd(), 'node_modules', '@react-native-community', 'cli', 'build', 'bin.js');

  try {
    // ESM import.meta.resolve can be used, but require.resolve is simpler for files.
    return require.resolve(localCli);
  } catch {
    // fallthrough
  }

  try {
    return require.resolve(communityCli);
  } catch {
    throw new Error(
      'Cannot find React Native CLI. Make sure "react-native" or "@react-native-community/cli" is installed.'
    );
  }
}

/**
 * Run the bundle command.
 */
function runBundle(options) {
  const {
    entryFile,
    bundleOutput,
    assetsDest,
    platform,
    dev,
    minify,
  } = options;

  console.log(chalk.blue('Bundling React Native app for HarmonyOS...'));
  console.log(chalk.gray(`  entry: ${entryFile}`));
  console.log(chalk.gray(`  output: ${bundleOutput}`));
  console.log(chalk.gray(`  assets: ${assetsDest}`));
  console.log(chalk.gray(`  dev: ${dev}`));

  const reactNativeBundlePath = resolveReactNativeCli();
  const args = [
    reactNativeBundlePath,
    'bundle',
    '--platform', platform,
    '--dev', dev ? 'true' : 'false',
    '--minify', minify ? 'true' : 'false',
    '--entry-file', path.resolve(entryFile),
    '--bundle-output', path.resolve(bundleOutput),
    '--assets-dest', path.resolve(assetsDest),
  ];

  const result = spawnSync('node', args, {
    stdio: 'inherit',
    cwd: process.cwd(),
  });

  if (result.error) {
    console.error(chalk.red(`Bundle failed: ${result.error.message}`));
    process.exit(1);
  }

  if (result.status !== 0) {
    console.error(chalk.red(`Bundle exited with code ${result.status}`));
    process.exit(result.status ?? 1);
  }

  console.log(chalk.green('Bundle completed successfully.'));
}

export default runBundle;
