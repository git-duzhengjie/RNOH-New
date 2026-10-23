/**
 * RNOH-New: RNHMountingManager 实现
 *
 * 接收 Fabric 渲染器（C++ 侧）下发的变更指令（Mutation），
 * 并应用到实际的 ArkUI 组件树。
 *
 * 性能优化：
 *  - 批量变更合并：同一 tag 的同类变更（UPDATE_PROPS / UPDATE_LAYOUT /
 *    UPDATE_STATE / UPDATE_EVENT_EMITTER）在应用前合并为最后一次，
 *    减少 ArkUI 节点操作次数。
 *  - CREATE+DELETE 抵消：同一 tag 先创建后删除，直接跳过，避免
 *    不必要的组件生命周期开销。
 *  - INSERT+REMOVE 抵消：同一父子对的插入后立即移除，直接跳过。
 *
 * 所有公共方法均加锁保证线程安全。
 */

#include "RNHMountingManager.h"
#include "RNHComponentInstance.h"
#include "RNHComponentDescriptorProvider.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

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
    // 整个批次在同一把锁内执行，保证原子性
    std::lock_guard<std::mutex> lock(mutex_);

    // 第一步：合并/优化变更列表（减少 ArkUI 操作次数）
    auto optimized = coalesceMutations(mutations);

    // 第二步：逐个应用优化后的变更
    for (const auto& mutation : optimized) {
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
// 批量变更合并优化
// ---------------------------------------------------------------------------

std::vector<RNHMutation> RNHMountingManager::coalesceMutations(
    const std::vector<RNHMutation>& mutations) const {
    // 结果列表：按顺序保留最终需要应用的变更
    std::vector<RNHMutation> result;
    result.reserve(mutations.size());

    // 记录各 tag 最后一次同类变更在 result 中的索引（-1 表示未出现）
    // 用于 UPDATE_PROPS / UPDATE_LAYOUT / UPDATE_STATE / UPDATE_EVENT_EMITTER
    // 的合并（后写覆盖前写）
    std::unordered_map<int, size_t> lastPropsIndex;     // tag → result index
    std::unordered_map<int, size_t> lastLayoutIndex;
    std::unordered_map<int, size_t> lastStateIndex;
    std::unordered_map<int, size_t> lastEventEmitterIndex;

    // 记录 CREATE 的 tag → result index（用于 CREATE+DELETE 抵消检测）
    std::unordered_map<int, size_t> createIndex;

    // 记录 INSERT 的 (parentTag, tag) → result index（用于 INSERT+REMOVE 抵消）
    // key 用 (parentTag << 32) | (tag & 0xFFFFFFFF) 合成
    auto makeInsertKey = [](int parent, int tag) -> uint64_t {
        return (static_cast<uint64_t>(parent) << 32) |
               (static_cast<uint64_t>(tag) & 0xFFFFFFFFULL);
    };
    std::unordered_map<uint64_t, size_t> insertIndex;

    for (const auto& m : mutations) {
        switch (m.type) {
            case RNHMutationType::UPDATE_PROPS: {
                auto it = lastPropsIndex.find(m.tag);
                if (it != lastPropsIndex.end()) {
                    // 同 tag 已有 UPDATE_PROPS，用最新的 propsJson 覆盖
                    result[it->second].propsJson = m.propsJson;
                } else {
                    // 新的变更，追加到结果
                    lastPropsIndex[m.tag] = result.size();
                    result.push_back(m);
                }
                break;
            }

            case RNHMutationType::UPDATE_LAYOUT: {
                auto it = lastLayoutIndex.find(m.tag);
                if (it != lastLayoutIndex.end()) {
                    // 同 tag 已有 UPDATE_LAYOUT，用最新的布局值覆盖
                    auto& dst = result[it->second];
                    dst.x = m.x;
                    dst.y = m.y;
                    dst.width = m.width;
                    dst.height = m.height;
                } else {
                    lastLayoutIndex[m.tag] = result.size();
                    result.push_back(m);
                }
                break;
            }

            case RNHMutationType::UPDATE_STATE: {
                auto it = lastStateIndex.find(m.tag);
                if (it != lastStateIndex.end()) {
                    result[it->second].stateJson = m.stateJson;
                } else {
                    lastStateIndex[m.tag] = result.size();
                    result.push_back(m);
                }
                break;
            }

            case RNHMutationType::UPDATE_EVENT_EMITTER: {
                auto it = lastEventEmitterIndex.find(m.tag);
                if (it != lastEventEmitterIndex.end()) {
                    // 事件发射器更新只保留最后一次
                    result[it->second] = m;
                } else {
                    lastEventEmitterIndex[m.tag] = result.size();
                    result.push_back(m);
                }
                break;
            }

            case RNHMutationType::CREATE: {
                createIndex[m.tag] = result.size();
                result.push_back(m);
                break;
            }

            case RNHMutationType::DELETE: {
                auto it = createIndex.find(m.tag);
                if (it != createIndex.end()) {
                    // CREATE + DELETE 成对出现 → 两者抵消
                    // 将 result 中对应位置的 CREATE 标记为无效（用空 tag = -1
                    // 占位，最后过滤掉）
                    result[it->second].tag = -1;
                    createIndex.erase(it);
                    // 同时清理该 tag 的其他待合并变更
                    lastPropsIndex.erase(m.tag);
                    lastLayoutIndex.erase(m.tag);
                    lastStateIndex.erase(m.tag);
                    lastEventEmitterIndex.erase(m.tag);
                } else {
                    result.push_back(m);
                }
                break;
            }

            case RNHMutationType::INSERT: {
                uint64_t key = makeInsertKey(m.parentTag, m.tag);
                insertIndex[key] = result.size();
                result.push_back(m);
                break;
            }

            case RNHMutationType::REMOVE: {
                uint64_t key = makeInsertKey(m.parentTag, m.tag);
                auto it = insertIndex.find(key);
                if (it != insertIndex.end()) {
                    // INSERT + REMOVE 成对且父子相同 → 两者抵消
                    result[it->second].tag = -1; // 标记为无效
                    insertIndex.erase(it);
                } else {
                    result.push_back(m);
                }
                break;
            }
        }
    }

    // 过滤掉被标记为无效的条目（tag == -1）
    std::vector<RNHMutation> finalResult;
    finalResult.reserve(result.size());
    for (auto& m : result) {
        if (m.tag != -1) {
            finalResult.push_back(std::move(m));
        }
    }

    return finalResult;
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
