# RNOH-New

**React Native for HarmonyOS** - 一个现代化的 React Native → HarmonyOS 桥接框架

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-HarmonyOS-blue.svg)](https://www.harmonyos.com/)
[![React Native](https://img.shields.io/badge/React%20Native-0.79+-green.svg)](https://reactnative.dev/)

## 🎯 项目目标

RNOH-New 是一个全新的开源项目，旨在让 React Native 应用能够无缝运行在 HarmonyOS 上。本项目基于 [RNOH](https://gitcode.com/CPF-RN/ohos_react_native) 的设计经验，完全重写核心引擎，摆脱历史包袱。

### 核心特性

- ✅ **Fabric 渲染器**：支持 React Native 新架构（Fabric + TurboModule）
- ✅ **Hermes 引擎**：高性能 JS 引擎
- ✅ **Bridgeless 模式**：无桥通信，性能更优
- ✅ **ArkUI 原生**：使用 HarmonyOS ArkUI 作为渲染后端
- ✅ **Expo 兼容**：支持 Expo 项目直接打包
- ✅ **Monorepo**：统一管理核心引擎、工具链、示例项目

## 📁 项目结构

```
RNOH-New/
├── packages/
│   ├── react-native-harmony/          # 核心运行时库（自建编译）
│   │   ├── src/main/cpp/              # C++ 核心（Fabric、TurboModule、NAPI）
│   │   │   ├── RNOH/                  # 适配层核心（ArkUI、JSI、TaskExecutor/uv）
│   │   │   ├── RNOHCorePackage/       # 内置组件与 TurboModule
│   │   │   ├── patches/               # ReactCommon 补丁头
│   │   │   └── third-party/           # 第三方源码（不入库，脚本获取）
│   │   ├── src/main/ets/              # ArkTS 层（RNApp、RNSurface、Worker、Package）
│   │   │   ├── RNOH/                  # 运行时 ArkTS 实现
│   │   │   └── RNOHCorePackage/       # 核心包（组件/模块描述符）
│   │   ├── libs/                      # 原生库产物（编译后发布，不入库）
│   │   └── legacy/                    # 早期自研实现（已归档，不参与构建）
│   ├── react-native-harmony-hvigor-plugin/  # hvigor 构建插件
│   └── create-rnh-app/                # 项目脚手架工具（create-rnh-app CLI）
├── examples/
│   └── HelloWorld/                    # 示例项目
├── docs/                              # 文档
└── scripts/                           # 构建脚本
```

## 🚀 快速开始

### 前置要求

- **HarmonyOS SDK**: API 12+ (HarmonyOS 5.0.0+)
- **DevEco Studio**: 5.0+
- **Node.js**: 18+
- **React Native**: 0.79+

### 安装

```bash
# 克隆仓库
git clone https://github.com/git-duzhengjie/RNOH-New.git
cd RNOH-New

# 安装依赖
npm install

# 构建核心库
npm run build
```

### 原生构建（自建运行时）

核心运行时由本仓库**自行编译**：Hermes 使用官方 prebuilt `libhermesvm.so`，其余（ReactCommon / Folly /
glog / fmt / boost / double-conversion / fast_float / RNOH / RNOHCorePackage）全部从源码构建。

```powershell
# 1. 同步第三方源码（约 790MB，不入库）
powershell -ExecutionPolicy Bypass -File scripts/vendor-thirdparty.ps1

# 2. 编译原生库并产出 HAR
cd packages
$env:DEVECO_SDK_HOME='D:\Program Files\Huawei\DevEco Studio\sdk'
hvigorw --mode module -p module=react_native_harmony@default -p product=default assembleHar --no-daemon

# 3. 发布原生库到包目录（hvigor 从依赖包的 libs/<arch>/ 收集 .so）
powershell -ExecutionPolicy Bypass -File scripts/publish-libs.ps1
powershell -ExecutionPolicy Bypass -File scripts/add-ffrt-lib.ps1

# 4. 构建示例 HAP
cd ../examples/HelloWorld/harmony
hvigorw --mode module -p product=default assembleHap --no-daemon
```

> 详细步骤、依赖要求与常见坑位见 [docs/BUILD-NATIVE.md](docs/BUILD-NATIVE.md)。

### 创建新项目

```bash
# 使用脚手架创建新项目
npx @rnoh-new/create-rnh-app MyApp
cd MyApp

# 启动 Metro
npm start

# 构建鸿蒙应用（在 DevEco Studio 中打开 harmony/ 目录）
```

脚手架常用参数：

```bash
# 使用 TypeScript 模板
npx @rnoh-new/create-rnh-app MyApp --typescript

# 跳过依赖安装 / 不初始化 Git
npx @rnoh-new/create-rnh-app MyApp --skip-install --no-git
```

> 完整参数说明与常见问题，见 [packages/create-rnh-app/README.md](packages/create-rnh-app/README.md)

### 集成到现有 Expo 项目

```bash
# 安装 RNOH-New
npm install @rnoh-new/react-native-harmony

# 生成鸿蒙工程（待实现）
npx rnh-new init
```

## 🏗️ 架构设计

### 整体架构

```
┌─────────────────────────────────────────┐
│         React Native App (JS)           │
├─────────────────────────────────────────┤
│      Metro Bundler (JS Bundle)          │
├─────────────────────────────────────────┤
│  RNOH-New JS Layer (Worker Thread)      │
│  - JS Bundle Execution                  │
│  - TurboModule JS Proxy                 │
│  - Event Emitter                        │
├─────────────────────────────────────────┤
│  NAPI Bridge (C++ <-> ArkTS)            │
├─────────────────────────────────────────┤
│  RNOH-New C++ Core (Main Thread)        │
│  - RNHInstance (实例管理)                │
│  - RNHMountingManager (Fabric Mounting) │
│  - RNHTurboModuleProvider (TurboModule) │
│  - RNHArkUINode (ArkUI 节点抽象)         │
│  - RNHEventDispatcher (事件分发)         │
├─────────────────────────────────────────┤
│  React Native Core (ReactCommon)        │
│  - Fabric Renderer                      │
│  - TurboModule Registry                 │
│  - Yoga Layout Engine                   │
├─────────────────────────────────────────┤
│  HarmonyOS ArkUI (Native UI)            │
└─────────────────────────────────────────┘
```

### 核心组件

#### C++ 层 (`packages/react-native-harmony/src/main/cpp/`)

- **RNHInstance**: 管理一个 RN 运行时实例的生命周期
- **RNHMountingManager**: 处理 Fabric 的 ShadowTree 变更，应用到 ArkUI
- **RNHComponentInstance**: 表示一个原生组件实例（View、Text 等）
- **RNHArkUINode**: ArkUI 节点的 C++ 封装
- **RNHTurboModule**: TurboModule 基类
- **RNHPackage**: Package 基类（类似 ReactPackage）
- **RNHNapiBridge**: NAPI 桥接层

#### ArkTS 层 (`packages/react-native-harmony/src/main/ets/`)

- **RNHApp**: 顶层应用组件
- **RNHSurface**: RN 根视图容器
- **RNHAbility**: Ability 基类
- **RNHWorker**: JS 执行 Worker
- **RNHTurboModule**: TurboModule ArkTS 基类
- **RNHPackage**: Package ArkTS 基类

## 📖 开发指南

### 添加自定义 TurboModule

1. **C++ 侧**（继承 `RNHTurboModule`）：

```cpp
#include "RNH/RNHTurboModuleProvider.h"

class MyTurboModule : public rnoh_new::RNHTurboModule {
public:
    std::string getName() const override {
        return "MyModule";
    }

    void myMethod() {
        // 实现
    }
};
```

2. **ArkTS 侧**（继承 `RNHTurboModule`）：

```typescript
import { RNHTurboModule, RNHTurboModuleContext } from '@rnoh-new/react-native-harmony';

export class MyTurboModule extends RNHTurboModule {
  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'MyModule';
  }

  myMethod(): void {
    // ArkTS 实现
  }
}
```

3. **注册到 Package**：

```typescript
export class MyPackage extends RNHPackage {
  override getTurboModuleFactoryMap(): Map<string, () => Object> {
    const map = new Map();
    map.set('MyModule', () => new MyTurboModule(this.context));
    return map;
  }
}
```

### 添加自定义组件

（待补充）

## 🛣️ 路线图

- [x] **Phase 1**: 项目初始化和架构设计 ✅
- [ ] **Phase 2**: 核心引擎实现（C++ Fabric + NAPI）
- [ ] **Phase 3**: 基础组件支持（View、Text、Image、ScrollView）
- [ ] **Phase 4**: TurboModule 系统
- [ ] **Phase 5**: hvigor 构建插件
- [ ] **Phase 6**: CLI 工具（create-rnh-app）
- [ ] **Phase 7**: Expo 兼容性
- [ ] **Phase 8**: 性能优化和稳定性
- [ ] **Phase 9**: 文档和社区建设

## 🤝 贡献指南

欢迎贡献代码！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 🙏 致谢

- [RNOH](https://gitcode.com/CPF-RN/ohos_react_native) - 原 RNOH 项目，提供了宝贵的设计经验
- [React Native](https://reactnative.dev/) - React Native 框架
- [HarmonyOS](https://www.harmonyos.com/) - HarmonyOS 平台
- [我的助手](https://my-assis.com) - 智能助手，协助本项目的开发与构建调试

## 📮 联系方式

- **GitHub Issues**: [https://github.com/git-duzhengjie/RNOH-New/issues](https://github.com/git-duzhengjie/RNOH-New/issues)
- **讨论区**: [https://github.com/git-duzhengjie/RNOH-New/discussions](https://github.com/git-duzhengjie/RNOH-New/discussions)

---

**⚠️ 注意**: 本项目处于早期开发阶段，API 可能会发生变化。生产环境使用请谨慎。
