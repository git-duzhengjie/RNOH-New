/**
 * RNOH-New: Text 组件实例实现
 *
 * 将 React Native <Text> 的 props 映射到 ArkUI TEXT 节点。
 */

#include "TextComponentInstance.h"
#include "RNHJsonProps.h"

namespace rnoh_new {

/**
 * 构造函数
 * 创建 TEXT 类型的 ArkUI 节点
 */
TextComponentInstance::TextComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::TEXT)) {
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* TextComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "Text"
 */
std::string TextComponentInstance::getComponentName() const {
    return "Text";
}

/**
 * 更新 Text 属性
 * 解析 JSON 并映射以下属性：
 *   - text: 文本内容
 *   - fontSize: 字体大小
 *   - fontWeight: 字重 (normal/bold/100~900)
 *   - color: 文字颜色
 *   - textAlign: 对齐方式 (left/center/right/auto/justify)
 *   - maxLines: 最大行数 (numberOfLines)
 *   - lineHeight: 行高
 * @param propsJson JSON 格式属性字符串
 */
void TextComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 文本内容
    if (props.has("text")) {
        std::string text = props.getString("text", "");
        node_->setText(text);
    }

    // 字体大小
    if (props.has("fontSize")) {
        float fontSize = props.getFloat("fontSize", 14.0f);
        node_->setFontSize(fontSize);
    }

    // 字重
    if (props.has("fontWeight")) {
        std::string weightStr = props.getString("fontWeight", "normal");
        int weight = 400; // 默认 normal = 400

        if (weightStr == "bold") {
            weight = 700;
        } else if (weightStr == "normal") {
            weight = 400;
        } else if (weightStr == "100") {
            weight = 100;
        } else if (weightStr == "200") {
            weight = 200;
        } else if (weightStr == "300") {
            weight = 300;
        } else if (weightStr == "500") {
            weight = 500;
        } else if (weightStr == "600") {
            weight = 600;
        } else if (weightStr == "700") {
            weight = 700;
        } else if (weightStr == "800") {
            weight = 800;
        } else if (weightStr == "900") {
            weight = 900;
        }
        node_->setFontWeight(weight);
    }

    // 文字颜色
    if (props.has("color")) {
        std::string colorStr = props.getString("color", "#000000");
        uint32_t color = RNHJsonProps::parseColor(colorStr);
        node_->setFontColor(color);
    }

    // 对齐方式
    if (props.has("textAlign")) {
        std::string alignStr = props.getString("textAlign", "auto");
        int align = 0; // 0=auto/start

        if (alignStr == "left") {
            align = 1;
        } else if (alignStr == "center") {
            align = 2;
        } else if (alignStr == "right") {
            align = 3;
        } else if (alignStr == "justify") {
            align = 4;
        }
        node_->setTextAlign(align);
    }

    // 最大行数（RN 属性名为 numberOfLines，也兼容 maxLines）
    if (props.has("numberOfLines")) {
        int maxLines = props.getInt("numberOfLines", 0);
        node_->setMaxLines(maxLines);
    } else if (props.has("maxLines")) {
        int maxLines = props.getInt("maxLines", 0);
        node_->setMaxLines(maxLines);
    }

    // 行高
    if (props.has("lineHeight")) {
        float lineHeight = props.getFloat("lineHeight", 0.0f);
        node_->setLineHeight(lineHeight);
    }
}

/**
 * 测量文本尺寸
 * 委托给 ArkUI 节点的测量能力
 * @param widthConstraint 宽度约束
 * @param heightConstraint 高度约束
 * @param outWidth 输出测量宽度
 * @param outHeight 输出测量高度
 */
void TextComponentInstance::measure(float widthConstraint, float heightConstraint,
                                     float& outWidth, float& outHeight) {
    node_->measure(widthConstraint, heightConstraint, outWidth, outHeight);
}

/**
 * 布局文本组件
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void TextComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    node_->setWidth(width);
    node_->setHeight(height);
}

} // namespace rnoh_new
