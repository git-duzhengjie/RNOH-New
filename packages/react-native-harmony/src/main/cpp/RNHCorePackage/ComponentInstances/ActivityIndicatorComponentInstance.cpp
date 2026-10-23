/**
 * RNOH-New: ActivityIndicator 组件实例实现
 *
 * 将 React Native <ActivityIndicator> 的 props 映射到 ArkUI LOADING_PROGRESS 节点。
 */

#include "ActivityIndicatorComponentInstance.h"
#include "RNHJsonProps.h"

namespace rnoh_new {

/**
 * 构造函数
 * 创建 LOADING_PROGRESS 类型节点，默认尺寸 20px
 */
ActivityIndicatorComponentInstance::ActivityIndicatorComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::LOADING_PROGRESS)) {
    // 设置默认尺寸
    node_->setWidth(size_);
    node_->setHeight(size_);
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* ActivityIndicatorComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "ActivityIndicatorView"
 */
std::string ActivityIndicatorComponentInstance::getComponentName() const {
    return "ActivityIndicatorView";
}

/**
 * 更新 ActivityIndicator 属性
 * 解析 JSON 并映射以下属性：
 *   - animating: 是否播放动画
 *   - color: 指示器颜色
 *   - size: 尺寸 (small/large)
 * @param propsJson JSON 格式属性字符串
 */
void ActivityIndicatorComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 是否播放动画
    if (props.has("animating")) {
        animating_ = props.getBool("animating", true);
        // 动画开关通过可见性近似实现
        // TODO: RNHArkUINode 暂未提供 setLoadingAnimation，后续补充
        node_->setVisibility(animating_);
    }

    // 指示器颜色
    if (props.has("color")) {
        std::string colorStr = props.getString("color", "");
        if (!colorStr.empty()) {
            uint32_t color = RNHJsonProps::parseColor(colorStr);
            // TODO: RNHArkUINode 暂未提供 setLoadingColor，后续补充
            (void)color;
        }
    }

    // 尺寸（small/large 映射）
    if (props.has("size")) {
        std::string sizeStr = props.getString("size", "small");
        size_ = mapSize(sizeStr);
        node_->setWidth(size_);
        node_->setHeight(size_);
    }
}

/**
 * 布局加载指示器组件
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void ActivityIndicatorComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    // 指示器使用自身尺寸，忽略外部布局尺寸
    node_->setWidth(size_);
    node_->setHeight(size_);
}

/**
 * 尺寸字符串映射为像素值
 * small -> 20px, large -> 36px，数字字符串直接转换
 * @param sizeStr 尺寸字符串
 * @return 像素值
 */
float ActivityIndicatorComponentInstance::mapSize(const std::string& sizeStr) {
    if (sizeStr == "large") return 36.0f;
    if (sizeStr == "small") return 20.0f;
    // 数字字符串直接转换（自定义尺寸）
    try {
        return std::stof(sizeStr);
    } catch (...) {
        return 20.0f;
    }
}

} // namespace rnoh_new
