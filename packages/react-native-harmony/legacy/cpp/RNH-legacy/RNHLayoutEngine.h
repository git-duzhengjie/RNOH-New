/**
 * RNOH-New: Layout Engine
 *
 * 布局引擎：从根节点开始递归测量（measure）并摆放（layout）组件。
 *
 * 当前实现提供简化版 Flexbox（支持 flexDirection / justifyContent / alignItems），
 * 作为 Yoga 未接入时的降级方案。后续接入 Yoga 时，measure/layout 委托给 Yoga 即可。
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace rnoh_new {

// Forward declarations
class RNHComponentInstance;
class RNHArkUINode;

/**
 * Flex 主轴方向
 */
enum class RNHFlexDirection {
    Row = 0,            // 水平方向
    Column = 1,         // 垂直方向
    RowReverse = 2,
    ColumnReverse = 3,
};

/**
 * 主轴对齐方式
 */
enum class RNHJustifyContent {
    FlexStart = 0,
    Center = 1,
    FlexEnd = 2,
    SpaceBetween = 3,
    SpaceAround = 4,
    SpaceEvenly = 5,
};

/**
 * 交叉轴对齐方式
 */
enum class RNHAlignItems {
    Auto = 0,
    FlexStart = 1,
    Center = 2,
    FlexEnd = 3,
    Stretch = 4,
    Baseline = 5,
};

/**
 * 单个组件的布局信息
 */
struct RNHLayoutMetrics {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float marginLeft = 0.0f;
    float marginTop = 0.0f;
    float marginRight = 0.0f;
    float marginBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingTop = 0.0f;
    float paddingRight = 0.0f;
    float paddingBottom = 0.0f;
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = 0.0f; // 0 表示 auto
    RNHFlexDirection flexDirection = RNHFlexDirection::Column;
    RNHJustifyContent justifyContent = RNHJustifyContent::FlexStart;
    RNHAlignItems alignItems = RNHAlignItems::Stretch;
    RNHAlignItems alignSelf = RNHAlignItems::Auto; // 默认继承父 alignItems
};

/**
 * 布局引擎
 */
class RNHLayoutEngine {
public:
    RNHLayoutEngine();
    ~RNHLayoutEngine();

    /**
     * 执行根节点布局
     * @param rootTag   根组件 tag
     * @param width     根容器可用宽度
     * @param height    根容器可用高度
     * @return 是否成功
     */
    bool performLayout(int rootTag, float width, float height);

    /**
     * 设置根组件（命中测试与布局起点）
     */
    void setRootComponent(std::shared_ptr<RNHComponentInstance> root);

    /**
     * 注册组件的布局属性
     */
    void setLayoutMetrics(int tag, const RNHLayoutMetrics& metrics);

    /**
     * 获取组件的布局结果
     */
    RNHLayoutMetrics getLayoutResult(int tag) const;

    /**
     * 清除所有缓存的布局结果
     */
    void clear();

private:
    /**
     * 测量单个节点：优先使用组件自身 measure，否则使用 ArkUINode::measure
     */
    void measureNode(const std::shared_ptr<RNHComponentInstance>& node,
                     float widthConstraint, float heightConstraint,
                     float& outWidth, float& outHeight);

    /**
     * 对单个节点及其子节点进行布局
     */
    void layoutNode(const std::shared_ptr<RNHComponentInstance>& node,
                    float x, float y, float width, float height);

    /**
     * 简化 Flex 布局：对子节点按主轴/交叉轴排列
     */
    void layoutFlex(const std::shared_ptr<RNHComponentInstance>& node,
                    const RNHLayoutMetrics& parentMetrics,
                    float contentX, float contentY,
                    float contentWidth, float contentHeight);

    /**
     * 获取子节点列表（当前依赖 RNHComponentInstance 暴露的子节点接口）
     * TODO: 在 RNHComponentInstance 中增加 getChildren() 后替换
     */
    std::vector<std::shared_ptr<RNHComponentInstance>> getChildren(
        const std::shared_ptr<RNHComponentInstance>& node);

    // 根组件
    std::weak_ptr<RNHComponentInstance> rootComponent_;

    // 输入：每个 tag 对应的布局属性
    std::unordered_map<int, RNHLayoutMetrics> layoutMetrics_;

    // 输出：每个 tag 的布局结果
    std::unordered_map<int, RNHLayoutMetrics> layoutResults_;

    // 线程安全锁
    mutable std::mutex mutex_;
};

} // namespace rnoh_new
