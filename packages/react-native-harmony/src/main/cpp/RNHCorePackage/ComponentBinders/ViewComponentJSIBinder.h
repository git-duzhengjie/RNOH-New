/**
 * RNOH-New: View 组件 JSI Binder
 *
 * 定义 <View> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <unordered_map>

namespace rnoh_new {

/**
 * 属性描述结构体
 * 定义单个 prop 的名称、类型和默认值
 */
struct RNHPropDescriptor {
    std::string name;       // 属性名
    std::string type;       // 类型: "string" | "number" | "boolean" | "color" | "object"
    std::string defaultVal; // 默认值（字符串表示）
};

/**
 * View 组件 props schema
 *
 * 声明 View 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class ViewComponentJSIBinder {
public:
    /**
     * 获取 View 组件的 props 描述列表
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        return {
            // 基础视觉属性
            {"backgroundColor",    "color",   "#00000000"},
            {"opacity",            "number",  "1.0"},
            {"borderRadius",       "number",  "0"},
            {"borderWidth",        "number",  "0"},
            {"borderColor",        "color",   "#FF000000"},

            // Flex 布局属性
            {"flex",               "number",  "0"},
            {"flexDirection",      "string",  "column"},
            {"justifyContent",     "string",  "flexStart"},
            {"alignItems",         "string",  "stretch"},
            {"alignSelf",          "string",  "auto"},
            {"flexGrow",           "number",  "0"},
            {"flexShrink",         "number",  "1"},
            {"flexBasis",          "number",  "0"},

            // 尺寸属性
            {"width",              "number",  "0"},
            {"height",             "number",  "0"},
            {"minWidth",           "number",  "0"},
            {"minHeight",          "number",  "0"},
            {"maxWidth",           "number",  "0"},
            {"maxHeight",          "number",  "0"},

            // 间距属性
            {"margin",             "number",  "0"},
            {"marginTop",          "number",  "0"},
            {"marginBottom",       "number",  "0"},
            {"marginLeft",         "number",  "0"},
            {"marginRight",        "number",  "0"},
            {"padding",            "number",  "0"},
            {"paddingTop",         "number",  "0"},
            {"paddingBottom",      "number",  "0"},
            {"paddingLeft",        "number",  "0"},
            {"paddingRight",       "number",  "0"},

            // 其他
            {"overflow",           "string",  "visible"},
            {"zIndex",             "number",  "0"},
            {"pointerEvents",      "string",  "auto"},
            {"removeClippedSubviews", "boolean", "false"},

            // 触摸事件
            {"onTouchStart",       "boolean", "false"},
            {"onTouchMove",        "boolean", "false"},
            {"onTouchEnd",         "boolean", "false"},
            {"onTouchCancel",      "boolean", "false"},
        };
    }

    /**
     * 获取组件名称
     * @return "View"
     */
    static std::string getComponentName() {
        return "View";
    }
};

} // namespace rnoh_new
