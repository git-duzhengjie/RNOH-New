#!/usr/bin/env node

/**
 * RNOH-New CLI entry point
 *
 * Provides commands for bundling, codegen and environment diagnostics.
 */

import path from 'path';
import { fileURLToPath } from 'url';
import { program } from 'commander';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const packageJson = await import(path.resolve(__dirname, '../package.json'), {
  assert: { type: 'json' },
});

program
  .name('rnh-new')
  .description('CLI for RNOH-New - React Native for HarmonyOS')
  .version(packageJson.default.version);

program
  .command('bundle')
  .description('Bundle the React Native app for HarmonyOS')
  .requiredOption('--entry-file <path>', 'Path to the JS entry file')
  .requiredOption('--bundle-output <path>', 'Path for the output bundle')
  .requiredOption('--assets-dest <path>', 'Path for the output assets')
  .option('--platform <platform>', 'Target platform', 'harmony')
  .option('--dev', 'Build in development mode', false)
  .option('--minify', 'Minify the bundle', false)
  .action(async (options) => {
    const { default: runBundle } = await import('../scripts/rnh-bundle.js');
    runBundle(options);
  });

program
  .command('codegen')
  .description('Run codegen and autolinking for RNOH-New')
  .requiredOption('--project-root <path>', 'Root of the React Native project')
  .requiredOption('--rnoh-module-path <path>', 'Path to the react-native-harmony module')
  .action(async (options) => {
    const { default: runCodegen } = await import('../scripts/rnh-codegen.js');
    runCodegen(options);
  });

program
  .command('doctor')
  .description('Check the development environment for RNOH-New')
  .action(async () => {
    const { default: runDoctor } = await import('../scripts/rnh-doctor.js');
    runDoctor();
  });

program.parse();
