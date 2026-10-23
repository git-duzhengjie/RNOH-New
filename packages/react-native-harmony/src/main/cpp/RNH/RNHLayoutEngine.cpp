/**
 * RNOH-New: RNHLayoutEngine 实现
 *
 * 简化版 Flexbox 布局引擎：
 * - measureNode：优先调用组件自身 measure，否则用 ArkUINode::measure
 * - layoutNode：设置节点最终 x/y/width/height
 * - layoutFlex：按 flexDirection / justifyContent / alignItems 排列子节点
 *
 * TODO: 后续接入 Yoga 时，将 measure/layout 逻辑替换为 Yoga 调用。
 */

#include "RNHLayoutEngine.h"
#include "RNHComponentInstance.h"
#include "RNHArkUINode.h"

#include <algorithm>
#include <cmath>

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHLayoutEngine::RNHLayoutEngine() = default;

RNHLayoutEngine::~RNHLayoutEngine() {
    clear();
}

// ---------------------------------------------------------------------------
// 根组件与属性注册
// ---------------------------------------------------------------------------

void RNHLayoutEngine::setRootComponent(
    std::shared_ptr<RNHComponentInstance> root) {
    std::lock_guard<std::mutex> lock(mutex_);
    rootComponent_ = std::move(root);
}

void RNHLayoutEngine::setLayoutMetrics(int tag,
                                        const RNHLayoutMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    layoutMetrics_[tag] = metrics;
}

RNHLayoutMetrics RNHLayoutEngine::getLayoutResult(int tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = layoutResults_.find(tag);
    if (it != layoutResults_.end()) {
        return it->second;
    }
    return RNHLayoutMetrics{};
}

void RNHLayoutEngine::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    layoutMetrics_.clear();
    layoutResults_.clear();
}

// ---------------------------------------------------------------------------
// 顶层布局入口
// ---------------------------------------------------------------------------

/**
 * 执行根节点布局：
 * 1. 查找根组件
 * 2. 对根节点调用 measureNode
 * 3. 对根节点调用 layoutNode
 * 4. 递归布局所有子节点
 */
bool RNHLayoutEngine::performLayout(int rootTag, float width, float height) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto root = rootComponent_.lock();
    if (!root) {
        return false;
    }

    // 根节点必须匹配指定 tag
    if (root->getTag() != rootTag) {
        return false;
    }

    float measuredWidth = 0.0f;
    float measuredHeight = 0.0f;

    // 测量根节点
    measureNode(root, width, height, measuredWidth, measuredHeight);

    // 布局根节点：占据整个可用区域
    layoutNode(root, 0.0f, 0.0f, width, height);

    return true;
}

// ---------------------------------------------------------------------------
// 节点测量
// ---------------------------------------------------------------------------

/**
 * 测量节点期望尺寸：
 * - 优先调用组件自身 measure 虚函数
 * - 否则尝试通过 getArkUINode()->measure 测量原生节点
 * - 若都不可用，返回约束尺寸作为降级
 */
void RNHLayoutEngine::measureNode(
    const std::shared_ptr<RNHComponentInstance>& node,
    float widthConstraint, float heightConstraint,
    float& outWidth, float& outHeight) {
    if (!node) {
        outWidth = 0.0f;
        outHeight = 0.0f;
        return;
    }

    // 1. 优先使用组件自身 measure（子类可覆盖实现自身尺寸逻辑）
    float compW = 0.0f, compH = 0.0f;
    node->measure(widthConstraint, heightConstraint, compW, compH);

    // 若组件返回有效尺寸，则直接使用
    if (compW > 0.0f && compH > 0.0f) {
        outWidth = compW;
        outHeight = compH;
        return;
    }

    // 2. 否则委托给 ArkUI 原生节点测量
    RNHArkUINode* arkuiNode = node->getArkUINode();
    if (arkuiNode) {
        arkuiNode->measure(widthConstraint, heightConstraint, outWidth,
                           outHeight);
        return;
    }

    // 3. 降级：返回约束尺寸
    outWidth = widthConstraint;
    outHeight = heightConstraint;
}

// ---------------------------------------------------------------------------
// 节点布局
// ---------------------------------------------------------------------------

/**
 * 对单个节点应用最终位置与尺寸，并递归布局子节点。
 */
