/**
 * RNOH-New: Image 组件 JSI Binder
 *
 * 定义 <Image> 组件的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * Image 组件 props schema
 *
 * 声明 Image 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class ImageComponentJSIBinder {
public:
    /**
     * 获取 Image 组件的 props 描述列表
     * 包含 View 的基础属性 + Image 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // Image 特有属性
        std::vector<RNHPropDescriptor> imageProps = {
            // 图片源
            {"source",             "object",  "{uri:''}"},

            // 缩放模式
            {"resizeMode",         "string",  "cover"},

            // 着色
            {"tintColor",          "color",   ""},

            // 加载占位
            {"defaultSource",      "object",  "{uri:''}"},
            {"loadingIndicatorSource", "object", "{uri:''}"},

            // 模糊半径
            {"blurRadius",         "number",  "0"},

            // 圆角
            {"borderRadius",       "number",  "0"},
            {"borderTopLeftRadius",     "number", "0"},
            {"borderTopRightRadius",    "number", "0"},
            {"borderBottomLeftRadius",  "number", "0"},
            {"borderBottomRightRadius", "number", "0"},

            // 覆盖层颜色
            {"overlayColor",       "color",   ""},

            // 渐进加载
            {"progressiveRenderingEnabled", "boolean", "false"},
            {"fadeDuration",       "number",  "300"},

            // 事件
            {"onLoad",             "boolean", "false"},
            {"onLoadStart",        "boolean", "false"},
            {"onLoadEnd",          "boolean", "false"},
            {"onError",            "boolean", "false"},
            {"onProgress",         "boolean", "false"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), imageProps.begin(), imageProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "Image"
     */
    static std::string getComponentName() {
        return "Image";
    }
};

} // namespace rnoh_new
