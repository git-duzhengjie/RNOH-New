/**
 * RNOH-New: RNHArkUINode 实现
 *
 * ArkUI 原生节点 API 的轻量 C++ 封装。
 *
 * 编译策略（条件编译）：
 * - RNH_USE_ARKUI_NATIVE：真实 HarmonyOS 环境，包含 arkui/native_node.h，
 *   通过 OH_ArkUI_GetNodeAPI() 单例调用 createNode/insertChild 等真实 API
 * - 未定义时：Windows 语法验证/单元测试环境，所有 native 调用为安全空实现
 *
 * ArkUI C API 版本说明：
 * - ArkUI_NativeNodeAPI_1 提供 createNode/destroyNode/insertChild 等
 * - 属性设置通过 setAttribute + ArkUI_AttributeItem 构造
 * - 事件注册通过 registerNodeEvent + onNodeEvent 回调
 */

#include "RNHArkUINode.h"

#include <algorithm>
#include <unordered_map>

#ifdef RNH_USE_ARKUI_NATIVE
#include <arkui/native_node.h>
#include <arkui/native_interface.h>
#endif

namespace rnoh_new {

// ---------------------------------------------------------------------------
// ArkUI 节点类型映射（RNH 类型 -> ArkUI 原生类型）
// ---------------------------------------------------------------------------

#ifdef RNH_USE_ARKUI_NATIVE

/**
 * 将 RNHArkUINodeType 转换为 ArkUI_NodeType
 */
static ArkUI_NodeType toArkUINodeType(RNHArkUINodeType type) {
    switch (type) {
        case RNHArkUINodeType::COLUMN:        return ARKUI_NODE_COLUMN;
        case RNHArkUINodeType::ROW:           return ARKUI_NODE_ROW;
        case RNHArkUINodeType::STACK:         return ARKUI_NODE_STACK;
        case RNHArkUINodeType::FLEX:          return ARKUI_NODE_FLEX;
        case RNHArkUINodeType::LIST:          return ARKUI_NODE_LIST;
        case RNHArkUINodeType::GRID:          return ARKUI_NODE_GRID;
        case RNHArkUINodeType::SCROLL:        return ARKUI_NODE_SCROLL;
        case RNHArkUINodeType::TEXT:          return ARKUI_NODE_TEXT;
        case RNHArkUINodeType::TEXT_INPUT:    return ARKUI_NODE_TEXT_INPUT;
        case RNHArkUINodeType::TEXT_AREA:     return ARKUI_NODE_TEXT_AREA;
        case RNHArkUINodeType::IMAGE:         return ARKUI_NODE_IMAGE;
        case RNHArkUINodeType::BUTTON:        return ARKUI_NODE_BUTTON;
        case RNHArkUINodeType::CHECKBOX:      return ARKUI_NODE_CHECKBOX;
        case RNHArkUINodeType::RADIO:         return RNHArkUINodeType::RADIO == type ? ARKUI_NODE_RADIO : ARKUI_NODE_CUSTOM;
        case RNHArkUINodeType::TOGGLE:        return ARKUI_NODE_CUSTOM; // TODO: ArkUI 无独立 TOGGLE 节点，用 CUSTOM 占位
        case RNHArkUINodeType::SLIDER:        return ARKUI_NODE_SLIDER;
        case RNHArkUINodeType::PROGRESS:      return ARKUI_NODE_PROGRESS;
        case RNHArkUINodeType::LOADING_PROGRESS: return ARKUI_NODE_LOADING_PROGRESS;
        case RNHArkUINodeType::SWIPER:        return ARKUI_NODE_SWIPER;
        case RNHArkUINodeType::TABS:          return ARKUI_NODE_TABS;
        case RNHArkUINodeType::NAVIGATION:    return ARKUI_NODE_NAVIGATION;
        case RNHArkUINodeType::CUSTOM:
        default:                               return ARKUI_NODE_CUSTOM;
    }
}

/**
 * 全局事件接收器：ArkUI 回调 -> 转发到对应 RNHArkUINode 实例
 * ArkUI 的 onNodeEvent 回调只传 handle，需要 handle -> 实例 的映射
 */
static std::unordered_map<void*, RNHArkUINode*>& getNodeInstanceMap() {
    static std::unordered_map<void*, RNHArkUINode*> instanceMap;
    return instanceMap;
}

static std::mutex& getInstanceMapMutex() {
    static std::mutex m;
    return m;
}

/**
 * ArkUI 节点事件回调（C 风格），转发到 C++ 成员函数
 */
static void arkuiEventReceiver(ArkUI_NodeEvent* event, void* extraParam) {
    (void)extraParam;
    ArkUI_NodeHandle node = OH_ArkUI_NodeEvent_GetNodeHandle(event);
    if (!node) {
        return;
    }

    RNHArkUINode* instance = nullptr;
    {
        std::lock_guard<std::mutex> lock(getInstanceMapMutex());
        auto it = getNodeInstanceMap().find(node);
        if (it != getNodeInstanceMap().end()) {
            instance = it->second;
        }
    }
    if (!instance) {
        return;
    }

    int32_t eventType = OH_ArkUI_NodeEvent_GetEventType(event);
    switch (eventType) {
        case NODE_TOUCH_EVENT: {
            ArkUI_UIInputEvent* inputEvent =
                OH_ArkUI_NodeEvent_GetInputEvent(event);
            if (inputEvent) {
                int action = static_cast<int>(
                    OH_ArkUI_UIInputEvent_GetAction(inputEvent));
                float x = OH_ArkUI_UIInputEvent_GetX(inputEvent);
                float y = OH_ArkUI_UIInputEvent_GetY(inputEvent);
                instance->handleNativeTouch(action, x, y);
            }
            break;
        }
        case NODE_ON_CLICK: {
            instance->handleNativeClick();
            break;
        }
        default:
            break;
    }
}

#endif // RNH_USE_ARKUI_NATIVE

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHArkUINode::RNHArkUINode(RNHArkUINodeType type)
    : handle_(nullptr), type_(type) {
    createNode();
}

RNHArkUINode::~RNHArkUINode() {
    destroyNode();
}

// ---------------------------------------------------------------------------
// 移动语义（转移节点所有权，源对象置空）
// ---------------------------------------------------------------------------

RNHArkUINode::RNHArkUINode(RNHArkUINode&& other) noexcept
    : handle_(other.handle_),
      type_(other.type_),
      children_(std::move(other.children_)),
      measuredWidth_(other.measuredWidth_),
      measuredHeight_(other.measuredHeight_),
      touchCallback_(std::move(other.touchCallback_)),
      clickCallback_(std::move(other.clickCallback_)) {
    other.handle_ = nullptr;
}

RNHArkUINode& RNHArkUINode::operator=(RNHArkUINode&& other) noexcept {
    if (this != &other) {
        destroyNode();
        handle_ = other.handle_;
        type_ = other.type_;
        children_ = std::move(other.children_);
        measuredWidth_ = other.measuredWidth_;
        measuredHeight_ = other.measuredHeight_;
        touchCallback_ = std::move(other.touchCallback_);
        clickCallback_ = std::move(other.clickCallback_);
        other.handle_ = nullptr;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// 节点创建与销毁
// ---------------------------------------------------------------------------

/**
 * 创建原生 ArkUI 节点
 */
void RNHArkUINode::createNode() {
#ifdef RNH_USE_ARKUI_NATIVE
    ArkUI_NativeNodeAPI_1* api = getNodeAPI();
    if (!api) {
        return; // API 不可用，保持 handle_ 为空（安全降级）
    }

    // 创建节点
    ArkUI_NodeHandle node = api->createNode(toArkUINodeType(type_));
    if (!node) {
        return;
    }
    handle_ = node;

    // 注册到全局实例映射表（供事件回调查找）
    {
        std::lock_guard<std::mutex> lock(getInstanceMapMutex());
        getNodeInstanceMap()[node] = this;
    }

    // 注册触摸与点击事件（ArkUI 事件回调统一入口）
    api->registerNodeEvent(node, NODE_TOUCH_EVENT, 0);
    api->registerNodeEvent(node, NODE_ON_CLICK, 0);
    api->onNodeEvent(node, arkuiEventReceiver);
#endif
    // TODO: 非真实环境下 handle_ 保持 nullptr，后续 API 调用需判空
}

/**
 * 销毁原生 ArkUI 节点
 */
void RNHArkUINode::destroyNode() {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        ArkUI_NativeNodeAPI_1* api = getNodeAPI();
        if (api) {
            api->unregisterNodeEvent(handle_, NODE_TOUCH_EVENT);
            api->unregisterNodeEvent(handle_, NODE_ON_CLICK);
            api->onNodeEvent(handle_, nullptr);
            api->destroyNode(handle_);
        }

        // 从全局实例映射表移除
        {
            std::lock_guard<std::mutex> lock(getInstanceMapMutex());
            getNodeInstanceMap().erase(handle_);
        }
        handle_ = nullptr;
    }
#endif
}

// ---------------------------------------------------------------------------
// 子节点管理
// ---------------------------------------------------------------------------

/**
 * 添加子节点到指定位置（-1 表示追加到末尾）
 */
void RNHArkUINode::addChild(RNHArkUINode* child, int index) {
    if (!child || child == this) {
        return;
    }

    // 计算插入位置
    int pos = (index < 0 || index >= static_cast<int>(children_.size()))
                  ? static_cast<int>(children_.size())
                  : index;

#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_ && child->handle_) {
        ArkUI_NativeNodeAPI_1* api = getNodeAPI();
        if (api) {
            api->insertChild(handle_, child->handle_, pos);
        }
    }
#endif

    // 维护 children_ 向量
    children_.insert(children_.begin() + pos, child);
}

/**
 * 移除指定的子节点
 */
void RNHArkUINode::removeChild(RNHArkUINode* child) {
    if (!child) {
        return;
    }

    auto it = std::find(children_.begin(), children_.end(), child);
    if (it == children_.end()) {
        return;
    }

#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_ && child->handle_) {
        ArkUI_NativeNodeAPI_1* api = getNodeAPI();
        if (api) {
            api->removeChild(handle_, child->handle_);
        }
    }
#endif

