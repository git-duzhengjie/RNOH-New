/**
 * RNOH-New: ScrollView 组件实例实现
 *
 * 将 React Native <ScrollView> 的 props 映射到 ArkUI SCROLL 节点。
 */

#include "ScrollViewComponentInstance.h"
#include "RNHJsonProps.h"
#include <sstream>
#include <cmath>

namespace rnoh_new {

/**
 * 构造函数
 * 创建 SCROLL 类型节点并初始化滚动事件监听
 */
ScrollViewComponentInstance::ScrollViewComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::SCROLL)) {
    setupScrollEventListeners();
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* ScrollViewComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "ScrollView"
 */
std::string ScrollViewComponentInstance::getComponentName() const {
    return "ScrollView";
}

/**
 * 更新 ScrollView 属性
 * 解析 JSON 并映射以下属性：
 *   - contentContainerStyle.padding: 内容容器内边距
 *   - showsVerticalScrollIndicator: 是否显示垂直滚动条
 *   - showsHorizontalScrollIndicator: 是否显示水平滚动条
 *   - scrollEnabled: 是否启用滚动
 *   - horizontal: 是否横向滚动
 * @param propsJson JSON 格式属性字符串
 */
void ScrollViewComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 内容容器内边距（RN 属性 contentContainerStyle 为嵌套对象）
    if (props.has("contentContainerStyle")) {
        std::string styleStr = props.getString("contentContainerStyle", "");
        if (!styleStr.empty() && styleStr[0] == '{') {
            // 解析嵌套的 contentContainerStyle 对象
            RNHJsonProps styleProps(styleStr);
            if (styleProps.has("padding")) {
                float padding = styleProps.getFloat("padding", 0.0f);
                contentPaddingLeft_ = contentPaddingTop_ =
                contentPaddingRight_ = contentPaddingBottom_ = padding;
            }
            if (styleProps.has("paddingTop")) {
                contentPaddingTop_ = styleProps.getFloat("paddingTop", 0.0f);
            }
            if (styleProps.has("paddingBottom")) {
                contentPaddingBottom_ = styleProps.getFloat("paddingBottom", 0.0f);
            }
            if (styleProps.has("paddingLeft")) {
                contentPaddingLeft_ = styleProps.getFloat("paddingLeft", 0.0f);
            }
            if (styleProps.has("paddingRight")) {
                contentPaddingRight_ = styleProps.getFloat("paddingRight", 0.0f);
            }
        }
        // 应用内边距到节点
        node_->setPadding(contentPaddingLeft_, contentPaddingTop_,
                          contentPaddingRight_, contentPaddingBottom_);
    }

    // 垂直滚动条显隐
    if (props.has("showsVerticalScrollIndicator")) {
        bool show = props.getBool("showsVerticalScrollIndicator", true);
        // TODO: RNHArkUINode 暂未提供 setScrollBarVisibility，后续补充
        (void)show;
    }

    // 水平滚动条显隐
    if (props.has("showsHorizontalScrollIndicator")) {
        bool show = props.getBool("showsHorizontalScrollIndicator", true);
        // TODO: RNHArkUINode 暂未提供 setScrollBarVisibility，后续补充
        (void)show;
    }

    // 滚动开关
    if (props.has("scrollEnabled")) {
        scrollEnabled_ = props.getBool("scrollEnabled", true);
        // TODO: RNHArkUINode 暂未提供 setScrollEnabled，后续补充
        // scrollEnabled_ 状态保留，事件监听中判断使用
    }

    // 横向滚动模式
    if (props.has("horizontal")) {
        bool horizontal = props.getBool("horizontal", false);
        // TODO: RNHArkUINode 汳未提供 setScrollDirection，后续补充
        (void)horizontal;
    }
}

/**
 * 添加子组件到滚动容器
 * @param child 子组件实例
 * @param index 插入位置索引（-1 表示追加到末尾）
 */
void ScrollViewComponentInstance::addChild(std::shared_ptr<RNHComponentInstance> child, int index) {
    if (child && child->getArkUINode()) {
        node_->addChild(child->getArkUINode(), index);
    }
}

/**
 * 从滚动容器中移除子组件
 * @param child 子组件实例
 */
void ScrollViewComponentInstance::removeChild(std::shared_ptr<RNHComponentInstance> child) {
    if (child && child->getArkUINode()) {
        node_->removeChild(child->getArkUINode());
    }
}

/**
 * 布局滚动容器
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 偏移量
 * @param width 宽度
 * @param height 高度
 */
void ScrollViewComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    node_->setWidth(width);
    node_->setHeight(height);
}

/**
 * 滚动到指定偏移位置
 * @param xOffset X 轴偏移量
 * @param yOffset Y 轴偏移量
 * @param animated 是否启用动画
 */
void ScrollViewComponentInstance::scrollTo(float xOffset, float yOffset, bool animated) {
    // TODO: RNHArkUINode 暂未提供 scrollTo API，后续接入原生滚动控制
    // 当前仅记录目标偏移量
    scrollOffsetX_ = xOffset;
    scrollOffsetY_ = yOffset;
    (void)animated;
}

/**
 * 滚动到内容末尾
 * @param animated 是否启用动画
 */
void ScrollViewComponentInstance::scrollToEnd(bool animated) {
    // TODO: RNHArkUINode 暂未提供 scrollToIndex/scrollToEdge API
    // 当前仅标记为滚动到底部（offset 为最大值）
    scrollOffsetX_ = 0.0f;
    scrollOffsetY_ = std::numeric_limits<float>::max();
    (void)animated;
}

/**
 * 初始化滚动事件监听
 * 通过触摸回调近似模拟滚动事件上报
 * TODO: 后续接入 RNHArkUINode 的专用 onScroll 回调，获取真实偏移量
 */
void ScrollViewComponentInstance::setupScrollEventListeners() {
    // 捕获 this 指针用于事件上报（生命周期与组件实例一致）
    node_->setOnTouchCallback([this](int action, float x, float y) {
        // 触摸事件近似模拟滚动：
        // action: 0=按下, 1=移动, 2=抬起
        if (action == 1) { // 移动中，记录偏移
            // TODO: 真实实现需要从 ArkUI 滚动回调中获取 contentOffset
            // 当前仅记录触点位置，供调试使用
            (void)x;
            (void)y;
        }
    });
}

} // namespace rnoh_new
