/**
 * RNOH-New: ModalHostView 组件实例
 *
 * 对应 React Native 的 <Modal> 组件的宿主视图。
 * 内部持有 STACK 类型的 ArkUI 节点 + 遮罩层节点，提供模态弹层能力。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * ModalHostView 组件实例
 *
 * 负责将 RN 侧的 Modal props 映射到 ArkUI STACK 节点上。
 * 结构：STACK 容器（全屏）+ 遮罩层（半透明黑）+ 内容容器。
 * 支持可见性、透明背景、动画类型、关闭请求回调等属性。
 * 显示/隐藏通过透明度动画过渡实现。
 */
class ModalHostViewComponentInstance : public RNHComponentInstance {
public:
    ModalHostViewComponentInstance();
    ~ModalHostViewComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI STACK 节点
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
     * @return "ModalHostView"
     */
    std::string getComponentName() const override;

    /**
     * 添加子组件到模态内容区
     * @param child 子组件实例
     * @param index 插入位置索引
     */
    void addChild(std::shared_ptr<RNHComponentInstance> child, int index) override;

    /**
     * 从模态内容区移除子组件
     * @param child 子组件实例
     */
    void removeChild(std::shared_ptr<RNHComponentInstance> child) override;

    /**
     * 布局模态宿主视图（全屏）
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

private:
    /**
     * 显示模态弹层
     * 通过透明度动画（渐显）实现
     */
    void show();

    /**
     * 隐藏模态弹层
     * 通过透明度动画（渐隐）实现
     */
    void hide();

    /**
     * 初始化遮罩层点击监听
     * 点击遮罩层时触发 onRequestClose 事件
     */
    void setupMaskEventListeners();

    // 内部持有的 ArkUI STACK 主容器节点
    std::unique_ptr<RNHArkUINode> node_;

    // 遮罩层节点（STACK 的第一个子节点）
    std::unique_ptr<RNHArkUINode> maskNode_;

    // 是否可见
    bool visible_ = false;

    // 是否透明背景（transparent 时遮罩层为全透明）
    bool transparent_ = false;

    // 动画类型: "none" | "slide" | "fade"
    std::string animationType_ = "none";
};

} // namespace rnoh_new
