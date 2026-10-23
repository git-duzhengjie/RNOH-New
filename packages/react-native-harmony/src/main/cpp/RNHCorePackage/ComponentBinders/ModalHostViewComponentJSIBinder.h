/**
 * RNOH-New: ModalHostView 组件 JSI Binder
 *
 * 定义 <Modal> 宿主视图的 props schema，声明属性名、类型和默认值。
 * 用于在 JS 侧和 Native 侧之间建立属性映射契约。
 */

#pragma once

#include <string>
#include <vector>
#include "ViewComponentJSIBinder.h"

namespace rnoh_new {

/**
 * ModalHostView 组件 props schema
 *
 * 声明 ModalHostView 组件支持的所有属性及其类型信息，
 * 供 JSI 桥接层在序列化/反序列化时使用。
 */
class ModalHostViewComponentJSIBinder {
public:
    /**
     * 获取 ModalHostView 组件的 props 描述列表
     * 包含 View 的基础属性 + Modal 特有属性
     * @return 属性描述数组
     */
    static std::vector<RNHPropDescriptor> getPropsSchema() {
        // 继承 View 的基础属性
        auto schema = ViewComponentJSIBinder::getPropsSchema();

        // Modal 特有属性
        std::vector<RNHPropDescriptor> modalProps = {
            // 可见性与外观
            {"visible",         "boolean", "false"},
            {"transparent",     "boolean", "false"},
            {"animationType",   "string",  "none"},
            {"statusBarTranslucent", "boolean", "false"},
            {"navigationBarTranslucent", "boolean", "false"},

            // 硬件返回
            {"onRequestClose",  "boolean", "false"},

            // 显示方向
            {"supportedOrientations", "object", "['portrait']"},
        };

        // 合并到基础 schema
        schema.insert(schema.end(), modalProps.begin(), modalProps.end());
        return schema;
    }

    /**
     * 获取组件名称
     * @return "ModalHostView"
     */
    static std::string getComponentName() {
        return "ModalHostView";
    }
};

} // namespace rnoh_new
