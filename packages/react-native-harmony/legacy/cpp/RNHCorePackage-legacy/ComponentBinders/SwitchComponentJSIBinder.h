/**
 * RNOH-New: Switch 组件 JSI Binder
 *
 * 定义 <Switch> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * Switch 组件 props schema
 *
 * 声明 Switch 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class SwitchComponentJSIBinder {
public:
    /**
     * 获取 Switch 组件的 props 描述列表
     * 包含 View 的基础属性 + Switch 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // Switch 特有属性
        std::vector<RNHPropDescriptor> switchProps = {
            // 状态
            {"value",         "boolean", "false"},
            {"disabled",      "boolean", "false"},

            // 颜色
            {"onTintColor",   "color",   ""},
            {"thumbColor",    "color",   ""},
            {"trackColor",    "object",  "{true:'',false:''}"},

            // iOS 特有（保留字段以兼容）
            {"tintColor",     "color",   ""},

            // 事件
            {"onValueChange", "boolean", "false"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), switchProps.begin(), switchProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "Switch"
     */
    static std::string getComponentName() {
        return "Switch";
    }
};

} // namespace rnoh_new
