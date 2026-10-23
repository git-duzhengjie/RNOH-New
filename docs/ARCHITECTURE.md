# RNOH-New 架构文档

## 目录

1. [整体架构](#整体架构)
2. [核心概念](#核心概念)
3. [C++ 引擎](#c-引擎)
4. [ArkTS 层](#arkts-层)
5. [通信机制](#通信机制)
6. [Fabric 集成](#fabric-集成)
7. [TurboModule 系统](#turbomodule-系统)

---

## 整体架构

RNOH-New 采用分层架构设计，从下到上分为：

1. **HarmonyOS ArkUI 层**：原生 UI 组件
2. **C++ 核心引擎层**：Fabric 渲染器、TurboModule、NAPI 桥接
3. **ArkTS 中间层**：Worker 线程、事件处理、Package 管理
4. **JS 应用层**：React Native 应用代码

## 核心概念

### RNHInstance

一个 `RNHInstance` 对应一个独立的 React Native 运行时环境，包含：

- JS 引擎（Hermes）
- Fabric 渲染器
- TurboModule 注册表
- 事件分发器

一个应用可以有多个 `RNHInstance`（多 Surface 场景）。

### RNHSurface

一个 `RNHSurface` 对应一个根 React 组件树，管理：

- Surface 生命周期
- 布局尺寸
- 触摸事件

### RNHPackage

一个 `RNHPackage` 是一组相关的 TurboModule 和组件的集合，类似 React Native Android 的 `ReactPackage`。

## C++ 引擎

### 目录结构

```
src/main/cpp/
├── RNH/                          # 核心类
│   ├── RNHInstance.h             # 实例管理
│   ├── RNHMountingManager.h      # Fabric Mounting
│   ├── RNHComponentInstance.h    # 组件实例基类
│   ├── RNHArkUINode.h            # ArkUI 节点封装
│   ├── RNHTurboModuleProvider.h  # TurboModule 提供者
│   ├── RNHPackage.h              # Package 基类
│   ├── RNHNapiBridge.h           # NAPI 桥接
│   └── RNHEventDispatcher.h      # 事件分发
├── RNHCorePackage/               # 核心包
│   ├── ComponentInstances/       # 内置组件实例
│   ├── TurboModules/             # 内置 TurboModule
│   └── ComponentDescriptors/     # 组件描述符
├── third-party/                  # 第三方依赖
│   ├── rn/ReactCommon/           # React Native 核心
│   ├── hermes/                   # Hermes JS 引擎
│   ├── folly/                    # Facebook Folly
│   ├── boost/                    # Boost 库
│   └── ...
└── patches/                      # React Native 补丁
    └── react_native_core/        # 核心补丁
```

### 关键类说明

#### RNHInstance

负责管理一个 RN 运行时的完整生命周期：

```cpp
auto instance = rnoh_new::createRNHInstance({
    .bundlePath = "bundle.js",
    .enableHermes = true,
    .enableFabric = true,
    .enableTurboModules = true,
});

instance->loadBundle("bundle.js");
instance->startSurface(0, "MyApp", 375, 812, 3.0);
```

#### RNHMountingManager

接收 Fabric 的 ShadowTree 变更指令，应用到 ArkUI：

```cpp
void processMutations(const std::vector<RNHMutation>& mutations) {
    for (const auto& mutation : mutations) {
        switch (mutation.type) {
            case RNHMutationType::CREATE:
                handleCreate(mutation);
                break;
            case RNHMutationType::UPDATE_PROPS:
                handleUpdateProps(mutation);
                break;
            // ...
        }
    }
}
```

## ArkTS 层

### 目录结构

```
src/main/ets/
├── RNH/                          # 核心类
│   ├── RNHApp.ets                # 应用组件
│   ├── RNHSurface.ets            # Surface 组件
│   ├── RNHAbility.ets            # Ability 基类
│   ├── RNHWorker.ets             # JS Worker
│   ├── RNHInstance.ets           # 实例管理（TS 侧）
│   ├── RNHTurboModule.ets        # TurboModule 基类
│   ├── RNHPackage.ets            # Package 基类
│   ├── RNHEventEmitter.ets       # 事件发射器
│   ├── RNHLogger.ets             # 日志工具
│   └── RNHError.ets              # 错误类型
└── RNHCorePackage/               # 核心包
    ├── components/               # 内置组件
    └── turboModules/             # 内置 TurboModule
```

### 关键类说明

#### RNHApp

顶层应用组件，使用方式：

```typescript
@Entry
@Component
struct Index {
  build() {
    RNHApp({
      config: {
        moduleName: 'MyApp',
        bundlePath: 'bundle.js',
      }
    })
  }
}
```

#### RNHAbility

Ability 基类，使用方式：

```typescript
export default class EntryAbility extends RNHAbility {
  override getPagePath() {
    return 'pages/Index';
  }
}
```

## 通信机制

### JS → Native

1. JS 调用 TurboModule 方法
2. 通过 JSI 调用到 C++ `RNHTurboModule`
3. C++ 通过 NAPI 调用到 ArkTS `RNHTurboModule`
4. ArkTS 执行原生逻辑

### Native → JS

1. ArkTS 调用 `RNHEventEmitter.emit()`
2. 通过 NAPI 传递到 C++
3. C++ 通过 JSI 调用 JS 回调
4. JS 执行事件处理

## Fabric 集成

RNOH-New 完全支持 React Native 新架构（Fabric）：

1. **ComponentDescriptor**：描述组件的 props、events、state
2. **ComponentInstance**：原生组件实例，对应 ArkUI 节点
3. **MountingManager**：处理 ShadowTree 变更
4. **Yoga Layout**：使用 Yoga 引擎计算布局

## TurboModule 系统

TurboModule 是 JS 与原生代码通信的桥梁：

1. **类型安全**：使用 Codegen 生成类型定义
2. **惰性加载**：只在首次调用时初始化
3. **Bridgeless**：不依赖传统的 Bridge

### 创建 TurboModule

详见 [README.md](../README.md#添加自定义-turbomodule)

---

更多文档请查看 `docs/` 目录。
