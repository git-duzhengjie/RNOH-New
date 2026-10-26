/**
 * RNOH-New: ActivityIndicator 组件实例
 *
 * 对应 React Native 的 <ActivityIndicator> 组件。
 * 内部持有一个 LOADING_PROGRESS 类型的 ArkUI 节点，提供加载指示器能力。
 */

#pragma once

#include "../RNH/RNHComponentInstance.h"
#include "../RNH/RNHArkUINode.h"
#include <memory>

namespace rnoh_new {

/**
 * ActivityIndicator 组件实例
 *
 * 负责将 RN 侧的 ActivityIndicator props 映射到 ArkUI LOADING_PROGRESS 节点上。
 * 支持动画开关、指示器颜色、尺寸大小等属性。
 */
class ActivityIndicatorComponentInstance : public RNHComponentInstance {
public:
    ActivityIndicatorComponentInstance();
    ~ActivityIndicatorComponentInstance() override = default;

    /**
     * 更新组件属性
     * 解析 JSON 字符串中的 props，映射到 ArkUI LOADING_PROGRESS 节点
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
     * @return "ActivityIndicatorView"
     */
    std::string getComponentName() const override;

    /**
     * 布局加载指示器组件
     * @param x 左上角 X 坐标
     * @param y 左上角 Y 坐标
     * @param width 宽度
     * @param height 高度
     */
    void layout(float x, float y, float width, float height) override;

private:
    /**
     * 尺寸字符串映射为像素值
     * small -> 20, large -> 36, 其他数字字符串 -> 对应数值
     * @param sizeStr 尺寸字符串
     * @return 像素值
     */
    static float mapSize(const std::string& sizeStr);

    // 内部持有的 ArkUI LOADING_PROGRESS 节点
    std::unique_ptr<RNHArkUINode> node_;

    // 是否正在播放动画
    bool animating_ = true;

    // 指示器尺寸（像素）
    float size_ = 20.0f;
};

} // namespace rnoh_new