    children_.erase(it);
}

/**
 * 移除所有子节点
 */
void RNHArkUINode::removeAllChildren() {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        ArkUI_NativeNodeAPI_1* api = getNodeAPI();
        if (api) {
            for (RNHArkUINode* child : children_) {
                if (child && child->handle_) {
                    api->removeChild(handle_, child->handle_);
                }
            }
        }
    }
#endif

    children_.clear();
}

// ---------------------------------------------------------------------------
// 布局属性
// ---------------------------------------------------------------------------

#ifdef RNH_USE_ARKUI_NATIVE

/**
 * 属性设置辅助：构造 ArkUI_AttributeItem 并调用 setAttribute
 */
static void setAttr(ArkUI_NativeNodeAPI_1* api, ArkUI_NodeHandle node,
                    ArkUI_NodeAttributeType type, float value) {
    ArkUI_AttributeItem item = {0};
    item.value = &value;
    item.size = 1;
    api->setAttribute(node, type, &item);
}

/**
 * 属性设置辅助（颜色，uint32）
 */
static void setAttrColor(ArkUI_NativeNodeAPI_1* api, ArkUI_NodeHandle node,
                         ArkUI_NodeAttributeType type, uint32_t color) {
    ArkUI_AttributeItem item = {0};
    item.value = reinterpret_cast<ArkUI_NumberValue*>(&color);
    item.size = 1;
    api->setAttribute(node, type, &item);
}

