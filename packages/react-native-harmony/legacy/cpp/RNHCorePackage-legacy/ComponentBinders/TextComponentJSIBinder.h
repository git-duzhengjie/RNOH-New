/**
 * RNOH-New: Text 组件 JSI Binder
 *
 * 定义 <Text> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * Text 组件 props schema
 *
 * 声明 Text 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class TextComponentJSIBinder {
public:
    /**
     * 获取 Text 组件的 props 描述列表
     * 包含 View 的基础属性 + Text 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // Text 特有属性
        std::vector<RNHPropDescriptor> textProps = {
            // 文本内容
            {"text",               "string",  ""},

            // 字体属性
            {"fontSize",           "number",  "14"},
            {"fontWeight",         "string",  "normal"},
            {"fontStyle",          "string",  "normal"},
            {"fontFamily",         "string",  ""},
            {"color",              "color",   "#FF000000"},
            {"letterSpacing",      "number",  "0"},

            // 对齐与排版
            {"textAlign",          "string",  "auto"},
            {"textAlignVertical",  "string",  "auto"},
            {"lineHeight",         "number",  "0"},
            {"textDecorationLine", "string",  "none"},
            {"textDecorationColor","color",   "#FF000000"},
            {"textShadowColor",    "color",   "#00000000"},
            {"textShadowOffset",   "object",  "{width:0,height:0}"},
            {"textShadowRadius",   "number",  "0"},
            {"textTransform",      "string",  "none"},

            // 行数限制
            {"numberOfLines",      "number",  "0"},
            {"maxLines",           "number",  "0"},
            {"ellipsizeMode",      "string",  "tail"},

            // 可选中
            {"selectable",         "boolean", "false"},
            {"selectionColor",     "color",   "#FF000000"},

            // 事件
            {"onPress",            "boolean", "false"},
            {"onLongPress",        "boolean", "false"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), textProps.begin(), textProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "Text"
     */
    static std::string getComponentName() {
        return "Text";
    }
};

} // namespace rnoh_new
