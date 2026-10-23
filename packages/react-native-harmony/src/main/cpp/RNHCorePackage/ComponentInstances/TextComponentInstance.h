/**
 * RNOH-New: Text 组件实例
 *
 * 对应 React Native 的 <Text> 组件。
 * 内部持有一个 TEXT 类型的 ArkUI 节点，用于渲染文本内容。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * Text 组件实例
 *
 * 负责将 RN 侧的 Text props 映射到 ArkUI TEXT 节点上。
 * 支持文本内容、字体大小、字重、颜色、对齐方式、最大行数、行高等属性。
 */
class TextComponentInstance : public RNHComponentInstance {
public:
    TextComponentInstance();
    ~TextComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI TEXT 节点
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
     * @return "Text"
     */
    std::string getComponentName() const override;

    /**
     * 测量文本尺寸
     * @param widthConstraint 宽度约束
     * @param heightConstraint 高度约束
     * @param outWidth 输出测量宽度
     * @param outHeight 输出测量高度
     */
    void measure(float widthConstraint, float heightConstraint,
                 float& outWidth, float& outHeight) override;

    /**
     * 布局文本组件
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

private:
    // 内部持有的 ArkUI TEXT 节点
    std::unique_ptr<RNHArkUINode> node_;
};

} // namespace rnoh_new
