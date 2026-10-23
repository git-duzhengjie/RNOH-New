/**
 * RNOH-New: 核心包实现
 *
 * 注册 View、Text、Image 三个基础组件的 ComponentDescriptor。
 */

#include "RNHCorePackage.h"
#include "ComponentInstances/ViewComponentInstance.h"
#include "ComponentInstances/TextComponentInstance.h"
#include "ComponentInstances/ImageComponentInstance.h"
#include "ComponentInstances/ScrollViewComponentInstance.h"
#include "ComponentInstances/TextInputComponentInstance.h"
#include "ComponentInstances/SwitchComponentInstance.h"
#include "ComponentInstances/ActivityIndicatorComponentInstance.h"
#include "ComponentInstances/ModalHostViewComponentInstance.h"

namespace rnoh_new {

/**
 * 获取包名
 * @return "RNHCorePackage"
 */
std::string RNHCorePackage::getName() const {
    return "RNHCorePackage";
}

/**
 * 获取包版本
 * @return 版本号字符串
 */
std::string RNHCorePackage::getVersion() const {
    return "0.1.0";
}

/**
 * 组件描述符实现模板
 * 为每个组件类型创建一个具体的 RNHComponentDescriptor 子类
 */
template<typename T>
class RNHConcreteComponentDescriptor : public RNHComponentDescriptor {
public:
    /**
     * 构造描述符
     * @param name 组件名称
     * @param handle 组件句柄（唯一标识）
     */
    RNHConcreteComponentDescriptor(const std::string& name, const std::string& handle)
        : name_(name), handle_(handle) {}

    /**
     * 获取组件名称
     * @return 组件名称字符串
     */
    std::string getComponentName() const override {
        return name_;
    }

    /**
     * 获取组件句柄
     * @return 组件句柄字符串
     */
    std::string getComponentHandle() const override {
        return handle_;
    }

    /**
     * 创建组件实例
     * @return 新的组件实例智能指针
     */
    std::shared_ptr<RNHComponentInstance> createInstance() override {
        return std::make_shared<T>();
    }

private:
    std::string name_;
    std::string handle_;
};

/**
 * 注册组件描述符
 * 将 View、Text、Image 三个基础组件注册到提供者中
 * @param provider 组件描述符提供者
 */
void RNHCorePackage::registerComponentDescriptors(
    RNHComponentDescriptorProvider* provider) {
    if (!provider) return;

    // 注册 View 组件
    provider->registerDescriptor("View", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<ViewComponentInstance>>(
            "View", "RNH.View");
    });

    // 注册 Text 组件
    provider->registerDescriptor("Text", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<TextComponentInstance>>(
            "Text", "RNH.Text");
    });

    // 注册 Image 组件
    provider->registerDescriptor("Image", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<ImageComponentInstance>>(
            "Image", "RNH.Image");
    });

    // 注册 ScrollView 组件
    provider->registerDescriptor("ScrollView", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<ScrollViewComponentInstance>>(
            "ScrollView", "RNH.ScrollView");
    });

    // 注册 TextInput 组件
    provider->registerDescriptor("TextInput", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<TextInputComponentInstance>>(
            "TextInput", "RNH.TextInput");
    });

    // 注册 Switch 组件
    provider->registerDescriptor("Switch", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<SwitchComponentInstance>>(
            "Switch", "RNH.Switch");
    });

    // 注册 ActivityIndicator 组件
    provider->registerDescriptor("ActivityIndicatorView", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<ActivityIndicatorComponentInstance>>(
            "ActivityIndicatorView", "RNH.ActivityIndicator");
    });

    // 注册 ModalHostView 组件
    provider->registerDescriptor("ModalHostView", []() {
        return std::make_shared<RNHConcreteComponentDescriptor<ModalHostViewComponentInstance>>(
            "ModalHostView", "RNH.ModalHostView");
    });
}

/**
 * 包注册时回调
 */
void RNHCorePackage::onRegister() {
    // 核心包注册完成
}

/**
 * 包注销时回调
 */
void RNHCorePackage::onUnregister() {
    // 核心包注销，清理资源
}

} // namespace rnoh_new
