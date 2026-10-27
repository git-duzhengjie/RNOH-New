# React Native 0.86 → 0.88 升级记录

> 本文记录 RNOH-New 从 React Native 0.86 升级到 **0.88.0** 的完整改动、踩坑与复现步骤。
> 原生源码基线（`src/main/cpp/third-party/rn/ReactCommon`）由 `node_modules/react-native/ReactCommon` 替换而来，**不入库**；
> 因此对 vendor 源码的补丁在本文第 3 节逐条列出，重新同步基线后需按此重放。

## 1. 升级范围

| 项目 | 升级前 | 升级后 |
|---|---|---|
| react-native | 0.86.0 | **0.88.0** |
| react | 19.2.x | **19.3.1** |
| Expo SDK | 53 | **58**（`expo@58.0.19`） |
| Node 要求 | `>=20.19.4` | **`^22.13.0 \|\| ^24.3.0`** |
| CI Node 矩阵 | `[20, 22]` | **`[22, 24]`** |
| 原生 ReactCommon 基线 | RNOH 0.84.5 | **RN 0.88 官方 `ReactCommon`** |

## 2. JS 侧改动

- 根 `package.json`：`@react-native/metro-babel-transformer` → `^0.88.0`，新增 `@react-native/metro-config ^0.88.0`，`engines.node` 同步。
- `examples/HelloWorld`、`packages/create-rnh-app/template/default/tpl-package.json`：`react-native 0.88.0`、`react ^19.3.0`、`@types/react ^19.1.1`、`@react-native/babel-preset ^0.88.0`。
- `examples/ExpoDemo`：`expo ~58.0.19`、`expo-asset ~58.0.18`、`expo-constants ~58.0.13`、`expo-file-system ~58.0.11`、`expo-secure-store ~58.0.2`、`babel-preset-expo ~58.0.20`、`@expo/metro-config ~58.0.17`、`@react-native-community/cli ^20.2.0`。
  - `App.tsx`：SDK 58 起 `documentDirectory` / `cacheDirectory` 移入 `expo-file-system/legacy`，导入路径相应调整。
- `examples/HelloWorld/metro.config.js`（RN 0.88 CLI 收紧后必须的三项）：
  1. `resolver.platforms` 显式包含 `'harmony'`（否则 `Invalid platform "harmony"`）；
  2. `resolver.nodeModulesPaths` 补 monorepo 根 `node_modules`（显式设置会覆盖默认的祖先查找）；
  3. `watchFolders` 补 monorepo 根 `node_modules`（否则 Metro 无法访问被 hoist 的 react-native）。
  打包命令建议带 `--reset-cache`。
- `.github/workflows/ci.yml`：Node 矩阵 `[20, 22]` → `[22, 24]`。

## 3. 原生侧改动

### 3.1 基线替换

```powershell
# 备份旧基线（third-party 已 gitignore）
Rename-Item third-party/rn/ReactCommon third-party/rn/ReactCommon-0.84.5
robocopy node_modules/react-native/ReactCommon third-party/rn/ReactCommon /E
```

### 3.2 `third-party/rn/CMakeLists.txt`（vendor，需重放）

| 改动 | 原因 |
|---|---|
| 新增 `REACT_ANDROID_DIR` 指向 `patches/react_android` | 0.88 的 `jsinspector-modern/cdp` 会 include Android 专用 `SoMerging-utils.cmake`（已提供 no-op 版） |
| `add_react_common_subdir(react/cxxstableapi)`（在 oscompat 之前） | 0.88 新增 INTERFACE 头文件库，未注册则 `PrivateGuard.h` 找不到 |
| `jsinspector` 目标加 `-include patches/.../compat/ranges_empty_compat.h` | 鸿蒙 libc++ 未实现 `std::ranges::empty` |
| 新增 6 个 subdir：`react/renderer/bridging`、`css`、`animationbackend`、`viewtransition`、`observers/resize`、`animated`、`nativemodule/resizeobserver`、`nativemodule/viewtransition` | 0.88 新增模块，不注册会出现 `AnimatedModule` / `ViewTransitionModule` 等 undefined symbol |
| `reactnative` 目标追加 5 个 `$<TARGET_OBJECTS:...>`，`REACTNATIVE_LIBRARIES` 追加 `react_renderer_viewtransition` | 同上，符号链接 |

