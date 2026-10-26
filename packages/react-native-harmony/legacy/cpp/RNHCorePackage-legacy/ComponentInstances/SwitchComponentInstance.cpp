/**
 * RNOH-New: Switch 组件实例实现
 *
 * 将 React Native <Switch> 的 props 映射到 ArkUI TOGGLE 节点。
 */

#include "SwitchComponentInstance.h"
#include "RNHJsonProps.h"
#include <sstream>

namespace rnoh_new {

/**
 * 构造函数
 * 创建 TOGGLE 类型节点并初始化切换事件监听
 */
SwitchComponentInstance::SwitchComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::TOGGLE)) {
    setupToggleEventListeners();
}

/**
 * 获取底层 ArkUI 节点
 * @return 节点指针
 */
RNHArkUINode* SwitchComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "Switch"
 */
std::string SwitchComponentInstance::getComponentName() const {
    return "Switch";
}

/**
 * 更新 Switch 属性
 * 解析 JSON 并映射以下属性：
 *   - value: 开关状态
 *   - disabled: 是否禁用
 *   - onTintColor: 开启时轨道颜色（旧 API，对应 trackColor.true）
 *   - thumbColor: 滑块颜色
 *   - trackColor: 轨道颜色 {true: ..., false: ...}
 * @param propsJson JSON 格式属性字符串
 */
void SwitchComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 开关状态
    if (props.has("value")) {
        value_ = props.getBool("value", false);
        // TODO: RNHArkUINode 暂未提供 setToggleState，后续补充
        // 状态保存在 value_ 中，事件上报时使用
    }

    // 是否禁用
    if (props.has("disabled")) {
        disabled_ = props.getBool("disabled", false);
        // TODO: RNHArkUINode 暂未提供 setEnabled，后续补充
    }

    // 开启时轨道颜色（RN 旧属性，兼容处理）
    if (props.has("onTintColor")) {
        std::string colorStr = props.getString("onTintColor", "");
        if (!colorStr.empty()) {
            uint32_t color = RNHJsonProps::parseColor(colorStr);
            // TODO: RNHArkUINode 暂未提供 setToggleTrackColor，后续补充
            (void)color;
        }
    }

    // 滑块颜色
    if (props.has("thumbColor")) {
        std::string colorStr = props.getString("thumbColor", "");
        if (!colorStr.empty()) {
            uint32_t color = RNHJsonProps::parseColor(colorStr);
            // TODO: RNHArkUINode 暂未提供 setToggleThumbColor，后续补充
            (void)color;
        }
    }

    // 轨道颜色（新 API，支持 true/false 两种状态）
    if (props.has("trackColor")) {
        std::string trackStr = props.getString("trackColor", "");
        if (!trackStr.empty() && trackStr[0] == '{') {
            // 解析嵌套的 trackColor 对象
            RNHJsonProps trackProps(trackStr);
            std::string trueColorStr = trackProps.getString("true", "");
            std::string falseColorStr = trackProps.getString("false", "");
            // TODO: RNHArkUINode 暂未提供 setToggleTrackColor，后续补充
            (void)trueColorStr;
            (void)falseColorStr;
        }
    }
}

/**
 * 布局开关组件
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void SwitchComponentInstance::layout(float x, float y, float width, float height) {
    node_->setPosition(x, y);
    // Switch 固定尺寸，忽略传入的 width/height
    // ArkUI TOGGLE 默认尺寸约 40x24 vp
    node_->setWidth(width);
    node_->setHeight(height);
}

/**
 * 初始化切换事件监听
 * 通过点击回调模拟开关切换
 * TODO: 后续接入 RNHArkUINode 的专用 onToggle 回调获取真实切换状态
 */
void SwitchComponentInstance::setupToggleEventListeners() {
    node_->setOnClickCallback([this]() {
        // 禁用状态下不响应切换
        if (disabled_) return;

        // 翻转状态并上报 onValueChange 事件
        value_ = !value_;
        std::ostringstream oss;
        oss << "{\"value\":" << (value_ ? "true" : "false") << "}";
        dispatchEvent("onValueChange", oss.str());
    });
}

} // namespace rnoh_new
