# @rnoh-new/hvigor-plugin

RNOH-New 项目级 Hvigor 插件，用于 HarmonyOS 应用构建前的自动化准备步骤：

- 读取并注入 `.env` 环境变量
- 调用 Metro 打包 React Native JS bundle
- 调用 autolinking 生成 C++ / ETS 包注册表

---

## 安装

```bash
npm install @rnoh-new/hvigor-plugin --save-dev
```

---

## 用法

### 1. 在 `hvigorfile.ts` 中引入并注册插件

```typescript
import { createRNHProjectPlugin } from '@rnoh-new/hvigor-plugin';

export default {
  plugins: [
    createRNHProjectPlugin({
      nodeModulesPath: './node_modules',
      envFile: 'harmony/.env',
      buildMode: 'debug',
      bundler: {
        config: './metro.config.js',
        entryFile: 'index.js',
        bundleOutput: './harmony/entry/src/main/resources/rawfile/bundle.harmony.js',
        assetsDest: './harmony/entry/src/main/resources/rawfile/',
      },
    }),
  ],
};
```

### 2. 配置 `.env` 文件（可选）

```text
# harmony/.env
RNH_BUNDLE_NAME=com.example.rnohtest
RNH_APP_NAME=RNOHNewDemo
```

---

## API

### `createRNHProjectPlugin(options: RNHProjectPluginOptions): HvigorPlugin`

创建项目级插件。

#### `RNHProjectPluginOptions`

| 字段 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `nodeModulesPath` | `string` | 是 | - | `node_modules` 路径，用于 autolinking |
| `bundler.config` | `string` | 是 | - | Metro 配置文件路径 |
| `bundler.bundleOutput` | `string` | 是 | - | bundle 输出路径 |
| `bundler.assetsDest` | `string` | 是 | - | 静态资源输出目录 |
| `bundler.entryFile` | `string` | 否 | `index.js` | 入口文件 |
| `envFile` | `string` | 否 | `harmony/.env` | 环境变量文件路径 |
| `buildMode` | `'debug' \| 'release'` | 否 | `'debug'` | 构建模式 |

### `createRNHModulePlugin(options: RNHModulePluginOptions)`

创建模块级插件。在模块的 `beforeNodeBuild` 阶段：
- 检查 autolinking 生成文件是否存在，不存在则生成
- 执行 codegen（若配置）
- 将 autolinking.cmake include 到当前模块的 CMakeLists.txt

#### `RNHModulePluginOptions`

| 字段 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| `nodeModulesPath` | `string` | 是 | - | `node_modules` 目录路径 |
| `rnohModulePath` | `string` | 否 | 自动解析 | `@rnoh-new/react-native-harmony` 模块路径 |
| `codegen.projectRootPath` | `string` | 否 | - | codegen 项目根路径 |
| `codegen.rnohModulePath` | `string` | 否 | - | codegen 输出模块路径 |
| `autolinking.enabled` | `boolean` | 否 | `true` | 是否启用 autolinking 注入 |
| `autolinking.outputDir` | `string` | 否 | - | autolinking 输出目录 |

模块级插件用法示例：

```typescript
import { createRNHModulePlugin } from '@rnoh-new/hvigor-plugin';

export default {
  plugins: [
    createRNHModulePlugin({
      nodeModulesPath: '../../node_modules',
      rnohModulePath: '../../node_modules/@rnoh-new/react-native-harmony',
      codegen: {
        projectRootPath: '../../',
        rnohModulePath: '../../node_modules/@rnoh-new/react-native-harmony',
      },
      autolinking: {
        enabled: true,
        outputDir: 'src/main',
      },
    }),
  ],
};
```

## 构建

```bash
npm run clean
npm run build
```

编译产物输出到 `dist/` 目录。

---

## 插件执行流程

1. `beforeNodeBuild` 钩子触发
2. 读取 `envFile` 并注入 `process.env`
3. 调用 Metro bundler 生成 `bundle.harmony.js`
4. 调用 autolinking 脚本生成：
   - `src/main/cpp/RNHAutolinking/PackageProvider.h`
   - `src/main/cpp/RNHAutolinking/PackageProvider.cpp`
   - `src/main/cpp/RNHAutolinking/autolinking.cmake`
   - `src/main/ets/Autolinking/RNHPackageProvider.ets`

---

## 注意事项

- 本插件使用 `child_process.spawnSync` 调用外部命令，请确保 Node.js 版本 >= 18。
- 需要安装 `@react-native-community/cli` 和 `metro` 才能执行 bundle 命令。
