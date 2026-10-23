# RNOH-New 稳定性保障文档

> 本文档描述 RNOH-New 项目的稳定性保障机制，包括异常恢复、内存防护、崩溃防护、监控告警等。

---

## 1. 总体原则

- **故障隔离**：单个组件/模块异常不影响整体应用
- **优雅降级**：核心功能不可用时提供降级方案
- **可观测性**：所有异常均可被监控、追踪、复现
- **自愈能力**：尽可能自动恢复，减少人工干预

---

## 2. 异常恢复机制

### 2.1 ErrorBoundary（错误边界）

在 React 组件树的多个层级部署 ErrorBoundary：

- **根级**：捕获全局未处理异常，展示兜底页面
- **页面级**：单个页面崩溃时展示错误占位，不影响其他页面
- **组件级**：关键组件（如图片、列表项）异常时展示占位图

```typescript
// 伪代码示例
class RNHErrorBoundary extends React.Component {
  state = { hasError: false, error: null };

  static getDerivedStateFromError(error) {
    return { hasError: true, error };
  }

  componentDidCatch(error, info) {
    RNHLogger.error('ErrorBoundary caught:', error, info);
    // 上报监控
    RNHPerfMonitor.reportError(error);
  }

  render() {
    if (this.state.hasError) {
      return <ErrorFallback error={this.state.error} />;
    }
    return this.props.children;
  }
}
```

### 2.2 JS 全局异常捕获

- `global.ErrorUtils.setGlobalHandler`：捕获未处理的 JS 异常
- `unhandledrejection`：捕获未处理的 Promise rejection
- 异常发生时：记录日志 → 上报监控 → 尝试恢复（重载页面/重启 JS 引擎）

### 2.3 超时保护

| 场景 | 超时时间 | 处理方式 |
|------|----------|----------|
| JS Bundle 加载 | 10 秒 | 超时降级使用本地缓存 |
| 原生模块调用 | 5 秒 | 超时返回错误，不阻塞 JS 线程 |
| 网络请求 | 30 秒 | 超时取消，触发重试机制 |
| 组件渲染 | 1 秒 | 超时标记为卡顿，上报监控 |

### 2.4 限流与防抖

- 事件限流：滚动、触摸等高频事件限流（16ms 间隔）
- 日志限流：错误日志上报频率限制（最多 10 条/分钟）
- 重试过载：指数退避重试，最多 3 次，避免雪崩

---

## 3. 内存防护

### 3.1 MemoryGuard（内存守护）

周期性检查内存使用，超过阈值时触发回收策略：

```
内存等级：
  正常  < 80 MB   → 正常运行
  警告  80-120 MB  → 释放非必要缓存、GC 提示
  危险  120-200 MB → 销毁离屏页面、释放图片缓存
  致命  > 200 MB   → 重启 JS 引擎，保留应用状态
```

### 3.2 对象池模式

对高频创建销毁的对象使用对象池：

- **ArkUI 节点池**：复用 View/Text/Image 等节点，减少 malloc/free
- **事件对象池**：触摸、滚动事件对象复用
- **字符串缓冲池**：props 变更时的字符串复用

### 3.3 泄漏检测

- **组件泄漏检测**：页面销毁后检查组件引用是否释放
- **定时器泄漏检测**：页面销毁后自动清理 setTimeout/setInterval
- **事件监听泄漏检测**：自动管理 addEventListener 的移除
- **内存快照对比**：测试环境下周期性对比 heap snapshot

### 3.4 图片内存管理

- LRU 图片缓存，限制最大缓存数量与内存上限
- 页面不可见时自动释放大图内存
- 图片尺寸自动适配，避免超大图加载

---

## 4. 崩溃防护

### 4.1 C++ 侧防护

