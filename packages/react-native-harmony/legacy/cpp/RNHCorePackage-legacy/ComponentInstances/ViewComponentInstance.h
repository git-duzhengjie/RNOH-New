/**
 * RNOH-New: View 组件实例
 *
 * 对应 React Native 的 <View> 组件。
 * 内部持有一个 COLUMN 类型的 ArkUI 节点，作为最基础的容器组件。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * View 组件实例
 *
 * 负责将 RN 侧的 View props 映射到 ArkUI COLUMN 节点上。
 * 支持背景色、透明度、边框圆角、边框宽度、边框颜色、flex 等属性。
 */
class ViewComponentInstance : public RNHComponentInstance {
public:
    ViewComponentInstance();
    ~ViewComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI 节点
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
     * @return "View"
     */
    std::string getComponentName() const override;

    /**
     * 添加子组件
     * @param child 子组件实例
     * @param index 插入位置索引
     */
    void addChild(std::shared_ptr<RNHComponentInstance> child, int index) override;

    /**
     * 移除子组件
     * @param child 子组件实例
     */
    void removeChild(std::shared_ptr<RNHComponentInstance> child) override;

    /**
     * 布局组件
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

private:
    // 内部持有的 ArkUI COLUMN 节点
    std::unique_ptr<RNHArkUINode> node_;
};

} // namespace rnoh_new
