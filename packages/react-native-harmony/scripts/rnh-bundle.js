/**
 * RNOH-New: rnh-bundle.js
 *
 * Bundles the React Native app for HarmonyOS using Metro.
 */

import path from 'path';
import fs from 'fs';
import { spawnSync } from 'child_process';
import { createRequire } from 'module';
import chalk from 'chalk';

const require = createRequire(import.meta.url);

/**
 * Find the monorepo / project root by walking up from cwd.
 * Looks for node_modules/react-native/cli.js or node_modules/@react-native-community/cli.
 */
function findProjectRoot(startDir) {
  let dir = path.resolve(startDir);
  const { root } = path.parse(dir);

  while (dir !== root) {
    const rnCli = path.join(dir, 'node_modules', 'react-native', 'cli.js');
    const communityCli = path.join(dir, 'node_modules', '@react-native-community', 'cli', 'build', 'bin.js');

    if (fs.existsSync(rnCli) || fs.existsSync(communityCli)) {
      return dir;
    }
    dir = path.dirname(dir);
  }

  // Fallback: return startDir
  return path.resolve(startDir);
}

/**
 * Resolve the React Native CLI bundle script path.
 * Prefers the local project copy, falls back to @react-native-community/cli.
 */
function resolveReactNativeCli() {
  const projectRoot = findProjectRoot(process.cwd());
  const localCli = path.resolve(projectRoot, 'node_modules', 'react-native', 'cli.js');
  const communityCli = path.resolve(projectRoot, 'node_modules', '@react-native-community', 'cli', 'build', 'bin.js');

  try {
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

  // Use the current working directory (where react-native.config.js lives)
  const cwd = process.cwd();

  // Use `npx react-native bundle` which auto-loads CLI plugins.
  // This avoids manual plugin resolution issues in monorepos.
  const args = [
    'react-native',
    'bundle',
    '--platform', platform,
    '--dev', dev ? 'true' : 'false',
    '--minify', minify ? 'true' : 'false',
    '--entry-file', path.resolve(entryFile),
    '--bundle-output', path.resolve(bundleOutput),
    '--assets-dest', path.resolve(assetsDest),
  ];

  const result = spawnSync('npx', args, {
    stdio: 'inherit',
    cwd,
    shell: true,
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
