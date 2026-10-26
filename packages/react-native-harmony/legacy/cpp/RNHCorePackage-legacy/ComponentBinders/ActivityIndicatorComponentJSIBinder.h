/**
 * RNOH-New: ActivityIndicator 组件 JSI Binder
 *
 * 定义 <ActivityIndicator> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * ActivityIndicator 组件 props schema
 *
 * 声明 ActivityIndicator 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class ActivityIndicatorComponentJSIBinder {
public:
    /**
     * 获取 ActivityIndicator 组件的 props 描述列表
     * 包含 View 的基础属性 + ActivityIndicator 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // ActivityIndicator 特有属性
        std::vector<RNHPropDescriptor> indicatorProps = {
            // 动画开关
            {"animating", "boolean", "true"},

            // 颜色
            {"color",     "color",   ""},

            // 尺寸
            {"size",      "string",  "small"},

            // iOS 特有（保留字段以兼容）
            {"hidesWhenStopped", "boolean", "true"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), indicatorProps.begin(), indicatorProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "ActivityIndicatorView"
     */
    static std::string getComponentName() {
        return "ActivityIndicatorView";
    }
};

} // namespace rnoh_new
