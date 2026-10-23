/**
 * RNOH-New: TextInput 组件实例
 *
 * 对应 React Native 的 <TextInput> 组件。
 * 内部持有一个 TEXT_INPUT 类型的 ArkUI 节点，提供文本输入能力。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * TextInput 组件实例
 *
 * 负责将 RN 侧的 TextInput props 映射到 ArkUI TEXT_INPUT 节点上。
 * 支持值、占位符、密码模式、多行、最大长度、键盘类型、
 * 自动聚焦、可编辑等属性。
 * 事件：onChange、onFocus、onBlur、onSubmitEditing。
 */
class TextInputComponentInstance : public RNHComponentInstance {
public:
    TextInputComponentInstance();
    ~TextInputComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI TEXT_INPUT 节点
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
     * @return "TextInput"
     */
    std::string getComponentName() const override;

    /**
     * 布局文本输入组件
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

    // --- 焦点管理 ---

    /**
     * 让输入框获得焦点
     */
    void focus();

    /**
     * 让输入框失去焦点
     */
    void blur();

    /**
     * 清空输入内容
     */
    void clear();

    /**
     * 获取当前输入文本
     * @return 当前文本内容
     */
    std::string getText() const { return text_; }

private:
    /**
     * 初始化输入事件监听
     * TODO: 后续接入 RNHArkUINode 的专用 onChange/onFocus/onBlur 回调
     */
    void setupInputEventListeners();

    /**
     * 键盘类型字符串映射为内部枚举值
     * @param keyboardType 键盘类型字符串
     * @return 内部枚举值（0=default, 1=numeric, ...）
     */
    static int mapKeyboardType(const std::string& keyboardType);

    // 内部持有的 ArkUI TEXT_INPUT 节点
    std::unique_ptr<RNHArkUINode> node_;

    // 当前文本内容
    std::string text_;

    // 是否多行输入
    bool multiline_ = false;

    // 是否可编辑
    bool editable_ = true;
};

} // namespace rnoh_new
