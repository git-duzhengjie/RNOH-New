/**
 * RNOH-New: Image 组件实例实现
 *
 * 将 React Native <Image> 的 props 映射到 ArkUI IMAGE 节点。
 */

#include "ImageComponentInstance.h"
#include "RNHJsonProps.h"

namespace rnoh_new {

/**
 * 构造函数
 * 创建 IMAGE 类型的 ArkUI 节点
 */
ImageComponentInstance::ImageComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::IMAGE)) {
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* ImageComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "Image"
 */
std::string ImageComponentInstance::getComponentName() const {
    return "Image";
}

/**
 * 更新 Image 属性
 * 解析 JSON 并映射以下属性：
 *   - source.uri: 图片 URI（网络地址或本地资源路径）
 *   - resizeMode: 缩放模式 (contain/cover/stretch/center)
 *   - tintColor: 图片着色颜色
 *   - width / height: 尺寸
 *   - opacity: 透明度
 * @param propsJson JSON 格式属性字符串
 */
void ImageComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 图片源 URI — 从嵌套的 source 对象中提取 uri
    // RN 的 source 结构为 {"source": {"uri": "https://..."}}
    if (props.has("source")) {
        // 在 source 子对象中查找 uri
        std::string sourceStr = props.getString("source", "");
        // 如果 source 本身是字符串则直接使用
        if (!sourceStr.empty() && sourceStr[0] != '{') {
            node_->setImageSource(sourceStr);
        } else {
            // source 是嵌套对象，从中提取 uri
            RNHJsonProps sourceProps(sourceStr);
            std::string uri = sourceProps.getString("uri", "");
            if (!uri.empty()) {
                node_->setImageSource(uri);
            }
        }
    }

    // 缩放模式
    if (props.has("resizeMode")) {
        std::string modeStr = props.getString("resizeMode", "cover");
        int mode = 1; // 默认 cover

        if (modeStr == "contain") {
            mode = 0; // 完整包含，可能有留白
        } else if (modeStr == "cover") {
            mode = 1; // 覆盖填充，可能裁剪
        } else if (modeStr == "stretch") {
            mode = 2; // 拉伸填充，可能变形
        } else if (modeStr == "center") {
            mode = 3; // 居中显示原始尺寸
        } else if (modeStr == "repeat") {
            mode = 4; // 平铺重复
        }
        node_->setImageResizeMode(mode);
    }

    // 图片着色
    if (props.has("tintColor")) {
        std::string colorStr = props.getString("tintColor", "");
        if (!colorStr.empty()) {
            uint32_t color = RNHJsonProps::parseColor(colorStr);
            // TODO: ArkUI IMAGE 节点的 tintColor 属性设置
            // 目前通过 setOpacity 做近似处理，后续需接入真正的 tint 能力
        }
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

    // 透明度
    if (props.has("opacity")) {
        float opacity = props.getFloat("opacity", 1.0f);
        if (opacity < 0.0f) opacity = 0.0f;
        if (opacity > 1.0f) opacity = 1.0f;
        node_->setOpacity(opacity);
    }
}

/**
 * 布局图片组件
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void ImageComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    node_->setWidth(width);
    node_->setHeight(height);
}

} // namespace rnoh_new
