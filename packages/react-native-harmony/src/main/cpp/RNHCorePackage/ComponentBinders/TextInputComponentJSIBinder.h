/**
 * RNOH-New: TextInput 组件 JSI Binder
 *
 * 定义 <TextInput> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * TextInput 组件 props schema
 *
 * 声明 TextInput 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class TextInputComponentJSIBinder {
public:
    /**
     * 获取 TextInput 组件的 props 描述列表
     * 包含 View 的基础属性 + TextInput 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // TextInput 特有属性
        std::vector<RNHPropDescriptor> inputProps = {
            // 内容与占位
            {"value",                    "string",  ""},
            {"placeholder",              "string",  ""},
            {"placeholderTextColor",     "color",   ""},

            // 输入模式
            {"secureTextEntry",          "boolean", "false"},
            {"multiline",                "boolean", "false"},
            {"maxLength",                "number",  "-1"},
            {"keyboardType",             "string",  "default"},
            {"autoFocus",                "boolean", "false"},
            {"editable",                 "boolean", "true"},

            // 文本样式
            {"fontSize",                 "number",  "14"},
            {"fontWeight",               "string",  "normal"},
            {"fontFamily",               "string",  ""},
            {"color",                    "color",   "#FF000000"},
            {"textAlign",                "string",  "start"},
            {"letterSpacing",            "number",  "0"},

            // 自动行为
            {"autoCorrect",              "boolean", "true"},
            {"autoCapitalize",           "string",  "sentences"},
            {"autoComplete",             "string",  "off"},
            {"caretHidden",              "boolean", "false"},

            // 键盘外观
            {"keyboardAppearance",       "string",  "default"},
            {"returnKeyType",            "string",  "done"},
            {"enablesReturnKeyAutomatically", "boolean", "false"},

            // 选中
            {"selection",                "object",  "{start:0,end:0}"},
            {"selectionColor",           "color",   ""},
            {"selectTextOnFocus",        "boolean", "false"},

            // 事件
            {"onChange",                 "boolean", "false"},
            {"onChangeText",             "boolean", "false"},
            {"onFocus",                  "boolean", "false"},
            {"onBlur",                   "boolean", "false"},
            {"onSubmitEditing",          "boolean", "false"},
            {"onEndEditing",             "boolean", "false"},
            {"onKeyPress",               "boolean", "false"},
            {"onContentSizeChange",      "boolean", "false"},
            {"onScroll",                 "boolean", "false"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), inputProps.begin(), inputProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "TextInput"
     */
    static std::string getComponentName() {
        return "TextInput";
    }
};

} // namespace rnoh_new
