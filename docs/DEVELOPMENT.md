# RNOH-New 开发指南

## 环境搭建

### 1. 安装 HarmonyOS SDK

下载并安装 [DevEco Studio 5.0+](https://developer.harmonyos.com/cn/develop/deveco-studio)，它会自动安装 HarmonyOS SDK。

### 2. 配置环境变量

```bash
# Windows
set HARMONYOS_SDK_HOME=C:\Program Files\Huawei\DevEco Studio\sdk
set PATH=%PATH%;%HARMONYOS_SDK_HOME%\default\openharmony\toolchains

# macOS/Linux
export HARMONYOS_SDK_HOME=/Applications/DevEco-Studio.app/Contents/sdk
export PATH=$PATH:$HARMONYOS_SDK_HOME/default/openharmony/toolchains
```

### 3. 安装 Node.js 依赖

```bash
npm install
```

## 构建

### 构建核心库

```bash
# 构建所有包
npm run build

# 只构建核心引擎
cd packages/react-native-harmony
npm run build
```

### 构建示例项目

```bash
cd examples/HelloWorld/harmony
hvigorw assembleHap
```

## 调试

### 启用 Hermes Inspector

在 `RNHApp` 配置中启用：

```typescript
RNHApp({
  config: {
    moduleName: 'MyApp',
    enableInspector: true,
    inspectorPort: 9229,
  }
})
```

然后在 Chrome 中打开 `chrome://inspect` 进行调试。

### 查看日志

```bash
# 查看 HarmonyOS 日志
hdc shell hilog | grep RNH
```

## 测试

（待补充）

## 常见问题

### 1. 编译错误：找不到 ReactCommon 头文件

确保已正确初始化 `third-party/rn/ReactCommon` 子目录。

### 2. 运行时错误：TurboModule not found

确保已在 Package 中注册 TurboModule，并且 Package 已添加到 `RNHInstance`。

### 3. 白屏问题

检查以下几点：
- JS Bundle 是否正确加载
- Surface 是否正确启动
- 组件名是否正确注册

更多问题请查看 [FAQ](FAQ.md)。

## 贡献代码

请阅读 [CONTRIBUTING.md](../CONTRIBUTING.md) 了解贡献流程。

## 获取帮助

- [GitHub Issues](https://github.com/git-duzhengjie/RNOH-New/issues)
- [GitHub Discussions](https://github.com/git-duzhengjie/RNOH-New/discussions)
