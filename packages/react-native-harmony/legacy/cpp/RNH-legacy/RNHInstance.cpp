/**
 * RNOH-New: RNHInstance 核心实例实现
 *
 * 每个 RNHInstance 对应一个 JS bundle 执行上下文。
 * 负责：生命周期管理、Bundle 加载、Surface 管理、事件转发、TurboModule 注册。
 * 当前实现暂不依赖真实 Hermes/ReactCommon，涉及引擎处用 TODO 标注并返回安全默认值。
 */

#include "RNHInstance.h"
#include "RNHTurboModuleProvider.h"
#include "RNHComponentDescriptorProvider.h"
#include "RNHMountingManager.h"

#include <utility>
#include <mutex>

namespace rnoh_new {

/**
 * 内部状态持有结构体（Pimpl 惯用法）
 * 集中管理组件注册表、Surface 映射、TurboModule 工厂等运行时状态。
 */
struct RNHInstance::Impl {
    // TurboModule 工厂表：模块名 -> 创建函数
    std::unordered_map<std::string, std::function<void*()>> turboModuleFactories;

    // 已实例化的 TurboModule 缓存：模块名 -> 实例指针
    std::unordered_map<std::string, void*> turboModuleInstances;

    // 活跃 Surface 表：surfaceId -> 模块名
    std::unordered_map<int, std::string> activeSurfaces;

    // 当前 bundle 路径/URL
    std::string currentBundlePath;
    std::string currentBundleUrl;

    // MountingManager（Fabric 挂载管理）
    std::shared_ptr<RNHMountingManager> mountingManager;

    // 线程安全锁
    mutable std::mutex mutex;

    // Inspector 状态
    bool inspectorEnabled = false;
    uint16_t inspectorPort = 9229;
};

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHInstance::RNHInstance() : impl_(std::make_unique<Impl>()) {
    // 构造函数：初始化内部状态结构体
}

RNHInstance::~RNHInstance() {
    // 析构函数：确保资源被释放
    destroy();
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------

bool RNHInstance::initialize(const RNHInstanceConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (initialized_) {
        // 已初始化，直接返回成功（幂等）
        return true;
    }

    config_ = config;

    // TODO: 初始化 Hermes 引擎（依赖 react-hermes / ReactCommon）
    // TODO: 初始化 JSI Runtime
    // TODO: 初始化 Fabric Scheduler 与 MountingManager 绑定

    // 创建 MountingManager（若外部传入 ComponentDescriptorProvider）
    if (config_.componentDescriptorProvider) {
        impl_->mountingManager = std::make_shared<RNHMountingManager>(
            config_.componentDescriptorProvider);
    }

    initialized_ = true;
    return true;
}

void RNHInstance::destroy() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // TODO: 停止 Fabric Scheduler
    // TODO: 销毁 Hermes Runtime

    // 清理 TurboModule 实例缓存
    impl_->turboModuleInstances.clear();
    impl_->turboModuleFactories.clear();

    // 清理 Surface 表
    impl_->activeSurfaces.clear();

    // 释放 MountingManager
    impl_->mountingManager.reset();

    initialized_ = false;
}

// ---------------------------------------------------------------------------
// Bundle 管理
// ---------------------------------------------------------------------------

bool RNHInstance::loadBundle(const std::string& bundlePath) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return false;
    }

    // TODO: 通过 Hermes 引擎加载并执行本地 JS bundle 文件
    // 当前仅记录路径，返回成功占位
    impl_->currentBundlePath = bundlePath;
    return true;
}

bool RNHInstance::loadBundleFromUrl(const std::string& url) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return false;
    }

    // TODO: 通过 HTTP 下载 bundle 并交给 Hermes 引擎执行
    // 当前仅记录 URL，返回成功占位
    impl_->currentBundleUrl = url;
    return true;
}

void RNHInstance::reloadBundle() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // TODO: 销毁当前 JS 上下文，重新加载最近一次成功的 bundle
    // 优先使用本地路径，其次使用 URL
    // 当前为空实现占位
}

// ---------------------------------------------------------------------------
// Surface 管理
// ---------------------------------------------------------------------------

void RNHInstance::startSurface(int surfaceId, const std::string& moduleName,
                                float width, float height, float pixelRatio) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // 记录 Surface 到活跃表
    impl_->activeSurfaces[surfaceId] = moduleName;

    // TODO: 创建 Fabric SurfaceHandler，设置约束（宽高/像素比）
    // TODO: 调用 scheduler->startSurface(surfaceId, moduleName, {})
}

void RNHInstance::stopSurface(int surfaceId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // 从活跃表移除
    impl_->activeSurfaces.erase(surfaceId);

    // TODO: 调用 scheduler->stopSurface(surfaceId)
}

void RNHInstance::updateSurfaceSize(int surfaceId, float width, float height) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // TODO: 更新 SurfaceHandler 的布局约束
    (void)surfaceId;
    (void)width;
    (void)height;
}

// ---------------------------------------------------------------------------
// 事件处理
// ---------------------------------------------------------------------------

void RNHInstance::emitEvent(int surfaceId, const std::string& eventName,
                             const std::string& payload) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // TODO: 通过 EventDispatcher 将原生事件派发到 JS 侧
    (void)surfaceId;
    (void)eventName;
    (void)payload;
}

void RNHInstance::sendTouchEvent(int surfaceId, int action, float x, float y,
                                  int64_t timestamp) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // TODO: 构造 Fabric TouchEvent 并派发给对应的 EventTarget
    (void)surfaceId;
    (void)action;
    (void)x;
    (void)y;
    (void)timestamp;
}

// ---------------------------------------------------------------------------
// TurboModule
// ---------------------------------------------------------------------------

void RNHInstance::registerTurboModule(const std::string& name,
                                       std::function<void*()> factory) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->turboModuleFactories[name] = std::move(factory);
}

void* RNHInstance::getTurboModule(const std::string& name) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // 先查实例缓存
    auto it = impl_->turboModuleInstances.find(name);
    if (it != impl_->turboModuleInstances.end()) {
        return it->second;
    }

    // 再查工厂，惰性创建并缓存
    auto fit = impl_->turboModuleFactories.find(name);
    if (fit != impl_->turboModuleFactories.end()) {
        void* instance = fit->second();
        impl_->turboModuleInstances[name] = instance;
        return instance;
    }

    return nullptr;
}

// ---------------------------------------------------------------------------
// Inspector
// ---------------------------------------------------------------------------

void RNHInstance::enableInspector(uint16_t port) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->inspectorEnabled = true;
    impl_->inspectorPort = port;

    // TODO: 启动 Chrome DevTools Protocol 服务
}

void RNHInstance::disableInspector() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->inspectorEnabled = false;

    // TODO: 停止 Chrome DevTools Protocol 服务
}

// ---------------------------------------------------------------------------
// 内存管理
// ---------------------------------------------------------------------------

void RNHInstance::notifyMemoryPressure(int level) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!initialized_) {
        return;
    }

    // TODO: 通知 Hermes GC 进行内存回收（level: 0=正常, 1=中等, 2=严重）
    (void)level;
}

// ---------------------------------------------------------------------------
// 工厂函数
// ---------------------------------------------------------------------------

RNHInstanceRef createRNHInstance(const RNHInstanceConfig& config) {
    auto instance = std::make_shared<RNHInstance>();
    if (instance->initialize(config)) {
        return instance;
    }
    return nullptr;
}

} // namespace rnoh_new
