# @rnoh-new/create-rnh-app

**RNOH-New 项目脚手架工具** 🚀

通过一条命令快速创建一个可运行的 **React Native for HarmonyOS** 项目，内置 RNOH-New 核心引擎、hvigor 构建配置与最小示例代码。

## ✨ 特性

- ⚡ 一条命令创建新项目
- 🎨 支持 TypeScript / JavaScript 模板
- 🧩 内置 RNOH-New 核心引擎配置
- 📦 自动安装依赖（可选）
- 🌱 自动初始化 Git 仓库（可选）

## 📦 安装

`create-rnh-app` 推荐通过 `npx` 直接使用，无需全局安装：

```bash
npx @rnoh-new/create-rnh-app MyApp
```

也可以全局安装后使用：

```bash
npm install -g @rnoh-new/create-rnh-app

create-rnh-app MyApp
```

## 🚀 使用

### 基本用法

```bash
npx @rnoh-new/create-rnh-app MyApp
```

该命令会：

1. 在当前目录下创建 `MyApp/` 文件夹
2. 从内置模板复制项目骨架
3. （默认）初始化 Git 仓库
4. （默认）自动安装 npm 依赖
5. 输出后续操作指引

### 创建后启动

```bash
cd MyApp

# 启动 Metro
npm start

# 在 DevEco Studio 中打开 harmony/ 目录，构建并运行到鸿蒙设备
```

## ⚙️ 参数说明

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `--template <template>` | `string` | `default` | 使用的项目模板名称，如 `default`、`typescript`、`bare` |
| `--skip-install` | `boolean` | `false` | 跳过依赖安装 |
| `--no-git` | `boolean` | `false` | 不初始化 Git 仓库 |
| `--typescript` | `boolean` | `false` | 使用 TypeScript 模板（等价于 `--template typescript`） |
| `-h, --help` | — | — | 查看帮助信息 |
| `-V, --version` | — | — | 查看版本号 |

### 示例

```bash
# 使用 TypeScript 模板创建项目，不安装依赖、不初始化 Git
npx @rnoh-new/create-rnh-app MyApp --typescript --skip-install --no-git

# 使用指定模板
npx @rnoh-new/create-rnh-app MyApp --template bare
```

## 📁 生成的目录结构

```text
MyApp/
├── harmony/                      # HarmonyOS 工程（DevEco Studio 打开）
│   ├── entry/                    # 应用入口模块
│   │   ├── src/main/ets/         # ArkTS 源码
│   │   └── oh-package.json5
│   ├── build-profile.json5
│   ├── hvigorfile.ts
│   └── oh-package.json5
├── src/                          # React Native JS/TS 源码
│   ├── App.tsx                   # 应用根组件
│   └── index.js / index.ts       # 入口文件
├── babel.config.js
├── metro.config.js
├── package.json
├── tsconfig.json                 # （使用 TypeScript 模板时）
└── .gitignore
```

## ❓ 常见问题（FAQ）

### 1. 提示 `npx @rnoh-new/create-rnh-app` 找不到？

- 请确认 Node.js 版本 ≥ 18：`node -v`
- 尝试清除 npx 缓存后重试：

  ```bash
  npx clear-npx-cache
  npx @rnoh-new/create-rnh-app MyApp
  ```

### 2. 创建过程中依赖安装失败？

- 检查网络与 npm registry 配置：`npm config get registry`
- 可先跳过安装，再手动安装：

  ```bash
  npx @rnoh-new/create-rnh-app MyApp --skip-install
  cd MyApp
  npm install
  ```

### 3. 生成的工程在 DevEco Studio 中打不开？

- 请使用 DevEco Studio 5.0+，并确保 HarmonyOS SDK API 12+ 已安装
- 首次打开 `harmony/` 目录后，等待 Gradle/hvigor 同步完成

### 4. `npm start` 后页面白屏？

- 确认 Metro 已正常启动（终端显示 `Metro waiting on port 8081`）
- 确认鸿蒙模拟器/真机与开发机处于同一网络
- 检查 `metro.config.js` 中的 server 配置

### 5. 如何升级到最新模板？

目前脚手架模板随包版本发布，可通过重新创建项目或手动合并更新。后续版本将支持 `--update` 增量升级。

## 🤝 参与贡献

欢迎提交 Issue 和 Pull Request，详见根目录 [CONTRIBUTING.md](../../CONTRIBUTING.md)。

## 📄 许可证

[MIT](../../LICENSE)
