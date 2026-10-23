# 贡献指南

感谢您对 **RNOH-New**（React Native for HarmonyOS）的关注与贡献！🎉

本文档描述了如何参与本项目开发，请在使用前阅读并遵循相关规范。

## 📁 项目结构

```text
RNOH-New/
├── packages/
│   ├── react-native-harmony/           # 核心引擎库（C++ + ArkTS）
│   │   ├── src/main/cpp/               # C++ 核心（Fabric、TurboModule、NAPI）
│   │   │   ├── RNH/                    # RNH 核心类（Instance/Mounting/Event 等）
│   │   │   ├── RNHCorePackage/         # 内置组件和模块
│   │   │   ├── third-party/            # 第三方依赖（ReactCommon、Hermes 等）
│   │   │   └── patches/                # React Native 补丁
│   │   └── src/main/ets/               # ArkTS 层（UI、Worker、Package）
│   ├── react-native-harmony-hvigor-plugin/  # hvigor 构建插件
│   └── create-rnh-app/                 # 项目脚手架工具（CLI）
├── examples/
│   └── HelloWorld/                     # 示例项目
├── docs/                               # 文档
├── scripts/                            # 构建脚本
├── .github/workflows/                  # GitHub Actions（CI / 发布）
└── package.json                        # Monorepo 根配置
```

## 🛠️ 开发环境搭建

### 前置要求

- **Node.js** ≥ 18
- **DevEco Studio** 5.0+（含 HarmonyOS SDK API 12+）
- **CMake** ≥ 3.13（用于 C++ 部分）
- **Git**

### 安装与构建

```bash
# 1. 克隆仓库
git clone https://github.com/git-duzhengjie/RNOH-New.git
cd RNOH-New

# 2. 安装依赖（workspace 统一管理）
npm install

# 3. 构建所有包
npm run build

# 4. 运行 lint / 测试（若有）
npm run lint
npm test
```

### 本地调试脚手架工具

```bash
cd packages/create-rnh-app
npm link

# 任意目录执行
create-rnh-app MyApp
```

## 📐 代码规范

### C++（核心引擎）

- 使用 **C++17** 标准
- 遵循已有头文件的接口签名，新增实现必须先有声明
- 每个方法添加中文注释，说明用途与关键逻辑
- 涉及引擎调用的未完成部分，用 `TODO` 标注并返回安全默认值（保证可编译）
- 命名：类用 `RNH` 前缀 + 大驼峰，成员变量用 `驼峰 + 下划线`，常量用大写下划线
- 多线程场景必须加锁，明确锁的粒度

### ArkTS（UI 层）

- 开启**严格模式**（`arkts-no-any-unknown`、`arkts-no-untyped-obj-literals` 等规则）
- 禁止使用 `any` / `unknown`，必须显式类型
- 方法覆盖使用 `override` 关键字
- 组件 Props 使用接口定义，不使用 Object 字面量

### TypeScript（工具链）

- 开启 **TS 严格模式**（`strict: true`）
- 禁止 `any`，禁止隐式 `any`
- 统一使用 `import type` 导入类型
- 提交前必须通过 `tsc --noEmit` 类型检查

## 📝 提交规范（Conventional Commits）

提交信息采用 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/) 规范：

```
<type>(<scope>): <subject>
```

### type 类型

| type | 用途 |
|------|------|
| `feat` | 新功能 |
| `fix` | 修复 Bug |
| `docs` | 文档变更 |
| `style` | 格式调整（不影响逻辑） |
| `refactor` | 重构（不新增功能不修 Bug） |
| `perf` | 性能优化 |
| `test` | 测试相关 |
| `build` | 构建系统或依赖变更 |
| `ci` | CI 配置变更 |
| `chore` | 杂项（版本、配置等） |

### 示例

```bash
feat(cpp): 实现 RNHMountingManager 的批量变更处理
fix(arkts): 修复 RNHSurface 在快速切换时的内存泄漏
docs(readme): 补充 create-rnh-app 使用说明
ci: 添加 npm 自动发布工作流
```

## 🔀 PR 流程

1. **Fork** 本仓库并克隆到本地
2. 创建特性分支：

   ```bash
   git checkout -b feat/your-feature
   ```

3. 编写代码，确保：
   - 通过本地构建（`npm run build`）
   - 通过类型检查与 lint（`npm run lint`、`tsc --noEmit`）
   - C++ 改动通过语法检查（可用 `g++ -std=c++17 -fsyntax-only`）
   - 新增/修改文件有对应中文注释
4. 提交（遵循 Conventional Commits），推送：

   ```bash
   git push origin feat/your-feature
   ```

5. 在 GitHub 上创建 **Pull Request** 到 `main` 分支，填写：
   - 变更内容摘要
   - 为什么需要这个变更
   - 测试/验证方式
   - 关联 Issue（如有）
6. 等待 CI 通过（`.github/workflows/ci.yml`）并接受 Review 反馈

### PR 合并约定

- 每个 PR 至少需要 **1 位维护者** Review 通过
- CI 必须全部通过
- 合并使用 `Squash and merge`，保持主干历史简洁

## 🐛 Issue 模板说明

提交 Issue 时请按模板填写（`.github/ISSUE_TEMPLATE/`），包括：

- **Bug 报告**：环境信息（DevEco 版本、SDK 版本、设备型号）、复现步骤、期望行为、实际行为、日志/截图
- **功能请求**：背景与动机、期望的 API 或行为、替代方案
- **构建问题**：完整报错输出、`npm -v` 与 `node -v` 输出

> 若仓库尚未添加模板文件，欢迎通过 PR 补充。

## 📦 发布流程（维护者）

打 tag 或推送版本变更后，由 `.github/workflows/publish.yml` 自动发布：

```bash
git tag v0.1.0
git push origin v0.1.0
```

三个包将依次发布到 npm：
- `@rnoh-new/react-native-harmony`
- `@rnoh-new/hvigor-plugin`
- `@rnoh-new/create-rnh-app`

## 📄 许可证

贡献即代表您同意将代码以 [MIT](../LICENSE) 许可证发布。