/**
 * 属性设置辅助（字符串）
 */
static void setAttrString(ArkUI_NativeNodeAPI_1* api, ArkUI_NodeHandle node,
                          ArkUI_NodeAttributeType type,
                          const std::string& str) {
    ArkUI_AttributeItem item = {0};
    item.string = str.c_str();
    api->setAttribute(node, type, &item);
}

#endif // RNH_USE_ARKUI_NATIVE

/**
 * 设置宽度（单位：px）
 */
void RNHArkUINode::setWidth(float width) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_WIDTH, width);
    }
#endif
}

/**
 * 设置高度（单位：px）
 */
void RNHArkUINode::setHeight(float height) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_HEIGHT, height);
    }
#endif
}

/**
 * 设置绝对位置（相对父容器左上角）
 */
void RNHArkUINode::setPosition(float x, float y) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        ArkUI_AttributeItem item = {0};
        ArkUI_NumberValue values[2];
        values[0].f32 = x;
        values[1].f32 = y;
        item.value = values;
        item.size = 2;
        getNodeAPI()->setAttribute(handle_, NODE_POSITION, &item);
    }
#endif
}

/**
 * 设置外边距
 */
void RNHArkUINode::setMargin(float left, float top, float right, float bottom) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        ArkUI_AttributeItem item = {0};
        ArkUI_NumberValue values[4];
        values[0].f32 = left;
        values[1].f32 = top;
        values[2].f32 = right;
        values[3].f32 = bottom;
        item.value = values;
        item.size = 4;
        getNodeAPI()->setAttribute(handle_, NODE_MARGIN, &item);
    }
