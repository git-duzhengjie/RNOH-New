/**
 * RNOH-New: 核心包
 *
 * 注册所有内置基础组件（View、Text、Image）的 ComponentDescriptor。
 * 是框架默认加载的核心包，提供最基本的 UI 组件能力。
 */

#pragma once

#include "../RNH/RNHPackage.h"
#include "../RNH/RNHComponentDescriptorProvider.h"

namespace rnoh_new {

/**
 * 核心包
 *
 * 继承 RNHPackage，在 registerComponentDescriptors 中注册
 * View、Text、Image 三个基础组件。
 */
class RNHCorePackage : public RNHPackage {
public:
    RNHCorePackage() = default;
    ~RNHCorePackage() override = default;

    /**
     * 获取包名
     * @return "RNHCorePackage"
     */
    std::string getName() const override;

    /**
     * 获取包版本
     * @return 版本号字符串
     */
    std::string getVersion() const override;

    /**
     * 注册组件描述符
     * 将 View、Text、Image 组件注册到 ComponentDescriptorProvider 中
     * @param provider 组件描述符提供者
     */
    void registerComponentDescriptors(
        RNHComponentDescriptorProvider* provider) override;

    /**
     * 包注册时的回调
     */
    void onRegister() override;

    /**
     * 包注销时的回调
     */
    void onUnregister() override;
};

} // namespace rnoh_new
