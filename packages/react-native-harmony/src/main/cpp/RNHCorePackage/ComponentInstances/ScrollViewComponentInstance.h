/**
 * RNOH-New: ScrollView 组件实例
 *
 * 对应 React Native 的 <ScrollView> 组件。
 * 内部持有一个 SCROLL 类型的 ArkUI 节点，提供可滚动容器能力。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * ScrollView 组件实例
 *
 * 负责将 RN 侧的 ScrollView props 映射到 ArkUI SCROLL 节点上。
 * 支持内容容器样式、滚动条显隐、滚动开关等属性。
 * 提供 scrollTo/scrollToEnd 命令供 TurboModule 调用。
 */
class ScrollViewComponentInstance : public RNHComponentInstance {
public:
    ScrollViewComponentInstance();
    ~ScrollViewComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI SCROLL 节点
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
     * @return "ScrollView"
     */
    std::string getComponentName() const override;

    /**
     * 添加子组件到滚动容器
     * @param child 子组件实例
     * @param index 插入位置索引
     */
    void addChild(std::shared_ptr<RNHComponentInstance> child, int index) override;

    /**
     * 从滚动容器中移除子组件
     * @param child 子组件实例
     */
    void removeChild(std::shared_ptr<RNHComponentInstance> child) override;

    /**
     * 布局滚动容器
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

    // --- 滚动命令（供 TurboModule 调用） ---

    /**
     * 滚动到指定偏移位置
     * @param xOffset X 轴偏移量
     * @param yOffset Y 轴偏移量
     * @param animated 是否启用动画
     */
    void scrollTo(float xOffset, float yOffset, bool animated);

    /**
     * 滚动到内容末尾
     * @param animated 是否启用动画
     */
    void scrollToEnd(bool animated);

private:
    /**
     * 初始化滚动事件监听
     * 通过节点触摸回调近似模拟滚动监听
     * TODO: 后续接入 RNHArkUINode 的专用 onScroll 回调
     */
    void setupScrollEventListeners();

    // 内部持有的 ArkUI SCROLL 节点
    std::unique_ptr<RNHArkUINode> node_;

    // 内容容器内边距
    float contentPaddingLeft_ = 0.0f;
    float contentPaddingTop_ = 0.0f;
    float contentPaddingRight_ = 0.0f;
    float contentPaddingBottom_ = 0.0f;

    // 是否启用滚动
    bool scrollEnabled_ = true;

    // 当前滚动偏移（由触摸事件近似计算）
    float scrollOffsetX_ = 0.0f;
    float scrollOffsetY_ = 0.0f;
};

} // namespace rnoh_new
