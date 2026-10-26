/**
 * RNOH-New: Switch 组件实例
 *
 * 对应 React Native 的 <Switch> 组件。
 * 内部持有一个 TOGGLE 类型的 ArkUI 节点，提供开关切换能力。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * Switch 组件实例
 *
 * 负责将 RN 侧的 Switch props 映射到 ArkUI TOGGLE 节点上。
 * 支持开关状态、禁用、轨道颜色、滑块颜色等属性。
 * 事件：onValueChange。
 */
class SwitchComponentInstance : public RNHComponentInstance {
public:
    SwitchComponentInstance();
    ~SwitchComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI TOGGLE 节点
     * @param propsJson JSON 格式的属性字符串
     */
    void updateProps(const std::string& propsJson) override;

    /**
     * 获取底层 ArkUI 节点指针
     * @return ArkUI 节点指针
     */
    RNHArkUINode* getArkUINode() override;

    /**
     * 获取组件名称
     * @return "Switch"
     */
    std::string getComponentName() const override;

    /**
     * 布局开关组件
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

    /**
     * 获取当前开关状态
     * @return true 表示开启
     */
    bool getValue() const { return value_; }

private:
    /**
     * 初始化切换事件监听
     * TODO: 后续接入 RNHArkUINode 的专用 onToggle/onValueChange 回调
     */
    void setupToggleEventListeners();

    // 内部持有的 ArkUI TOGGLE 节点
    std::unique_ptr<RNHArkUINode> node_;

    // 当前开关状态
    bool value_ = false;

    // 是否禁用
    bool disabled_ = false;
};

} // namespace rnoh_new
