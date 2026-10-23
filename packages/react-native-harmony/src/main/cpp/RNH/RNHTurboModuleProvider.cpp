/**
 * RNOH-New: RNHTurboModuleProvider 默认实现
 *
 * 提供 DefaultRNHTurboModuleProvider 类，实现 TurboModule 的注册、
 * 惰性加载（首次 getModule 时创建并缓存）、查询等功能。
 * 线程安全：所有公共方法均加锁。
 */

#include "RNHTurboModuleProvider.h"

#include <mutex>

namespace rnoh_new {

/**
 * 默认 TurboModule 提供者实现
 *
 * 用法示例：
 *   auto provider = std::make_shared<DefaultRNHTurboModuleProvider>();
 *   provider->registerModule("DeviceInfo", []() {
 *       return std::make_shared<DeviceInfoTurboModule>();
 *   });
 *   auto module = provider->getModule("DeviceInfo"); // 惰性创建并缓存
 */
class DefaultRNHTurboModuleProvider : public RNHTurboModuleProvider {
public:
    DefaultRNHTurboModuleProvider() = default;
    ~DefaultRNHTurboModuleProvider() override = default;

    /**
     * 注册 TurboModule 工厂函数
     * 仅注册工厂，不立即创建实例（惰性加载）
     */
    void registerModule(const std::string& name,
                        RNHTurboModuleFactory factory) override {
        std::lock_guard<std::mutex> lock(mutex_);
        factories_[name] = std::move(factory);
    }

    /**
     * 获取 TurboModule 实例
     * 首次调用时通过工厂创建并缓存，后续直接返回缓存
     */
    std::shared_ptr<RNHTurboModule> getModule(
        const std::string& name) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // 先查实例缓存
        auto it = instances_.find(name);
        if (it != instances_.end()) {
            return it->second;
        }

        // 再查工厂，惰性创建并缓存
        auto fit = factories_.find(name);
        if (fit != factories_.end()) {
            auto instance = fit->second();
            if (instance) {
                instance->initialize();
                instances_[name] = instance;
            }
            return instance;
        }

        return nullptr;
    }

    /**
     * 检查模块是否已注册（仅查工厂表，不触发创建）
     */
    bool hasModule(const std::string& name) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return factories_.find(name) != factories_.end();
    }

    /**
     * 获取所有已注册的模块名称列表
     */
    std::vector<std::string> getModuleNames() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        names.reserve(factories_.size());
        for (const auto& pair : factories_) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    // TurboModule 工厂表：模块名 -> 创建函数
    std::unordered_map<std::string, RNHTurboModuleFactory> factories_;

    // 已实例化的 TurboModule 缓存：模块名 -> 实例
    std::unordered_map<std::string, std::shared_ptr<RNHTurboModule>> instances_;

    // 线程安全锁
    mutable std::mutex mutex_;
};

// ---------------------------------------------------------------------------
// 工厂函数：创建默认 TurboModule 提供者
// ---------------------------------------------------------------------------

/**
 * 创建一个 DefaultRNHTurboModuleProvider 实例
 * 供 RNHInstance 或外部代码使用
 */
std::shared_ptr<RNHTurboModuleProvider> createDefaultTurboModuleProvider() {
    return std::make_shared<DefaultRNHTurboModuleProvider>();
}

} // namespace rnoh_new
