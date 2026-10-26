/**
 * HelloWorld harmony bundle 打包脚本
 *
 * 直接用 metro + metro-config 打包，不依赖 @react-native/metro-config。
 */

const path = require('path');
const { loadConfig, getDefaultConfig, mergeConfig } = require('metro-config');
const Metro = require('metro');

const projectRoot = path.resolve(__dirname, '..', 'examples', 'HelloWorld');
const outFile = path.join(
  projectRoot,
  'harmony',
  'entry',
  'src',
  'main',
  'resources',
  'rawfile',
  'bundle.harmony.js'
);

async function main() {
  const defaults = await getDefaultConfig(projectRoot);
  const custom = require(path.join(projectRoot, 'metro.config.js'));
  const config = mergeConfig(defaults, custom);

  config.projectRoot = projectRoot;
  const rootNodeModules = path.resolve(__dirname, '..', 'node_modules');
  config.watchFolders = [
    projectRoot,
    rootNodeModules,
    path.resolve(projectRoot, '..', '..', 'packages'),
  ];
  config.resolver.platforms = ['harmony', 'native'];
  config.resolver.disableHierarchicalLookup = false;
  config.resolver.nodeModulesPaths = [
    rootNodeModules,
    path.join(projectRoot, 'node_modules'),
    path.resolve(projectRoot, '..', '..', 'packages'),
  ];
  config.transformer.babelTransformerPath = require.resolve(
    '@react-native/metro-babel-transformer'
  );

  await Metro.runBuild(config, {
    entry: path.join(projectRoot, 'src', 'index.js'),
    platform: 'harmony',
    dev: false,
    minify: false,
    out: outFile,
    transformOptions: { enableBabelRuntime: true },
  });

  console.log('BUNDLE OK ->', outFile);
}

main().catch((err) => {
  console.error('BUNDLE FAILED:', err && err.message ? err.message : err);
  if (err && err.stack) {
    console.error(err.stack);
  }
  process.exit(1);
});
