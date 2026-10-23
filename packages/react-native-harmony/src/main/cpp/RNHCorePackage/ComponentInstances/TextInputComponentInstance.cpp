/**
 * RNOH-New: TextInput 组件实例实现
 *
 * 将 React Native <TextInput> 的 props 映射到 ArkUI TEXT_INPUT 节点。
 */

#include "TextInputComponentInstance.h"
#include "RNHJsonProps.h"
#include <sstream>

namespace rnoh_new {

/**
 * 构造函数
 * 创建 TEXT_INPUT 类型节点并初始化输入事件监听
 */
TextInputComponentInstance::TextInputComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::TEXT_INPUT)) {
    setupInputEventListeners();
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* TextInputComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "TextInput"
 */
std::string TextInputComponentInstance::getComponentName() const {
    return "TextInput";
}

/**
 * 更新 TextInput 属性
 * 解析 JSON 并映射以下属性：
 *   - value: 输入框内容
 *   - placeholder: 占位符文本
 *   - placeholderTextColor: 占位符颜色
 *   - secureTextEntry: 密码模式
 *   - multiline: 多行输入
 *   - maxLength: 最大长度
 *   - keyboardType: 键盘类型
 *   - autoFocus: 自动聚焦
 *   - editable: 可编辑
 * @param propsJson JSON 格式属性字符串
 */
void TextInputComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 输入框内容
    if (props.has("value")) {
        text_ = props.getString("value", "");
        node_->setText(text_);
    }

    // 占位符文本
    if (props.has("placeholder")) {
        std::string placeholder = props.getString("placeholder", "");
        // TODO: RNHArkUINode 暂未提供 setPlaceholder，后续补充
        (void)placeholder;
    }

    // 占位符颜色
    if (props.has("placeholderTextColor")) {
        std::string colorStr = props.getString("placeholderTextColor", "");
        if (!colorStr.empty()) {
            uint32_t color = RNHJsonProps::parseColor(colorStr);
            // TODO: RNHArkUINode 暂未提供 setPlaceholderColor，后续补充
            (void)color;
        }
    }

    // 密码模式
    if (props.has("secureTextEntry")) {
        bool secure = props.getBool("secureTextEntry", false);
        // TODO: RNHArkUINode 暂未提供 setSecureTextEntry，后续补充
        (void)secure;
    }

    // 多行输入
    if (props.has("multiline")) {
        multiline_ = props.getBool("multiline", false);
        // TODO: 多行模式应切换为 TEXT_AREA 节点，当前仅记录状态
        // TODO: RNHArkUINode 暂未提供节点类型切换，后续补充
    }

    // 最大长度
    if (props.has("maxLength")) {
        int maxLength = props.getInt("maxLength", -1);
        // TODO: RNHArkUINode 暂未提供 setMaxLength，后续补充
        (void)maxLength;
    }

    // 键盘类型
    if (props.has("keyboardType")) {
        std::string kbType = props.getString("keyboardType", "default");
        int mappedType = mapKeyboardType(kbType);
        // TODO: RNHArkUINode 暂未提供 setKeyboardType，后续补充
        (void)mappedType;
    }

    // 自动聚焦
    if (props.has("autoFocus")) {
        bool autoFocus = props.getBool("autoFocus", false);
        if (autoFocus) {
            // 自动聚焦在挂载后执行
            focus();
        }
    }

    // 可编辑
    if (props.has("editable")) {
        editable_ = props.getBool("editable", true);
        // TODO: RNHArkUINode 暂未提供 setEditable，后续补充
    }
}

/**
 * 布局文本输入组件
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void TextInputComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    node_->setWidth(width);
    node_->setHeight(height);
}

/**
 * 让输入框获得焦点
 */
void TextInputComponentInstance::focus() {
    // TODO: RNHArkUINode 暂未提供 requestFocus，后续补充
}

/**
 * 让输入框失去焦点
 */
void TextInputComponentInstance::blur() {
    // TODO: RNHArkUINode 暂未提供 clearFocus，后续补充
}

/**
 * 清空输入内容
 */
void TextInputComponentInstance::clear() {
    text_.clear();
    node_->setText("");
}

/**
 * 键盘类型字符串映射为内部枚举值
 * @param keyboardType 键盘类型字符串
 * @return 内部枚举值（0=default, 1=numeric, 2=email, 3=phone, 4=url）
 */
int TextInputComponentInstance::mapKeyboardType(const std::string& keyboardType) {
    if (keyboardType == "numeric")          return 1;
    if (keyboardType == "decimal-pad")      return 1;
    if (keyboardType == "number-pad")       return 1;
    if (keyboardType == "email-address")    return 2;
    if (keyboardType == "phone-pad")        return 3;
    if (keyboardType == "url")              return 4;
    return 0; // default
}

/**
 * 初始化输入事件监听
 * TODO: 后续接入 RNHArkUINode 的专用 onChange/onFocus/onBlur 回调
 */
void TextInputComponentInstance::setupInputEventListeners() {
    // 点击回调近似模拟聚焦事件
    node_->setOnClickCallback([this]() {
        // 上报 onFocus 事件
        std::string payload = "{}";
        dispatchEvent("onFocus", payload);

        // TODO: 真实实现需要接入 ArkUI 的 onChange 回调获取输入文本
        // 当前上报的 onChange 使用缓存的 text_ 内容
        std::ostringstream oss;
        oss << "{\"text\":\"" << text_ << "\"}";
        dispatchEvent("onChange", oss.str());
    });
}

} // namespace rnoh_new