#endif
}

/**
 * 设置内边距
 */
void RNHArkUINode::setPadding(float left, float top, float right, float bottom) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        ArkUI_AttributeItem item = {0};
        ArkUI_NumberValue values[4];
        values[0].f32 = left;
        values[1].f32 = top;
        values[2].f32 = right;
        values[3].f32 = bottom;
        item.value = values;
        item.size = 4;
        getNodeAPI()->setAttribute(handle_, NODE_PADDING, &item);
    }
#endif
}

// ---------------------------------------------------------------------------
// 视觉属性
// ---------------------------------------------------------------------------

/**
 * 设置背景色（ARGB 格式）
 */
void RNHArkUINode::setBackgroundColor(uint32_t color) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttrColor(getNodeAPI(), handle_, NODE_BACKGROUND_COLOR, color);
    }
#endif
}

/**
 * 设置不透明度（0.0 ~ 1.0）
 */
void RNHArkUINode::setOpacity(float opacity) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_OPACITY, opacity);
    }
#endif
}

/**
 * 设置圆角半径
 */
void RNHArkUINode::setBorderRadius(float radius) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_BORDER_RADIUS, radius);
    }
#endif
}

/**
 * 设置边框宽度
 */
void RNHArkUINode::setBorderWidth(float width) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_BORDER_WIDTH, width);
    }
#endif
}

/**
 * 设置边框颜色
 */
void RNHArkUINode::setBorderColor(uint32_t color) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttrColor(getNodeAPI(), handle_, NODE_BORDER_COLOR, color);
    }
#endif
}

/**
 * 设置是否裁剪子内容
 */
void RNHArkUINode::setClip(bool clip) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_CLIP, clip ? 1.0f : 0.0f);
    }
#endif
}

/**
 * 设置可见性
 */
void RNHArkUINode::setVisibility(bool visible) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_VISIBILITY,
                visible ? 0.0f : 1.0f); // 0=VISIBLE, 1=HIDDEN
    }
#endif
}

// ---------------------------------------------------------------------------
// Flex 属性
// ---------------------------------------------------------------------------

void RNHArkUINode::setFlexDirection(int direction) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_FLEX_DIRECTION,
                static_cast<float>(direction));
    }
#endif
}

void RNHArkUINode::setJustifyContent(int justify) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_JUSTIFY_CONTENT,
                static_cast<float>(justify));
    }
#endif
}

void RNHArkUINode::setAlignItems(int align) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_ALIGN_ITEMS,
                static_cast<float>(align));
    }
#endif
}

void RNHArkUINode::setFlexGrow(float grow) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_FLEX_GROW, grow);
    }
#endif
}

void RNHArkUINode::setFlexShrink(float shrink) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_FLEX_SHRINK, shrink);
    }
#endif
}

void RNHArkUINode::setFlexBasis(float basis) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_FLEX_BASIS, basis);
    }
#endif
}

void RNHArkUINode::setAlignSelf(int align) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_ALIGN_SELF,
                static_cast<float>(align));
    }
#endif
}

// ---------------------------------------------------------------------------
// 文本属性
// ---------------------------------------------------------------------------

/**
 * 设置文本内容（仅 TEXT 节点有效）
 */
void RNHArkUINode::setText(const std::string& text) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttrString(getNodeAPI(), handle_, NODE_TEXT_CONTENT, text);
    }
#endif
}

/**
 * 设置字体大小
 */
void RNHArkUINode::setFontSize(float size) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_FONT_SIZE, size);
    }
#endif
}

/**
 * 设置字体粗细（0-9，0 最细）
 */
void RNHArkUINode::setFontWeight(int weight) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_FONT_WEIGHT,
                static_cast<float>(weight));
    }
#endif
}

/**
 * 设置字体颜色
 */
void RNHArkUINode::setFontColor(uint32_t color) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttrColor(getNodeAPI(), handle_, NODE_FONT_COLOR, color);
    }
#endif
}

/**
 * 设置文本对齐方式
 */
void RNHArkUINode::setTextAlign(int align) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_TEXT_ALIGN,
                static_cast<float>(align));
    }
#endif
}

/**
 * 设置最大行数（0 表示不限制）
 */
