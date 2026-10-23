/**
 * RNOH-New: View 组件实例实现
 *
 * 将 React Native <View> 的 props 映射到 ArkUI COLUMN 节点。
 */

#include "ViewComponentInstance.h"
#include "RNHJsonProps.h"

namespace rnoh_new {

/**
 * 构造函数
 * 创建 COLUMN 类型的 ArkUI 节点作为容器
 */
ViewComponentInstance::ViewComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::COLUMN)) {
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* ViewComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "View"
 */
std::string ViewComponentInstance::getComponentName() const {
    return "View";
}

/**
 * 更新 View 属性
 * 解析 JSON 并映射以下属性：
 *   - backgroundColor: 背景颜色
 *   - opacity: 透明度 (0.0 ~ 1.0)
 *   - borderRadius: 边框圆角半径
 *   - borderWidth: 边框宽度
 *   - borderColor: 边框颜色
 *   - flex: 弹性布局权重 (flexGrow)
 *   - width / height: 尺寸
 * @param propsJson JSON 格式属性字符串
 */
void ViewComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 背景颜色
    if (props.has("backgroundColor")) {
        std::string colorStr = props.getString("backgroundColor", "#000000");
        uint32_t color = RNHJsonProps::parseColor(colorStr);
        node_->setBackgroundColor(color);
    }

    // 透明度
    if (props.has("opacity")) {
        float opacity = props.getFloat("opacity", 1.0f);
        // 限制在 0~1 范围
        if (opacity < 0.0f) opacity = 0.0f;
        if (opacity > 1.0f) opacity = 1.0f;
        node_->setOpacity(opacity);
    }

    // 边框圆角
    if (props.has("borderRadius")) {
        float radius = props.getFloat("borderRadius", 0.0f);
        node_->setBorderRadius(radius);
    }

    // 边框宽度
    if (props.has("borderWidth")) {
        float width = props.getFloat("borderWidth", 0.0f);
        node_->setBorderWidth(width);
    }

    // 边框颜色
    if (props.has("borderColor")) {
        std::string colorStr = props.getString("borderColor", "#000000");
        uint32_t color = RNHJsonProps::parseColor(colorStr);
        node_->setBorderColor(color);
    }

    // Flex 弹性权重
    if (props.has("flex")) {
        float flex = props.getFloat("flex", 0.0f);
        node_->setFlexGrow(flex);
    }

    // 宽度
    if (props.has("width")) {
        float width = props.getFloat("width", 0.0f);
        node_->setWidth(width);
    }

    // 高度
    if (props.has("height")) {
        float height = props.getFloat("height", 0.0f);
        node_->setHeight(height);
    }
}

/**
 * 添加子组件到 View 容器
 * @param child 子组件实例
 * @param index 插入位置索引（-1 表示追加到末尾）
 */
void ViewComponentInstance::addChild(std::shared_ptr<RNHComponentInstance> child, int index) {
    if (child && child->getArkUINode()) {
        node_->addChild(child->getArkUINode(), index);
    }
}

/**
 * 从 View 容器中移除子组件
 * @param child 子组件实例
 */
void ViewComponentInstance::removeChild(std::shared_ptr<RNHComponentInstance> child) {
    if (child && child->getArkUINode()) {
        node_->removeChild(child->getArkUINode());
    }
}

/**
 * 布局 View 组件
 * 设置节点位置和尺寸
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void ViewComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    node_->setWidth(width);
    node_->setHeight(height);
}

} // namespace rnoh_new