### 3.3 其他 vendor 补丁（`third-party/rn/ReactCommon/**`，需重放）

| 文件 | 改动 | 原因 |
|---|---|---|
| `cmake-utils/react-native-flags.cmake` | `-Werror` → `-Wno-error` | 0.88 默认 `-Werror`，会把适配层 reorder/unused-lambda/pedantic 等告警升级为错误 |
| `react/renderer/core/graphicsConversions.h` | `std::format` → `fmt::format`（含 include） | 鸿蒙 libc++ 无 `std::format` |
| `react/renderer/core/Props.h` | `rawProps` + `initializeDynamicProps` 从 `#ifdef RN_SERIALIZABLE_STATE` 中解耦；`folly/dynamic.h` 改为无条件 include | 鸿蒙组件依赖 `props->rawProps` 读取原始 props |
| `react/renderer/core/Props.cpp` | 同上，补一份非宏分支的 `initializeDynamicProps` 实现 | 同上 |
| `react/renderer/core/ConcreteComponentDescriptor.h` | 非宏分支用 `requires` 检测后调用 `initializeDynamicProps` | `RootShadowNode` 等类型没有该方法 |
| `react/renderer/uimanager/UIManager.h` / `.cpp` | 补回 RNOH 的 `UIManagerNodeOperationHook` 注册与三个回调调用点（`createNode` / `cloneNode` / `appendChild`） | 0.84.5 中由 RNOH 打进源码，0.88 官方无此机制；`TextStorageProvider` 并行文本测量依赖它 |

> ⚠️ 不要全局定义 `RN_SERIALIZABLE_STATE`：它会激活大量 Android 专用代码（`fbjni`、`toAndroidRepr` 等），鸿蒙无法编译。

### 3.4 `patches/` 目录新增（入库）

| 文件 | 作用 |
|---|---|
| `patches/react_android/src/main/jni/first-party/jni-lib-merge/SoMerging-utils.cmake` | no-op 的 `target_merge_so`，满足 cdp 的 include |
| `patches/react_native_core/compat/ranges_empty_compat.h` | 补 `std::ranges::empty` |
| `patches/react_native_core/react/renderer/uimanager/UIManagerNodeOperationHook.h` | 回补 NodeOperationHook 接口 |
| `patches/.../FBReactNativeSpec/FBReactNativeSpecJSI.h` | 按 0.88 重建：88 个 FeatureFlag（85 bool + 3 double）+ 新增 `NativeResizeObserverCxxSpec` / `NativeViewTransitionCxxSpec`（后者由 `@react-native/codegen` 生成后注入） |

### 3.5 RNOH 适配层修复（`src/main/cpp/RNOH`、`RNOHCorePackage`）