void RNHArkUINode::setMaxLines(int maxLines) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_TEXT_MAX_LINES,
                static_cast<float>(maxLines));
    }
#endif
}

/**
 * 设置行高
 */
void RNHArkUINode::setLineHeight(float height) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_TEXT_LINE_HEIGHT, height);
    }
#endif
}

// ---------------------------------------------------------------------------
// 图片属性
// ---------------------------------------------------------------------------

/**
 * 设置图片资源（URL 或资源名）
 */
void RNHArkUINode::setImageSource(const std::string& src) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttrString(getNodeAPI(), handle_, NODE_IMAGE_SRC, src);
    }
#endif
}

/**
 * 设置图片缩放模式
 */
void RNHArkUINode::setImageResizeMode(int mode) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        setAttr(getNodeAPI(), handle_, NODE_IMAGE_RESIZE_MODE,
                static_cast<float>(mode));
    }
#endif
}

// ---------------------------------------------------------------------------
// 事件回调
// ---------------------------------------------------------------------------

/**
 * 设置触摸事件回调
 */
void RNHArkUINode::setOnTouchCallback(TouchEventCallback callback) {
    touchCallback_ = std::move(callback);
}

/**
 * 设置点击事件回调
 */
void RNHArkUINode::setOnClickCallback(ClickEventCallback callback) {
    clickCallback_ = std::move(callback);
}

/**
 * 原生触摸事件转发（由全局事件接收器调用）
 */
void RNHArkUINode::handleNativeTouch(int action, float x, float y) {
    if (touchCallback_) {
        touchCallback_(action, x, y);
    }
}

/**
 * 原生点击事件转发（由全局事件接收器调用）
 */
void RNHArkUINode::handleNativeClick() {
    if (clickCallback_) {
        clickCallback_();
    }
}

// ---------------------------------------------------------------------------
// 测量
// ---------------------------------------------------------------------------

/**
 * 测量节点期望尺寸
 * @param widthConstraint  宽度约束（可用宽度）
 * @param heightConstraint 高度约束（可用高度）
 * @param outWidth         [out] 测量出的宽度
 * @param outHeight        [out] 测量出的高度
 */
void RNHArkUINode::measure(float widthConstraint, float heightConstraint,
                           float& outWidth, float& outHeight) {
#ifdef RNH_USE_ARKUI_NATIVE
    if (handle_) {
        ArkUI_NativeNodeAPI_1* api = getNodeAPI();
        if (api) {
            // TODO: ArkUI measure 接口签名待确认，当前用 getLayoutSize 占位
            ArkUI_LayoutConstraint constraints = {0};
            ArkUI_IntOffset minSize = {0, 0};
            ArkUI_IntSize maxSize = {static_cast<int32_t>(widthConstraint),
                                     static_cast<int32_t>(heightConstraint)};
            constraints.minSize = minSize;
            constraints.maxSize = maxSize;
            api->measureNode(handle_, &constraints);

            ArkUI_IntSize size = api->getMeasuredSize(handle_);
            outWidth = static_cast<float>(size.width);
            outHeight = static_cast<float>(size.height);
            measuredWidth_ = outWidth;
            measuredHeight_ = outHeight;
            return;
        }
    }
#endif

    // 降级实现：直接返回约束尺寸（保证可编译且行为安全）
    outWidth = widthConstraint;
    outHeight = heightConstraint;
    measuredWidth_ = outWidth;
    measuredHeight_ = outHeight;
}

// ---------------------------------------------------------------------------
// Native Node API 单例获取
// ---------------------------------------------------------------------------

/**
 * 获取 ArkUI 原生节点 API 单例
 * TODO: 真实环境中通过 OH_ArkUI_GetNodeAPI() 获取
 */
ArkUI_NativeNodeAPI_1* RNHArkUINode::getNodeAPI() {
#ifdef RNH_USE_ARKUI_NATIVE
    static ArkUI_NativeNodeAPI_1* api = nullptr;
    if (!api) {
        // ArkUI 提供 OH_ArkUI_GetNodeAPI() 获取接口集合
        api = const_cast<ArkUI_NativeNodeAPI_1*>(
            reinterpret_cast<const ArkUI_NativeNodeAPI_1*>(
                OH_ArkUI_GetNodeAPI()));
    }
    return api;
#else
    return nullptr;
#endif
}

} // namespace rnoh_new