void RNHLayoutEngine::layoutNode(
    const std::shared_ptr<RNHComponentInstance>& node,
    float x, float y, float width, float height) {
    if (!node) {
        return;
    }

    int tag = node->getTag();

    // 保存布局结果
    RNHLayoutMetrics result;
    result.x = x;
    result.y = y;
    result.width = width;
    result.height = height;
    layoutResults_[tag] = result;

    // 调用组件自身的 layout（子类可同步到 ArkUINode）
    node->layout(x, y, width, height);

    // 获取该组件的布局属性
    RNHLayoutMetrics metrics;
    auto it = layoutMetrics_.find(tag);
    if (it != layoutMetrics_.end()) {
        metrics = it->second;
    }

    // 递归布局子节点（简化 Flex 布局）
    layoutFlex(node, metrics, x + metrics.paddingLeft, y + metrics.paddingTop,
               std::max(0.0f, width - metrics.paddingLeft - metrics.paddingRight),
               std::max(0.0f, height - metrics.paddingTop - metrics.paddingBottom));
}

// ---------------------------------------------------------------------------
// 简化 Flex 布局
// ---------------------------------------------------------------------------

/**
 * 对子节点执行简化 Flex 布局：
 * - 仅支持单行/单列（不处理 wrap）
 * - 主轴按 justifyContent 分配空间
 * - 交叉轴按 alignItems 对齐
 * - 支持 flexGrow 扩展剩余空间
 */