| 机制 | 说明 |
|------|------|
| `try-catch` 包裹 | 所有 JSI 调用、原生模块调用均有异常捕获 |
| 断言（assert） | Debug 模式下启用，关键前置条件校验 |
| 空指针检查 | 所有指针解引用前检查有效性 |
| 越界检查 | 数组/容器访问前检查边界 |
| RAII 资源管理 | 智能指针 + RAII 确保资源释放 |
| 信号捕获 | `SIGSEGV`/`SIGABRT` 等致命信号捕获，生成崩溃日志 |

### 4.2 JS 侧防护

- 所有原生模块调用包裹 try-catch
- 空值安全：可选链 `?.` + 空值合并 `??`
- 类型校验：关键路径参数类型检查
- 状态机保护：UI 状态按状态机流转，非法状态自动修正

### 4.3 线程安全

- JS 线程与原生线程交互使用线程安全队列
- 共享数据使用互斥锁保护
- UI 操作统一在主线程执行
- 避免死锁：统一锁的获取顺序

---

## 5. 监控告警

### 5.1 错误事件监控

| 错误类型 | 采集字段 | 上报时机 |
|----------|----------|----------|
| JS 异常 | 堆栈、消息、时间、页面 | 实时 |
| 原生崩溃 | 信号、寄存器、堆栈 | 下次启动 |
| Promise rejection | reason、promise 链 | 实时 |
| 资源加载失败 | URL、状态码、耗时 | 实时 |

### 5.2 性能阈值告警

使用 `RNHPerfMonitor` 监控性能指标，超过阈值时触发告警：

- **FPS 告警**：连续 3 秒 < 45 FPS
- **内存告警**：RSS > 120 MB 持续 10 秒
- **启动告警**：TTI > 800 ms
- **卡顿告警**：JS 线程单次阻塞 > 100 ms

### 5.3 日志分级

| 级别 | 用途 | 输出位置 |
|------|------|----------|
| `debug` | 调试信息 | 仅 Debug 包 console |
| `info` | 关键流程 | console + 文件 |
| `warn` | 警告（可恢复异常） | console + 文件 + 上报 |
| `error` | 错误（影响功能） | console + 文件 + 上报 + 告警 |
| `fatal` | 致命错误（崩溃） | 文件 + 上报 + 告警 |

### 5.4 监控接入

```typescript
import { RNHPerfMonitor, RNHLogger } from '@rnoh-new/react-native-harmony';

// 开启性能监控
const monitor = RNHPerfMonitor.getInstance();
monitor.startFpsMonitor(1000);
monitor.startMemoryMonitor(2000);

// FPS 告警
monitor.on('fps', (fps) => {
  if (fps < 45) {
    RNHLogger.warn(`FPS 过低: ${fps.toFixed(1)}`);
  }
});

// 内存告警
monitor.on('memory', (mem) => {
  if (mem.rss > 120 * 1024 * 1024) {
    RNHLogger.error(`内存超限: ${(mem.rss / 1024 / 1024).toFixed(1)} MB`);
  }
});
```

---

## 6. 测试与验证

### 6.1 稳定性测试

- **Monkey 测试**：随机点击 10 万次，无崩溃
- **长稳测试**：连续运行 24 小时，内存增长 < 5%
- **弱网测试**：网络抖动/断网场景下不崩溃
- **低内存测试**：内存不足时优雅降级

### 6.2 故障注入测试

| 注入故障 | 预期结果 |
|----------|----------|
| JS 执行抛错 | ErrorBoundary 捕获，展示兜底 |
| 原生模块崩溃 | 异常捕获，JS 侧收到错误回调 |
| 内存耗尽 | MemoryGuard 触发回收，应用不崩溃 |
| 网络超时 | 超时返回，重试机制生效 |
| 主线程阻塞 | ANR 保护，自动恢复 |

### 6.3 发布门禁

- 崩溃率 < 0.1%
- ANR 率 < 0.05%
- 内存泄漏 < 1 MB/小时
- 核心功能通过率 100%

---

## 7. 相关文档

- [性能基准测试文档](./PERFORMANCE.md)
- [Autolinking 文档](./AUTOLINKING.md)