| 文件 | 改动 |
|---|---|
| `FallbackComponentInstance.h` | `{x, y}` → 显式 `static_cast<Float>` |
| `CppComponentInstance.h` | `needsOffscreenAlphaCompositing` 改用 traits + `if constexpr`（0.88 移入 Android 专用 `HostPlatformViewProps`） |
| `ArkUITypography.h` | 4 处窄化显式 `static_cast<facebook::react::Float>` |
| `TextStorageBuilder.h` | `m_maxFontScale` 改读 `m_baseTextAttributes.maxFontSizeMultiplier` |
| `TextConversions.cpp` | `adjustsFontSizeToFit` / `minimumFontScale` 改读 `props->paragraphAttributes.*` |
| `TextStorageProvider.h/.cpp` | `getYGDirection()` → `getLayoutMetrics().layoutDirection`，`emplaceBuilder` 参数类型改 `LayoutDirection` |
| `TypographyStyleWrapper.cpp` | 删除 `paragraphAttributes.writingDirection` 分支（0.88 已移除该字段） |
| `StyledStringWrapper.cpp` | `maxFontSizeMultiplier` 改取首个 fragment 的 `TextAttributes` |
| `SchedulerDelegate.h/.cpp` | 实现 0.88 新增 4 个纯虚接口（`schedulerShouldMergeReactRevision` 等，no-op）；`pullTransaction` lambda 参数改 `const&`；`MountingTransaction` 不可拷贝 → 改为拷贝 mutations / telemetry；`logTransactionTelemetryMarkers` 改收 `TransactionTelemetry` |
| `RNFeatureFlags.h` | 删除 `enableFabricRenderer` / `useTurboModules` 的 `override`（0.88 已非虚） |
| `Performance/RNOHMarker.cpp` | `logTaggedMarkerImpl` 赋值改用 lambda（重载歧义）；移除已删除的 `logTaggedMarkerBrigdelessImpl` |
| `arkui/ArkUINode.cpp` | `std::max(x, 0.0)` → `0.0f`；`GradientDirection` 改 `std::holds_alternative`（0.88 改为 `std::variant`） |
| `arkui/TouchEventDispatcher.cpp` | `timestamp` 转 `float`，并补新字段 `timeStamp`（`HighResTimeStamp::fromDOMHighResTimeStamp`） |
| `MutationsToNapiConverter.cpp` | `rawProps` 透传逻辑按新模型处理 |
| `RNOHCorePackage/.../ModalHostViewComponentInstance.cpp` | 6 处 double → `Float` 显式转换 |
| `RNOHCorePackage/.../ImageComponentInstance.cpp` | `tintColor` 为 `std::optional<SharedColor>`，用 `value_or(SharedColor{})` |
| `RNOHCorePackage/.../ScrollViewComponentInstance.cpp` | 删除 `scrollViewMetrics.velocity` 赋值（0.88 已移除） |

## 4. 复现步骤

```powershell
# 0. 环境
$env:DEVECO_SDK_HOME = 'D:\Program Files\Huawei\DevEco Studio\sdk'

# 1. 依赖
npm install

# 2. 原生库编译（HAR）
cd packages
node 'D:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.js' assembleHar `
  --mode module -p product=default -p module=react_native_harmony@default -p buildMode=debug --no-daemon

# 3. 发布原生库到 dist-libs 与示例依赖目录
cd ..
powershell -ExecutionPolicy Bypass -File scripts/publish-libs.ps1

# 4. JS bundle（注意 --reset-cache）
cd examples/HelloWorld
npx react-native bundle --platform harmony --entry-file ./src/index.js `
  --bundle-output ./harmony/entry/src/main/resources/rawfile/bundle.harmony.js `
  --assets-dest ./harmony/entry/src/main/resources/rawfile/assets --dev false --reset-cache

# 5. HAP
cd harmony
node 'D:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.js' assembleHap `
  --mode module -p product=default -p module=entry@default -p buildMode=debug --no-daemon
```

## 5. 验证结果

| 项目 | 结果 |
|---|---|
| `npm run build`（turbo） | EXIT=0 |
| 类型检查（ExpoDemo / 模板 / hvigor 插件） | 全部 EXIT=0 |
| `assembleHar`（原生全量编译 + 链接） | **EXIT=0** |
| 原生库 | 7 个新编 + `libffrt_cpp.so`，共 8 个随 HAP 打包 |
| JS bundle | `bundle.harmony.js` 816 KB |
| `assembleHap` | **EXIT=0**，`entry-default-unsigned.hap` **18.33 MB** |
| 真机运行 | ⏳ 待设备接入（`hdc list targets` 为空） |
