/**
 * RNOH-New: ScrollView 组件 JSI Binder
 *
 * 定义 <ScrollView> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * ScrollView 组件 props schema
 *
 * 声明 ScrollView 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class ScrollViewComponentJSIBinder {
public:
    /**
     * 获取 ScrollView 组件的 props 描述列表
     * 包含 View 的基础属性 + ScrollView 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // ScrollView 特有属性
        std::vector<RNHPropDescriptor> scrollProps = {
            // 内容容器样式
            {"contentContainerStyle", "object",  "{padding:0}"},

            // 滚动条显隐
            {"showsVerticalScrollIndicator",   "boolean", "true"},
            {"showsHorizontalScrollIndicator", "boolean", "true"},

            // 滚动行为
            {"scrollEnabled",     "boolean", "true"},
            {"horizontal",        "boolean", "false"},
            {"bounces",           "boolean", "true"},
            {"alwaysBounceVertical",   "boolean", "false"},
            {"alwaysBounceHorizontal", "boolean", "false"},
            {"pagingEnabled",     "boolean", "false"},
            {"disableIntervalMomentum", "boolean", "false"},
            {"nestedScrollEnabled", "boolean", "false"},

            // 滚动指示器样式
            {"indicatorStyle",    "string",  "default"},
            {"scrollIndicatorInsets", "object", "{top:0,left:0,bottom:0,right:0}"},

            // 滚动结束对齐
            {"snapToInterval",    "number",  "0"},
            {"snapToAlignment",   "string",  "start"},
            {"decelerationRate",  "string",  "normal"},
            {"scrollEventThrottle", "number", "0"},

            // 键盘处理
            {"keyboardShouldPersistTaps", "string", "never"},
            {"keyboardDismissMode", "string", "none"},

            // 刷新控制
            {"refreshControl",    "object",  "null"},

            // 事件
            {"onScroll",              "boolean", "false"},
            {"onScrollBeginDrag",     "boolean", "false"},
            {"onScrollEndDrag",       "boolean", "false"},
            {"onMomentumScrollBegin", "boolean", "false"},
            {"onMomentumScrollEnd",   "boolean", "false"},
            {"onScrollToTop",         "boolean", "false"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), scrollProps.begin(), scrollProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "ScrollView"
     */
    static std::string getComponentName() {
        return "ScrollView";
    }
};

} // namespace rnoh_new
