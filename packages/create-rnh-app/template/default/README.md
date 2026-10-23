# {{PROJECT_NAME}}

React Native app running on HarmonyOS, powered by [RNOH-New](https://github.com/git-duzhengjie/RNOH-New).

> 本项目由 `create-rnh-app` 生成。以下占位符已在生成时替换：
> `{{PROJECT_NAME}}` → 项目名 · `{{PROJECT_SLUG}}` → slug · `{{BUNDLE_ID}}` → bundle id · `{{PROJECT_VERSION}}` → 版本号

## 环境要求

| 工具 | 版本 | 说明 |
|------|------|------|
| Node.js | >= 18 | 运行 `rnh-new doctor` 可自动检查 |
| DevEco Studio | 5.0+ | 用于编译 HarmonyOS 工程 |
| HarmonyOS SDK | 5.0.0(12)+ | 随 DevEco Studio 安装 |
| hvigorw | 随工程提供 | `harmony/hvigorw` |

## 快速开始

```bash
# 1. 安装依赖
npm install

# 2. 检查环境（可选）
npm run doctor

# 3. 启动 Metro 开发服务器（热更新开发模式）
npm start

# 4. 生成 HarmonyOS bundle（打包模式）
npm run bundle:harmony

# 5. 运行 codegen（生成原生桥接代码）
npm run codegen

# 6. 编译 HarmonyOS 应用
npm run build:harmony
# 或使用 DevEco Studio 打开 harmony/ 目录直接构建运行
```

## 目录结构

```
{{PROJECT_SLUG}}/
├── src/                    # React Native 业务代码
│   ├── App.tsx             # 根组件
│   └── index.js            # RN 入口（注册组件）
├── harmony/                # HarmonyOS 原生工程
│   ├── AppScope/           # 应用级配置
│   ├── entry/              # entry 模块
│   │   └── src/main/ets/   # ArkTS 代码（EntryAbility、pages/Index）
│   └── hvigorfile.ts       # 构建脚本（createRNHProjectPlugin）
├── metro.config.js         # Metro 配置
├── babel.config.js         # Babel 配置
├── tsconfig.json           # TypeScript 配置
└── package.json
```

## 常用脚本

| 命令 | 作用 |
|------|------|
| `npm start` | 启动 Metro 开发服务器 |
| `npm run bundle:harmony` | 打包 JS bundle 到 rawfile |
| `npm run codegen` | 生成 TurboModule 桥接代码 |
| `npm run doctor` | 检查开发环境 |
| `npm run build:harmony` | 编译 HAP 包 |

## 开发模式 vs 打包模式

- **开发模式**：`npm start` 启动 Metro，`RNHApp` 的 `metroUrl` 配置为开发服务器地址，支持热更新。
- **打包模式**：`npm run bundle:harmony` 生成 `bundle.harmony.js` 放入 `harmony/entry/src/main/resources/rawfile/`，随 HAP 一起发布。

## 许可证

MIT