void RNHLayoutEngine::layoutFlex(
    const std::shared_ptr<RNHComponentInstance>& node,
    const RNHLayoutMetrics& parentMetrics,
    float contentX, float contentY,
    float contentWidth, float contentHeight) {

    auto children = getChildren(node);
    if (children.empty()) {
        return;
    }

    bool isRow = (parentMetrics.flexDirection == RNHFlexDirection::Row ||
                  parentMetrics.flexDirection == RNHFlexDirection::RowReverse);
    bool reverse = (parentMetrics.flexDirection == RNHFlexDirection::RowReverse ||
                    parentMetrics.flexDirection == RNHFlexDirection::ColumnReverse);

    // 1. 测量每个子节点的自然尺寸
    struct ChildLayout {
        std::shared_ptr<RNHComponentInstance> node;
        float width = 0.0f;
        float height = 0.0f;
        float flexGrow = 0.0f;
        float marginMainStart = 0.0f;
        float marginMainEnd = 0.0f;
        float marginCrossStart = 0.0f;
        float marginCrossEnd = 0.0f;
    };

    std::vector<ChildLayout> childLayouts;
    childLayouts.reserve(children.size());

    float totalMain = 0.0f;
    float totalCross = 0.0f;
    float totalFlexGrow = 0.0f;

    for (const auto& child : children) {
        if (!child) {
            continue;
        }

        ChildLayout cl;
        cl.node = child;

        // 读取子节点的布局属性
        RNHLayoutMetrics childMetrics;
        auto it = layoutMetrics_.find(child->getTag());
        if (it != layoutMetrics_.end()) {
            childMetrics = it->second;
        }

        // 测量子节点
        measureNode(child, contentWidth, contentHeight, cl.width, cl.height);

        // 读取 flexGrow（若子节点未设置，使用默认值 0）
        cl.flexGrow = childMetrics.flexGrow;
        totalFlexGrow += cl.flexGrow;

        // 边距（主轴/交叉轴根据方向映射）
        if (isRow) {
            cl.marginMainStart = childMetrics.marginLeft;
            cl.marginMainEnd = childMetrics.marginRight;
            cl.marginCrossStart = childMetrics.marginTop;
            cl.marginCrossEnd = childMetrics.marginBottom;
        } else {
            cl.marginMainStart = childMetrics.marginTop;
            cl.marginMainEnd = childMetrics.marginBottom;
            cl.marginCrossStart = childMetrics.marginLeft;
            cl.marginCrossEnd = childMetrics.marginRight;
        }

        float mainSize = isRow ? cl.width : cl.height;
        mainSize += cl.marginMainStart + cl.marginMainEnd;
        totalMain += mainSize;

        float crossSize = isRow ? cl.height : cl.width;
        crossSize += cl.marginCrossStart + cl.marginCrossEnd;
        totalCross = std::max(totalCross, crossSize);

        childLayouts.push_back(std::move(cl));
    }

    if (childLayouts.empty()) {
        return;
    }

    // 2. 计算主轴可用空间
    float availableMain = isRow ? contentWidth : contentHeight;
    float remaining = std::max(0.0f, availableMain - totalMain);

    // 3. 计算 justifyContent 起始偏移
    float mainOffset = 0.0f;
    float gap = 0.0f;
    size_t childCount = childLayouts.size();

    switch (parentMetrics.justifyContent) {
        case RNHJustifyContent::FlexStart:
            mainOffset = 0.0f;
            break;
        case RNHJustifyContent::Center:
            mainOffset = remaining / 2.0f;
            break;
        case RNHJustifyContent::FlexEnd:
            mainOffset = remaining;
            break;
        case RNHJustifyContent::SpaceBetween:
            if (childCount > 1) {
                gap = remaining / static_cast<float>(childCount - 1);
            }
            break;
        case RNHJustifyContent::SpaceAround:
            gap = remaining / static_cast<float>(childCount);
            mainOffset = gap / 2.0f;
            break;
        case RNHJustifyContent::SpaceEvenly:
            gap = remaining / static_cast<float>(childCount + 1);
            mainOffset = gap;
            break;
    }

    // 4. 分配 flexGrow 额外空间
    float extraPerGrow = (totalFlexGrow > 0.0f)
                             ? (remaining / totalFlexGrow)
                             : 0.0f;

    // 5. 逐个摆放子节点
    float currentMain = mainOffset;
    float crossSize = isRow ? contentHeight : contentWidth;

    for (size_t i = 0; i < childLayouts.size(); ++i) {
        size_t idx = reverse ? (childLayouts.size() - 1 - i) : i;
        auto& cl = childLayouts[idx];

        // flexGrow 扩展主轴尺寸
        float extraMain = cl.flexGrow * extraPerGrow;
        float childMain = isRow ? cl.width : cl.height;
        childMain += extraMain;

        float childCross = isRow ? cl.height : cl.width;

        // 计算主轴位置
        float childMainPos = currentMain + cl.marginMainStart;

        // 计算交叉轴位置（alignItems）
        float childCrossPos = 0.0f;
        RNHAlignItems align = parentMetrics.alignItems;
        // TODO: 读取子节点 alignSelf 覆盖父 alignItems
        switch (align) {
            case RNHAlignItems::FlexStart:
                childCrossPos = cl.marginCrossStart;
                break;
            case RNHAlignItems::Center:
                childCrossPos = (crossSize - childCross) / 2.0f;
                break;
            case RNHAlignItems::FlexEnd:
                childCrossPos = crossSize - childCross - cl.marginCrossEnd;
                break;
            case RNHAlignItems::Stretch:
                childCrossPos = cl.marginCrossStart;
                childCross = std::max(0.0f, crossSize - cl.marginCrossStart -
                                                cl.marginCrossEnd);
                break;
            default:
                childCrossPos = cl.marginCrossStart;
                break;
        }

        // 将 (mainPos, crossPos) 映射回 (x, y)
        float childX = isRow ? childMainPos : childCrossPos;
        float childY = isRow ? childCrossPos : childMainPos;
        float childW = isRow ? childMain : childCross;
        float childH = isRow ? childCross : childMain;

        // 转换为全局坐标
        layoutNode(cl.node, contentX + childX, contentY + childY, childW,
                   childH);

        // 推进主轴光标
        currentMain += cl.marginMainStart + childMain + cl.marginMainEnd + gap;
    }
}

// ---------------------------------------------------------------------------
// 子节点获取（临时实现）
// ---------------------------------------------------------------------------

/**
 * 获取子节点列表。
 * TODO: 当前 RNHComponentInstance 未暴露 getChildren() 接口，
 * 暂时返回空列表。后续需在 RNHComponentInstance 中增加 children 管理。
 */
std::vector<std::shared_ptr<RNHComponentInstance>> RNHLayoutEngine::getChildren(
    const std::shared_ptr<RNHComponentInstance>& node) {
    (void)node;
    return {};
}

} // namespace rnoh_new
