/**
 * RNOH-New: RNHMountingManager 实现
 *
 * 接收 Fabric 渲染器（C++ 侧）下发的变更指令（Mutation），
 * 并应用到实际的 ArkUI 组件树。
 * 所有公共方法均加锁保证线程安全。
 */

#include "RNHMountingManager.h"
#include "RNHComponentInstance.h"
#include "RNHComponentDescriptorProvider.h"

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHMountingManager::RNHMountingManager(
    std::shared_ptr<RNHComponentDescriptorProvider> descriptorProvider)
    : descriptorProvider_(std::move(descriptorProvider)) {
    // 构造函数：持有组件描述符提供者，用于 CREATE 时创建原生组件实例
}

RNHMountingManager::~RNHMountingManager() {
    // 析构函数：清空所有已注册组件
    clear();
}

// ---------------------------------------------------------------------------
// 批量变更处理
// ---------------------------------------------------------------------------

void RNHMountingManager::processMutations(
    const std::vector<RNHMutation>& mutations) {
    // 遍历每条变更指令，分发到对应的处理器
    // 注意：整个批次在同一把锁内执行，保证原子性
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& mutation : mutations) {
        switch (mutation.type) {
            case RNHMutationType::CREATE:
                handleCreate(mutation);
                break;
            case RNHMutationType::DELETE:
                handleDelete(mutation);
                break;
            case RNHMutationType::INSERT:
                handleInsert(mutation);
                break;
            case RNHMutationType::REMOVE:
                handleRemove(mutation);
                break;
            case RNHMutationType::UPDATE_PROPS:
                handleUpdateProps(mutation);
                break;
            case RNHMutationType::UPDATE_LAYOUT:
                handleUpdateLayout(mutation);
                break;
            case RNHMutationType::UPDATE_STATE:
                handleUpdateState(mutation);
                break;
            case RNHMutationType::UPDATE_EVENT_EMITTER:
                handleUpdateEventEmitter(mutation);
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// 组件查询与注册
// ---------------------------------------------------------------------------

std::shared_ptr<RNHComponentInstance> RNHMountingManager::getComponentByTag(
    int tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = componentRegistry_.find(tag);
    if (it != componentRegistry_.end()) {
        return it->second;
    }
    return nullptr;
}

void RNHMountingManager::registerComponent(
    int tag, std::shared_ptr<RNHComponentInstance> component) {
    std::lock_guard<std::mutex> lock(mutex_);
    componentRegistry_[tag] = std::move(component);
}

void RNHMountingManager::unregisterComponent(int tag) {
    std::lock_guard<std::mutex> lock(mutex_);
    componentRegistry_.erase(tag);
}

void RNHMountingManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    componentRegistry_.clear();
}

// ---------------------------------------------------------------------------
// 各类型 Mutation 处理器（内部方法，调用时已持有锁）
// ---------------------------------------------------------------------------

/**
 * CREATE：根据组件名创建原生组件实例，并加入注册表
 */
void RNHMountingManager::handleCreate(const RNHMutation& mutation) {
    if (!descriptorProvider_) {
        return;
    }

    // 获取对应的组件描述符
    auto descriptor =
        descriptorProvider_->getDescriptor(mutation.componentName);
    if (!descriptor) {
        // TODO: 记录日志：未找到组件描述符
        return;
    }

    // 创建原生组件实例
    auto component = descriptor->createInstance();
    if (!component) {
        return;
    }

    component->setTag(mutation.tag);
    component->onCreate();

    componentRegistry_[mutation.tag] = std::move(component);
}

/**
 * DELETE：从注册表移除并销毁组件实例
 */
void RNHMountingManager::handleDelete(const RNHMutation& mutation) {
    auto it = componentRegistry_.find(mutation.tag);
    if (it != componentRegistry_.end()) {
        it->second->onDestroy();
        componentRegistry_.erase(it);
    }
}

/**
 * INSERT：将子组件插入到父组件的指定位置
 */
void RNHMountingManager::handleInsert(const RNHMutation& mutation) {
    auto parentIt = componentRegistry_.find(mutation.parentTag);
    auto childIt = componentRegistry_.find(mutation.tag);

    if (parentIt == componentRegistry_.end() ||
        childIt == componentRegistry_.end()) {
        return;
    }

    auto& parent = parentIt->second;
    auto& child = childIt->second;

    // 调用父组件的 addChild 挂载子节点
    parent->addChild(child, mutation.index);
    child->onAttach();
}

/**
 * REMOVE：将子组件从父组件中移除
 */
void RNHMountingManager::handleRemove(const RNHMutation& mutation) {
    auto parentIt = componentRegistry_.find(mutation.parentTag);
    auto childIt = componentRegistry_.find(mutation.tag);

    if (parentIt == componentRegistry_.end() ||
        childIt == componentRegistry_.end()) {
        return;
    }

    auto& parent = parentIt->second;
    auto& child = childIt->second;

    child->onDetach();
    parent->removeChild(child);
}

/**
 * UPDATE_PROPS：更新组件属性（JSON 序列化字符串）
 */
void RNHMountingManager::handleUpdateProps(const RNHMutation& mutation) {
    auto it = componentRegistry_.find(mutation.tag);
    if (it != componentRegistry_.end()) {
        it->second->updateProps(mutation.propsJson);
    }
}

/**
 * UPDATE_LAYOUT：更新组件布局（x, y, width, height）
 */
void RNHMountingManager::handleUpdateLayout(const RNHMutation& mutation) {
    auto it = componentRegistry_.find(mutation.tag);
    if (it != componentRegistry_.end()) {
        it->second->layout(mutation.x, mutation.y,
                           mutation.width, mutation.height);
    }
}

/**
 * UPDATE_STATE：更新组件内部状态（如文本内容、滚动偏移等）
 */
void RNHMountingManager::handleUpdateState(const RNHMutation& mutation) {
    auto it = componentRegistry_.find(mutation.tag);
    if (it != componentRegistry_.end()) {
        // TODO: 调用组件的 onStateChanged（当前基类未暴露该虚函数，先用 dispatchEvent 占位）
        it->second->dispatchEvent("__stateChanged__", mutation.stateJson);
    }
}

/**
 * UPDATE_EVENT_EMITTER：更新组件的事件发射器
 */
void RNHMountingManager::handleUpdateEventEmitter(
    const RNHMutation& mutation) {
    auto it = componentRegistry_.find(mutation.tag);
    if (it != componentRegistry_.end()) {
        // TODO: 绑定新的 EventEmitter 到组件实例
        (void)it;
    }
}

} // namespace rnoh_new
