# 原生构建链说明（自建运行时）

本仓库 `packages/react-native-harmony` 为**自建**的 RNOH 运行时：Hermes 使用官方 prebuilt `libhermesvm.so`，
其余（ReactCommon / Folly / glog / fmt / boost / double-conversion / fast_float / RNOH / RNOHCorePackage）**全部从源码编译**。

## 一、首次准备：同步第三方源码

第三方源码（fmt、double-conversion、glog、fast_float、boost、folly、ReactCommon、Hermes 头文件、prebuilt 引擎）
体积约 790MB，**不纳入 git 仓库**，需从官方 RNOH 源码树同步：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/vendor-thirdparty.ps1
# 或指定源目录
powershell -ExecutionPolicy Bypass -File scripts/vendor-thirdparty.ps1 -Source "D:\path\to\third-party"
```

> 注意：不包含 `libevent` 与 Hermes 引擎源码，构建链不依赖它们（引擎二进制走 `prebuilt/libhermesvm.so`）。

## 二、目录约定

```
packages/react-native-harmony/
├── src/main/cpp/
│   ├── CMakeLists.txt          # 构建入口
│   ├── RNOH/                   # 适配层核心（含 TaskExecutor/uv）
│   ├── RNOHCorePackage/        # 内置组件与 TurboModule
│   ├── patches/react_native_core/  # 对 ReactCommon 的补丁头
│   ├── third-party/
│   │   ├── fmt/ double-conversion/ glog/ fast_float/ boost/ folly/
│   │   ├── rn/ReactCommon/     # RN C++ 核心源码
│   │   ├── hermes/API + hermes/public   # 仅头文件（jsi、hermes API）
│   │   └── prebuilt/{debug,release}/{arm64-v8a,x86_64}/libhermesvm.so
│   └── （RNH-legacy/ 等为早期自研实现，已归档，不参与构建）
├── dist-libs/arm64-v8a/        # 原生库发布目录（不入库，由 publish-libs.ps1 生成）
└── src/main/resources/rawfile/rnoh.profdata   # PGO profile（仅 arm64-v8a）
```

## 三、依赖

- `@ppd/ffrt@1.1.10`（ohpm）：提供 `ffrt/cpp/pattern/*.h` 与 `libffrt_cpp.so`
  - 若 hvigor 未安装到 `oh_modules`，CMake 会走 fallback：从 `${RNOH_CPP_DIR}/../../../oh_modules/@ppd/ffrt` 建 imported target
- DevEco Studio SDK：`D:\Program Files\Huawei\DevEco Studio\sdk`
- `libuv.so`、`libffrt.z.so`、`libqos.so` 等由系统提供，**不需要**打进 HAP

## 四、构建原生库（产出 HAR 与 libs）

```powershell
cd e:\rnoh-new\packages
$env:DEVECO_SDK_HOME='D:\Program Files\Huawei\DevEco Studio\sdk'
& 'D:\Program Files\Huawei\DevEco Studio\tools\node\node.exe' `
  'D:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.js' `
  --mode module -p module=react_native_harmony@default -p product=default assembleHar --no-daemon
```

产物：
- HAR：`packages/react-native-harmony/build/default/outputs/default/react_native_harmony.har`
- .so：`build/default/intermediates/stripped_native_libs/default/arm64-v8a/`

> ⚠️ `externalNativeOptions` 必须写在模块级 **`buildOption`**，写到 `buildOptionSet` 里不会触发 CMake（只会得到 1ms 的空任务）。

## 五、发布原生库到包目录（关键步骤）

hvigor 从**依赖包目录的 `libs/<arch>/`** 收集原生库，因此编译完成后需发布：

> ⚠️ 不能发布到本包自己的 `libs/`：会与 CMake 输出重复，触发 `ProcessLibs` 的 Duplicated files 报错。
> 因此发布到 `dist-libs/`，再由脚本复制到下游工程的 `oh_modules/<pkg>/libs/<arch>/`。

```powershell
powershell -ExecutionPolicy Bypass -File e:\rnoh-new\scripts\publish-libs.ps1
powershell -ExecutionPolicy Bypass -File e:\rnoh-new\scripts\add-ffrt-lib.ps1   # 补 libffrt_cpp.so
```

## 六、构建 HelloWorld HAP

```powershell
cd e:\rnoh-new\examples\HelloWorld\harmony
$env:DEVECO_SDK_HOME='D:\Program Files\Huawei\DevEco Studio\sdk'
& 'D:\Program Files\Huawei\DevEco Studio\tools\node\node.exe' `
  'D:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.js' `
  --mode module -p product=default assembleHap --no-daemon
```

产物：`examples/HelloWorld/harmony/entry/build/default/outputs/default/entry-default-unsigned.hap`

校验：

```powershell
powershell -ExecutionPolicy Bypass -File e:\rnoh-new\scripts\inspect-hap.ps1
powershell -ExecutionPolicy Bypass -File e:\rnoh-new\scripts\verify-native.ps1
```

## 七、已知坑位

1. **ohpm 会覆盖手工放入 `oh_modules` 的包内容**：`ohpm install` 时按 `file:` 指向的 `.har` 重新解包，`.ohpm` 缓存优先级最高。
   本地联调时可删除 `oh_modules/.ohpm/@rnoh-new*` 缓存，并用 `scripts/replace-links.ps1` 把链接替换为真实目录副本。
2. **删除 `.ohpm` 缓存会使 `entry/oh_modules` 下的 junction 失效**（ENOENT stat），需重新 `ohpm install` 或替换为真实目录。
3. **`robocopy /XD <名字>` 会按目录名全局排除**，曾在复制 ReactCommon 时误删 `ReactCommon/hermes` 等目录。
4. 打包前务必确认 `libs/arm64-v8a/` 中存在 `libffrt_cpp.so`，否则 `librnoh_core.so` 在设备上加载失败。
5. 构建日志中的 `WARN: dependency which is not installed at oh-package.json5` 可忽略。
6. **不要**把编译产物放到包自己的 `libs/` 目录，否则下次 `assembleHar` 会报 `00306049 Duplicated files`；
   发布目录统一为 `dist-libs/`（已加入 .gitignore）。
