/**
 * RNOH-New: Image 组件实例
 *
 * 对应 React Native 的 <Image> 组件。
 * 内部持有一个 IMAGE 类型的 ArkUI 节点，用于渲染图片。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * Image 组件实例
 *
 * 负责将 RN 侧的 Image props 映射到 ArkUI IMAGE 节点上。
 * 支持图片源（URI）、缩放模式、着色等属性。
 */
class ImageComponentInstance : public RNHComponentInstance {
public:
    ImageComponentInstance();
    ~ImageComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI IMAGE 节点
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
     * @return "Image"
     */
    std::string getComponentName() const override;

    /**
     * 布局图片组件
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

private:
    // 内部持有的 ArkUI IMAGE 节点
    std::unique_ptr<RNHArkUINode> node_;
};

} // namespace rnoh_new
