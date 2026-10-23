/**
 * RNOH-New: ModalHostView 组件实例实现
 *
 * 将 React Native <Modal> 的 props 映射到 ArkUI STACK 节点 + 遮罩层。
 */

#include "ModalHostViewComponentInstance.h"
#include "RNHJsonProps.h"
#include <sstream>

namespace rnoh_new {

/**
 * 构造函数
 * 创建 STACK 主容器 + 遮罩层节点，初始状态隐藏
 */
ModalHostViewComponentInstance::ModalHostViewComponentInstance()
    : node_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::STACK)),
      maskNode_(std::make_unique<RNHArkUINode>(RNHArkUINodeType::COLUMN)) {
    // 遮罩层默认半透明黑色背景
    maskNode_->setBackgroundColor(0x80000000);
    // 主容器初始隐藏
    node_->setVisibility(false);
    node_->setOpacity(0.0f);
    // 遮罩层作为 STACK 的第一个子节点
    node_->addChild(maskNode_.get(), 0);
    // 初始化遮罩层点击监听
    setupMaskEventListeners();
}

/**
 * 获取底层 ArkUI 节点
 * @return 主容器节点指针
 */
RNHArkUINode* ModalHostViewComponentInstance::getArkUINode() {
    return node_.get();
}

/**
 * 获取组件名称
 * @return "ModalHostView"
 */
std::string ModalHostViewComponentInstance::getComponentName() const {
    return "ModalHostView";
}

/**
 * 更新 ModalHostView 属性
 * 解析 JSON 并映射以下属性：
 *   - visible: 是否可见
 *   - transparent: 是否透明背景
 *   - animationType: 动画类型 (none/slide/fade)
 *   - onRequestClose: 关闭请求回调标记
 *   - statusBarTranslucent / navigationBarTranslucent: 沉浸式标记
 * @param propsJson JSON 格式属性字符串
 */
void ModalHostViewComponentInstance::updateProps(const std::string& propsJson) {
    RNHJsonProps props(propsJson);

    // 是否可见
    if (props.has("visible")) {
        bool newVisible = props.getBool("visible", false);
        if (newVisible != visible_) {
            visible_ = newVisible;
            if (visible_) {
                show();
            } else {
                hide();
            }
        }
    }

    // 是否透明背景
    if (props.has("transparent")) {
        transparent_ = props.getBool("transparent", false);
        // 透明模式下遮罩层全透明，否则半透明黑
        if (transparent_) {
            maskNode_->setBackgroundColor(0x00000000);
        } else {
            maskNode_->setBackgroundColor(0x80000000);
        }
    }

    // 动画类型
    if (props.has("animationType")) {
        animationType_ = props.getString("animationType", "none");
    }

    // 关闭请求回调（仅记录是否注册了回调）
    if (props.has("onRequestClose")) {
        bool hasCallback = props.getBool("onRequestClose", false);
        // 回调标记由遮罩层点击监听器使用
        (void)hasCallback;
    }

    // 状态栏沉浸式
    if (props.has("statusBarTranslucent")) {
        bool translucent = props.getBool("statusBarTranslucent", false);
        // TODO: 需要接入窗口级 API，超出节点能力范围，后续在 Window 层实现
        (void)translucent;
    }

    // 导航栏沉浸式
    if (props.has("navigationBarTranslucent")) {
        bool translucent = props.getBool("navigationBarTranslucent", false);
        // TODO: 需要接入窗口级 API，超出节点能力范围，后续在 Window 层实现
        (void)translucent;
    }
}

/**
 * 添加子组件到模态内容区
 * 子组件添加在遮罩层之上（索引 +1 偏移）
 * @param child 子组件实例
 * @param index 插入位置索引
 */
void ModalHostViewComponentInstance::addChild(std::shared_ptr<RNHComponentInstance> child, int index) {
    if (child && child->getArkUINode()) {
        // 遮罩层占据索引 0，内容从索引 1 开始
        node_->addChild(child->getArkUINode(), index < 0 ? -1 : index + 1);
    }
}

/**
 * 从模态内容区移除子组件
 * @param child 子组件实例
 */
void ModalHostViewComponentInstance::removeChild(std::shared_ptr<RNHComponentInstance> child) {
    if (child && child->getArkUINode()) {
        node_->removeChild(child->getArkUINode());
    }
}

/**
 * 布局模态宿主视图（全屏覆盖）
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param width 宽度
 * @param height 高度
 */
void ModalHostViewComponentInstance::layout(float x, float y, float width, float height) {
    // 主容器铺满全屏
    node_->setPosition(x, y);
    node_->setWidth(width);
    node_->setHeight(height);
    // 遮罩层同样铺满（STACK 内子节点默认居中，需手动铺满）
    maskNode_->setWidth(width);
    maskNode_->setHeight(height);
}

/**
 * 显示模态弹层
 * fade 动画：透明度 0 -> 1 渐显
 * slide 动画：TODO 后续接入位移动画
 */
void ModalHostViewComponentInstance::show() {
    node_->setVisibility(true);
    if (animationType_ == "fade") {
        // TODO: RNHArkUINode 暂未提供动画 API，当前直接设置终态透明度
        node_->setOpacity(1.0f);
    } else {
        // none / slide：直接显示
        node_->setOpacity(1.0f);
    }
}

/**
 * 隐藏模态弹层
 * fade 动画：透明度 1 -> 0 渐隐
 */
void ModalHostViewComponentInstance::hide() {
    if (animationType_ == "fade") {
        // TODO: RNHArkUINode 暂未提供动画 API，当前直接设置终态透明度
        node_->setOpacity(0.0f);
    } else {
        node_->setOpacity(0.0f);
    }
    node_->setVisibility(false);
}

/**
 * 初始化遮罩层点击监听
 * 点击遮罩层时触发 onRequestClose 事件（Android 返回键语义）
 */
void ModalHostViewComponentInstance::setupMaskEventListeners() {
    maskNode_->setOnClickCallback([this]() {
        // 上报关闭请求事件
        dispatchEvent("onRequestClose", "{}");
    });
}

} // namespace rnoh_new
