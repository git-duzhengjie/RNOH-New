/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

const babel = require('@babel/core');
const crypto = require('crypto');

const ARKTS_DECORATORS = [
  '@Sendable',
  '@Component',
  '@Entry',
  '@Builder',
  '@CustomDialog',
  '@State',
  '@Prop',
  '@Require',
  '@BuilderParam',
  '@Styles',
  '@Watch',
  '@Provide',
  '@Consume',
  '@ObjectLink',
  '@StorageLink',
  '@StorageProp',
  '@LocalStorageLink',
  '@LocalStorageProp',
  '@Preview',
  '@Reusable',
  '@Declarative',
];

function stripDecorators(sourceText) {
  let processed = sourceText;
  for (const decorator of ARKTS_DECORATORS) {
    const escaped = decorator.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    const regex = new RegExp(`^\\s*${escaped}\\s*$`, 'gm');
    processed = processed.replace(regex, '');
  }
  return processed;
}

module.exports = {
  process(sourceText, sourcePath, transformOptions) {
    const processed = stripDecorators(sourceText);
    const result = babel.transformSync(processed, {
      filename: sourcePath.replace(/\.ets$/, '.ts'),
      presets: [
        ['@babel/preset-env', { targets: { node: 'current' }, modules: 'commonjs' }],
        ['@babel/preset-typescript', { allowDeclareFields: true }],
      ],
      babelrc: false,
      configFile: false,
      compact: false,
    });
    return { code: result.code, map: result.map };
  },
  getCacheKey(sourceText, sourcePath, transformOptions) {
    const processed = stripDecorators(sourceText);
    return crypto.createHash('sha1')
      .update('ets-transformer:')
      .update(sourcePath)
      .update(':')
      .update(processed)
      .update(':')
      .update(JSON.stringify(transformOptions || {}))
      .digest('hex');
  },
};
