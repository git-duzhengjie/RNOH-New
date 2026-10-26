/**
 * RNOH-New: rnh-codegen.js
 *
 * Runs codegen and autolinking for RNOH-New.
 */

import path from 'path';
import chalk from 'chalk';

/**
 * Run the codegen command.
 */
async function runCodegen(options) {
  const { projectRoot, rnohModulePath } = options;

  console.log(chalk.blue('Running RNOH-New codegen...'));
  console.log(chalk.gray(`  project root: ${projectRoot}`));
  console.log(chalk.gray(`  rnoh module: ${rnohModulePath}`));

  try {
    const resolvedProjectRoot = path.resolve(projectRoot);
    const resolvedRnohPath = path.resolve(rnohModulePath);

    // Try to use the hvigor-plugin's codegen helpers if available
    let plugin;
    try {
      plugin = await import('@rnoh-new/hvigor-plugin');
    } catch {
      const fallbackPath = path.resolve(resolvedRnohPath, '..', 'react-native-harmony-hvigor-plugin', 'src', 'index.ts');
      plugin = await import(fallbackPath);
    }

    const pluginModule = plugin.default ?? plugin;

    if (pluginModule.generateAutolinking) {
      pluginModule.generateAutolinking(resolvedProjectRoot, resolvedRnohPath);
      console.log(chalk.green('Autolinking generated successfully.'));
    } else {
      console.log(chalk.yellow('generateAutolinking not exposed by @rnoh-new/hvigor-plugin, skipping.'));
    }

    if (pluginModule.runCodegen) {
      pluginModule.runCodegen(resolvedProjectRoot, resolvedRnohPath);
      console.log(chalk.green('Codegen completed successfully.'));
    } else {
      console.log(chalk.yellow('runCodegen not exposed by @rnoh-new/hvigor-plugin, skipping.'));
    }
  } catch (e) {
    console.error(chalk.red(`Codegen failed: ${e.message}`));
    process.exit(1);
  }

  console.log(chalk.green('RNOH-New codegen finished.'));
}

export default runCodegen;
