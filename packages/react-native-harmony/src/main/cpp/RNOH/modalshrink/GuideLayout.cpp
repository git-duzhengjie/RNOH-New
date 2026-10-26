/*
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#include "GuideLayout.h"
#include <react/renderer/components/view/YogaStylableProps.h>
#include <react/renderer/core/ShadowNode.h>
#include <yoga/Yoga.h>
#include <yoga/node/Node.h>
#include <yoga/style/Style.h>
#include <yoga/style/StyleLength.h>
#include <yoga/style/StyleSizeLength.h>
#include <cstring>

namespace facebook {
namespace react {

// Helper function to convert YGNodeRef to yoga::Node*
static inline yoga::Node* toYogaNode(YGNodeRef node) {
  return static_cast<yoga::Node*>(node);
}

bool GuideLayout::scanAndScaleModalSubtrees(YGNodeRef rootYogaNode) {
  if (!rootYogaNode || !enableModalContentShrink_) {
    return false;
  }

  bool foundModal = false;
  auto* yogaNode = toYogaNode(rootYogaNode);

  // Safety check: ensure context is valid
  void* context = yogaNode->getContext();
  if (!context) {
    // No context, recursively check child nodes
    size_t childCount = yogaNode->getChildCount();
    for (size_t i = 0; i < childCount; ++i) {
      YGNodeRef child = yogaNode->getChild(i);
      if (child && scanAndScaleModalSubtrees(child)) {
        foundModal = true;
      }
    }
    return foundModal;
  }

  auto* shadowNode = static_cast<ShadowNode*>(context);
  if (!shadowNode) {
    return false;
  }

  const char* componentName = shadowNode->getComponentName();
  if (!componentName) {
    return false;
  }

  if (std::strcmp(componentName, "ModalHostView") == 0) {
    // Found Modal node. Scale starting from the content layer: find the first
    // node whose height differs from the screen height, then scale it and all
    // its siblings (the entire layer) rather than the full-screen wrappers
    // above it.
    // Use parent from traversal; getOwner() may return a dangling pointer.
    auto [contentParent, contentNode] =
        findContentNode(rootYogaNode, rawScreenHeight_);
    if (contentNode) {
      auto* cpNode = contentParent ? toYogaNode(contentParent) : nullptr;
      if (cpNode) {
        size_t layerChildCount = cpNode->getChildCount();
        for (size_t i = 0; i < layerChildCount; ++i) {
          YGNodeRef sibling = cpNode->getChild(i);
          if (sibling) {
            scaleYogaNodeStyleWithPercentHandling(sibling, false, false);
            collectModalSubtreeTags(sibling);
          }
        }
      } else {
        // Fallback: no parent found, scale contentNode alone
        scaleYogaNodeStyleWithPercentHandling(contentNode, false, false);
        collectModalSubtreeTags(contentNode);
      }
    }

    // Add marginTop to the first child of contentNode for vertical centering.
    // Calculate offset: screenHeight * (1 - scaleFactor) / 2
    if (scaleFactor_ < 1.0f && screenHeight_ > 0 && contentNode) {
      auto* contentYogaNode = toYogaNode(contentNode);
      if (contentYogaNode && contentYogaNode->getChildCount() > 0) {
        YGNodeRef contentChild = contentYogaNode->getChild(0);
        if (contentChild) {
          float marginTopOffset = screenHeight_ * (1.0f - scaleFactor_) / 2.0f;
          auto* nodeContentChild = toYogaNode(contentChild);
          auto& styleContentChild = nodeContentChild->style();

          // Get existing marginTop and add offset
          auto existingMargin = styleContentChild.margin(yoga::Edge::Top);
          float existingValue = 0.0f;
          if (existingMargin.isPoints() && existingMargin.value().isDefined()) {
            existingValue = existingMargin.value().unwrap();
          }

          styleContentChild.setMargin(
              yoga::Edge::Top,
              yoga::StyleLength::points(existingValue + marginTopOffset));
          nodeContentChild->setDirty(true);
        }
      }
    }

    foundModal = true;
    // No need to continue recursion, Modal's subtree has been processed
    return true;
  }

  // Recursively scan child nodes
  size_t childCount = yogaNode->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = yogaNode->getChild(i);
    if (child && scanAndScaleModalSubtrees(child)) {
      foundModal = true;
    }
  }

  return foundModal;
}

void GuideLayout::scaleYogaNodeStyleWithPercentHandling(
    YGNodeRef yogaNode,
    bool parentHasPercentWidth,
    bool parentHasPercentHeight) {
  if (!yogaNode) {
    return;
  }

  auto* node = toYogaNode(yogaNode);

  auto& style = node->style();
  bool modified = false;

  // Check if current node has percentage width/height
  auto width = style.dimension(yoga::Dimension::Width);
  auto height = style.dimension(yoga::Dimension::Height);
  bool currentHasPercentWidth =
      (width.isPercent() && width.value().isDefined() &&
       width.value().unwrap() > 0);
  bool currentHasPercentHeight =
      (height.isPercent() && height.value().isDefined() &&
       height.value().unwrap() > 0);

  // Check if current node has explicit dimensions (points or percentage).
  // Nodes without explicit dimensions (e.g., flex layout) derive their size
  // from the parent container. Scaling their padding would alter the content
  // area and cause percentage-based children to be scaled incorrectly.
  bool hasExplicitWidth = (width.isPoints() && width.value().isDefined() &&
                           width.value().unwrap() > 0) ||
      currentHasPercentWidth;
  bool hasExplicitHeight = (height.isPoints() && height.value().isDefined() &&
                            height.value().unwrap() > 0) ||
      currentHasPercentHeight;

  // Scale width - supports percentage
  if (width.isPoints() && width.value().isDefined() &&
      width.value().unwrap() > 0) {
    style.setDimension(
        yoga::Dimension::Width,
        yoga::StyleSizeLength::points(width.value().unwrap() * scaleFactor_));
    modified = true;
  } else if (currentHasPercentWidth && !parentHasPercentWidth) {
    // Scale percentage width only if parent doesn't have percentage width
    // If parent has percentage, child's percentage is relative to parent's
    // scaled size
    style.setDimension(
        yoga::Dimension::Width,
        yoga::StyleSizeLength::percent(width.value().unwrap() * scaleFactor_));
    modified = true;
  }

  // Scale height - supports percentage
  if (height.isPoints() && height.value().isDefined() &&
      height.value().unwrap() > 0) {
    style.setDimension(
        yoga::Dimension::Height,
        yoga::StyleSizeLength::points(height.value().unwrap() * scaleFactor_));
    modified = true;
  } else if (currentHasPercentHeight && !parentHasPercentHeight) {
    // Scale percentage height only if parent doesn't have percentage height
    // If parent has percentage, child's percentage is relative to parent's
    // scaled size
    style.setDimension(
        yoga::Dimension::Height,
        yoga::StyleSizeLength::percent(height.value().unwrap() * scaleFactor_));
    modified = true;
  }

  // Scale minimum dimensions (only process absolute values)
  auto minWidth = style.minDimension(yoga::Dimension::Width);
  if (minWidth.isPoints() && minWidth.value().isDefined() &&
      minWidth.value().unwrap() > 0) {
    style.setMinDimension(
        yoga::Dimension::Width,
        yoga::StyleSizeLength::points(
            minWidth.value().unwrap() * scaleFactor_));
    modified = true;
  }

  auto minHeight = style.minDimension(yoga::Dimension::Height);
  if (minHeight.isPoints() && minHeight.value().isDefined() &&
      minHeight.value().unwrap() > 0) {
    style.setMinDimension(
        yoga::Dimension::Height,
        yoga::StyleSizeLength::points(
            minHeight.value().unwrap() * scaleFactor_));
    modified = true;
  }

  // Scale maximum dimensions (only process absolute values)
  auto maxWidth = style.maxDimension(yoga::Dimension::Width);
  if (maxWidth.isPoints() && maxWidth.value().isDefined() &&
      maxWidth.value().unwrap() > 0) {
    style.setMaxDimension(
        yoga::Dimension::Width,
        yoga::StyleSizeLength::points(
            maxWidth.value().unwrap() * scaleFactor_));
    modified = true;
  }

  auto maxHeight = style.maxDimension(yoga::Dimension::Height);
  if (maxHeight.isPoints() && maxHeight.value().isDefined() &&
      maxHeight.value().unwrap() > 0) {
    style.setMaxDimension(
        yoga::Dimension::Height,
        yoga::StyleSizeLength::points(
            maxHeight.value().unwrap() * scaleFactor_));
    modified = true;
  }

  // Scale margin (only process absolute values)
  for (int edge = static_cast<int>(yoga::Edge::Left);
       edge <= static_cast<int>(yoga::Edge::All);
       ++edge) {
    auto margin = style.margin(static_cast<yoga::Edge>(edge));
    if (margin.isPoints() && margin.value().isDefined() &&
        margin.value().unwrap() > 0) {
      style.setMargin(
          static_cast<yoga::Edge>(edge),
          yoga::StyleLength::points(margin.value().unwrap() * scaleFactor_));
      modified = true;
    }
  }

  // Scale padding (only process absolute values)
  // Skip padding on axes where the node has no explicit dimension (flex
  // layout), because scaling such padding would change the content area and
  // interfere with the scaling of percentage-based children.
  for (int edge = static_cast<int>(yoga::Edge::Left);
       edge <= static_cast<int>(yoga::Edge::All);
       ++edge) {
    auto padEdge = static_cast<yoga::Edge>(edge);

    // Skip horizontal padding when node has no explicit width
    if (!hasExplicitWidth &&
        (padEdge == yoga::Edge::Left || padEdge == yoga::Edge::Right ||
         padEdge == yoga::Edge::Start || padEdge == yoga::Edge::End ||
         padEdge == yoga::Edge::Horizontal)) {
      continue;
    }
    // Skip vertical padding when node has no explicit height
    if (!hasExplicitHeight &&
        (padEdge == yoga::Edge::Top || padEdge == yoga::Edge::Bottom ||
         padEdge == yoga::Edge::Vertical)) {
      continue;
    }
    // Edge::All applies to all axes. When one axis has an explicit dimension
    // but the other does not, we cannot simply scale or skip Edge::All.
    // Instead, decompose it: scale only the axes that have explicit dimensions.
    if (padEdge == yoga::Edge::All) {
      auto padding = style.padding(yoga::Edge::All);
      if (padding.isPoints() && padding.value().isDefined() &&
          padding.value().unwrap() > 0) {
        if (hasExplicitWidth && hasExplicitHeight) {
          // Both axes explicit: scale Edge::All as before
          style.setPadding(
              yoga::Edge::All,
              yoga::StyleLength::points(
                  padding.value().unwrap() * scaleFactor_));
          modified = true;
        } else if (hasExplicitWidth && !hasExplicitHeight) {
          // Only width explicit: scale horizontal padding, keep vertical
          style.setPadding(
              yoga::Edge::Horizontal,
              yoga::StyleLength::points(
                  padding.value().unwrap() * scaleFactor_));
          modified = true;
        } else if (!hasExplicitWidth && hasExplicitHeight) {
          // Only height explicit: scale vertical padding, keep horizontal
          style.setPadding(
              yoga::Edge::Vertical,
              yoga::StyleLength::points(
                  padding.value().unwrap() * scaleFactor_));
          modified = true;
        }
        // Neither axis explicit: skip entirely
      }
      continue;
    }

    auto padding = style.padding(padEdge);
    if (padding.isPoints() && padding.value().isDefined() &&
        padding.value().unwrap() > 0) {
      style.setPadding(
          padEdge,
          yoga::StyleLength::points(padding.value().unwrap() * scaleFactor_));
      modified = true;
    }
  }

  if (modified) {
    node->setDirty(true);
  }

  // Recursively process child nodes
  // Pass whether current node or any ancestor has percentage width/height
  bool childParentHasPercentWidth =
      parentHasPercentWidth || currentHasPercentWidth;
  bool childParentHasPercentHeight =
      parentHasPercentHeight || currentHasPercentHeight;

  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    scaleYogaNodeStyleWithPercentHandling(
        child, childParentHasPercentWidth, childParentHasPercentHeight);
  }
}

void GuideLayout::collectModalSubtreeTags(YGNodeRef yogaNode) {
  if (!yogaNode) {
    return;
  }

  auto* node = toYogaNode(yogaNode);

  // Safety check: ensure context is valid
  void* context = node->getContext();
  if (context) {
    auto* shadowNode = static_cast<ShadowNode*>(context);
    if (shadowNode) {
      int32_t tag = shadowNode->getTag();
      {
        std::lock_guard<std::mutex> lock(mutex_);
        modalSubtreeTags_.insert(tag);

        // If it's a Paragraph node, mark it as needing content cache refresh
        const char* componentName = shadowNode->getComponentName();
        if (componentName && std::strcmp(componentName, "Paragraph") == 0) {
          needsContentRefreshTags_.insert(tag);
        }
      }
    }
  }

  // Recursively collect child nodes
  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    if (child) {
      collectModalSubtreeTags(child);
    }
  }
}

YGNodeRef GuideLayout::findModalNode(YGNodeRef rootYogaNode) {
  if (!rootYogaNode) {
    return nullptr;
  }

  auto* node = toYogaNode(rootYogaNode);

  // Safety check: ensure context is valid
  void* context = node->getContext();
  if (!context) {
    return nullptr;
  }

  // Get corresponding ShadowNode from YogaNode's context
  auto* shadowNode = static_cast<ShadowNode*>(context);
  if (!shadowNode) {
    return nullptr;
  }

  const char* componentName = shadowNode->getComponentName();
  if (componentName && std::strcmp(componentName, "ModalHostView") == 0) {
    return rootYogaNode;
  }

  // Recursively find child nodes
  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    if (child) {
      YGNodeRef found = findModalNode(child);
      if (found) {
        return found;
      }
    }
  }

  return nullptr;
}

std::pair<YGNodeRef, YGNodeRef> GuideLayout::findContentNode(
    YGNodeRef modalNode,
    float screenHeight) {
  // Returns {parent, contentNode} tracked during downward traversal.
  // Avoids getOwner() which can be a dangling pointer after a commit.
  if (!modalNode) {
    return {nullptr, nullptr};
  }

  auto pickChild = [](yoga::Node* parent) -> YGNodeRef {
    if (!parent)
      return nullptr;
    size_t count = parent->getChildCount();
    for (size_t i = 0; i < count; ++i) {
      YGNodeRef child = parent->getChild(i);
      if (child) {
        auto* c = static_cast<yoga::Node*>(child);
        if (c && c->getChildCount() > 0) {
          return child;
        }
      }
    }
    // fallback: return first child even if it has no children
    return (count > 0) ? parent->getChild(0) : nullptr;
  };

  auto* modal = toYogaNode(modalNode);
  if (!modal) {
    return {nullptr, nullptr};
  }

  YGNodeRef parent = modalNode;
  YGNodeRef current = pickChild(modal);
  while (current) {
    if (screenHeight > 0) {
      float h = YGNodeLayoutGetHeight(current);
      // First node whose height differs from screenHeight is the content node
      if (std::abs(h - screenHeight) > 1.0f) {
        return {parent, current};
      }
    }
    YGNodeRef next = pickChild(toYogaNode(current));
    if (!next) {
      break;
    }
    // Update parent only after confirming there is a deeper level to go to,
    // so that when we break or return early, parent always matches current.
    parent = current;
    current = next;
  }
  // Fallback: return the last traversed node (deepest we could reach).
  // If current is null (modal has no children at all), return {null, null}.
  return {current ? parent : nullptr, current};
}

float GuideLayout::calculateTopDistance(YGNodeRef contentNode) {
  // Calculate top distance: contentNode.top + firstChild.top
  const int DEFAULT_VALUE = 100;

  if (!contentNode) {
    return DEFAULT_VALUE;
  }

  auto* nodeContent = toYogaNode(contentNode);
  if (!nodeContent) {
    return DEFAULT_VALUE;
  }

  float contentNodeTop = YGNodeLayoutGetTop(contentNode);

  // Get first child of content node
  if (nodeContent->getChildCount() == 0) {
    return DEFAULT_VALUE;
  }
  YGNodeRef contentChild = nodeContent->getChild(0);
  if (!contentChild) {
    return DEFAULT_VALUE;
  }
  float contentChildTop = YGNodeLayoutGetTop(contentChild);

  return contentNodeTop + contentChildTop;
}

bool GuideLayout::checkModalNeedsScaling(
    YGNodeRef rootYogaNode,
    float screenHeight) {
  if (!rootYogaNode || !enableModalContentShrink_ || screenHeight <= 0) {
    return false;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (deviceType_ == "pc" || deviceType_ == "2in1" ||
        deviceType_ == "tablet") {
      return false;
    }
  }

  // Subtract real status bar height for scale calculation
  float statusBarHeight = 0.0f;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    statusBarHeight = statusBarHeightReceived_ ? statusBarHeight_ : 0.0f;
  }
  float availableHeight = screenHeight - statusBarHeight;
  if (availableHeight <= 0) {
    availableHeight = screenHeight;
  }

  // Cache available height and raw screen height
  {
    std::lock_guard<std::mutex> lock(mutex_);
    screenHeight_ = availableHeight;
    rawScreenHeight_ = screenHeight;
  }

  // Find Modal node
  YGNodeRef modalNode = findModalNode(rootYogaNode);
  if (!modalNode) {
    scaleFactor_ = 1.0f;
    return false;
  }

  // Get Modal Tag for dedup check
  auto* modalYogaNode = toYogaNode(modalNode);
  void* modalContext = modalYogaNode->getContext();
  int32_t modalTag = -1;
  if (modalContext) {
    auto* modalShadowNode = static_cast<ShadowNode*>(modalContext);
    if (modalShadowNode) {
      modalTag = modalShadowNode->getTag();
    }
  }

  // Get L4 node from Modal
  // During keyboard transitions, reuse cached result to avoid
  // reading KAV-distorted layout measurements.
  if (modalTag >= 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cacheIt = modalScaleCache_.find(modalTag);
    if (cacheIt != modalScaleCache_.end()) {
      auto timeSinceKbChange =
          std::chrono::steady_clock::now() - lastKeyboardChangeTime_;
      bool recentKeyboardChange =
          timeSinceKbChange < std::chrono::milliseconds(500);
      bool keyboardUp = keyboardHeight_ > 0 ||
          screenHeight < cacheIt->second.screenHeight - 1.0f ||
          recentKeyboardChange;
      if (keyboardUp) {
        scaleFactor_ = cacheIt->second.scaleFactor;
        topDistance_ = cacheIt->second.topDistance;
        return cacheIt->second.needsScale;
      }
    }
  }

  // findContentNode returns {parent, contentNode} from the traversal.
  auto [contentParent, contentNode] = findContentNode(modalNode, screenHeight);
  if (!contentNode) {
    // Content node not found, cannot proceed with scaling
    std::lock_guard<std::mutex> lock(mutex_);
    scaleFactor_ = 1.0f;
    return false;
  }

  // Find the tallest sibling in the content layer.
  // ScrollView nodes are skipped when comparing heights because their layout
  // height is bounded by their container, not their actual content, and would
  // produce an inaccurate scale factor.
  auto isScrollView = [](YGNodeRef node) -> bool {
    if (!node) {
      return false;
    }
    void* ctx = toYogaNode(node)->getContext();
    if (!ctx) {
      return false;
    }
    auto* sn = static_cast<ShadowNode*>(ctx);
    const char* name = sn ? sn->getComponentName() : nullptr;
    return name && std::strcmp(name, "ScrollView") == 0;
  };

  // Horizontal ScrollView indicates a carousel/swiper layout — skip scaling.
  auto isHorizontalScrollView = [&isScrollView](YGNodeRef node) -> bool {
    if (!isScrollView(node)) {
      return false;
    }
    auto* n = toYogaNode(node);
    return n->style().flexDirection() == yoga::FlexDirection::Row;
  };

  YGNodeRef tallestContentNode = contentNode;
  {
    auto* cpNode = contentParent ? toYogaNode(contentParent) : nullptr;
    if (cpNode) {
      float maxLayerHeight =
          isScrollView(contentNode) ? 0.0f : YGNodeLayoutGetHeight(contentNode);
      size_t siblingCount = cpNode->getChildCount();
      for (size_t i = 0; i < siblingCount; ++i) {
        YGNodeRef sibling = cpNode->getChild(i);
        if (!sibling) {
          continue;
        }
        auto* sibNode = toYogaNode(sibling);
        auto& sibStyle = sibNode->style();
        if (sibStyle.positionType() == yoga::PositionType::Absolute) {
          continue;
        }
        // Skip ScrollView nodes — their height is container-bounded
        if (isScrollView(sibling)) {
          continue;
        }
        float h = YGNodeLayoutGetHeight(sibling);
        if (h > maxLayerHeight) {
          maxLayerHeight = h;
          tallestContentNode = sibling;
        }
      }
    }
  }

  // Iterate all children of the tallest content node, skip absolute positioned
  // nodes, take max of (childTop + childHeight) for accurate content height.
  float maxContentHeight = 0;
  {
    auto* nodeTallest = toYogaNode(tallestContentNode);
    size_t contentChildCount = nodeTallest->getChildCount();
    for (size_t i = 0; i < contentChildCount; ++i) {
      YGNodeRef child = nodeTallest->getChild(i);
      if (!child) {
        continue;
      }
      auto* childNode = toYogaNode(child);
      auto& childStyle = childNode->style();
      if (childStyle.positionType() == yoga::PositionType::Absolute) {
        continue;
      }
      if (isHorizontalScrollView(child)) {
        std::lock_guard<std::mutex> lock(mutex_);
        scaleFactor_ = 1.0f;
        return false;
      }
      size_t grandChildCount = childNode->getChildCount();
      for (size_t j = 0; j < grandChildCount; ++j) {
        YGNodeRef grandChild = childNode->getChild(j);
        if (isHorizontalScrollView(grandChild)) {
          std::lock_guard<std::mutex> lock(mutex_);
          scaleFactor_ = 1.0f;
          return false;
        }
      }
      float childTop = YGNodeLayoutGetTop(child);
      float childHeight = YGNodeLayoutGetHeight(child);
      float childBottom = childTop + childHeight;
      if (childBottom > maxContentHeight) {
        maxContentHeight = childBottom;
      }
    }
  }

  // Check cache: if content hasn't changed, reuse cached result
  if (modalTag >= 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cacheIt = modalScaleCache_.find(modalTag);
    if (cacheIt != modalScaleCache_.end()) {
      bool contentChanged =
          std::abs(maxContentHeight - cacheIt->second.maxContentHeight) > 1.0f;
      bool screenChanged =
          std::abs(screenHeight - cacheIt->second.screenHeight) > 1.0f;
      if (!contentChanged && !screenChanged) {
        scaleFactor_ = cacheIt->second.scaleFactor;
        topDistance_ = cacheIt->second.topDistance;
        return cacheIt->second.needsScale;
      }
      // Content changed, clear cache and recalculate
      scaledModalTags_.erase(modalTag);
      modalScaleCache_.erase(cacheIt);
    }
  }

  // Calculate top distance: contentNode.top + firstChild.top
  topDistance_ = calculateTopDistance(contentNode);

  bool needsScale = maxContentHeight > screenHeight + 1;

  // Scale when topDistance < status bar height (real value or fallback 0 vp)
  constexpr float DEFAULT_STATUS_BAR_HEIGHT = 0.0f;
  const float MIN_TOP_THRESHOLD =
      statusBarHeightReceived_ ? statusBarHeight_ : DEFAULT_STATUS_BAR_HEIGHT;
  bool needsScaleForSmallTop =
      (topDistance_ < MIN_TOP_THRESHOLD && topDistance_ != 0);
  if (needsScale) {
    // Auto-calculate scale factor: available height / content height
    float calculatedScale =
        (maxContentHeight > 0) ? availableHeight / maxContentHeight : 1.0f;
    // Scale factor cannot be less than MIN_SCALE_FACTOR (0.85)
    scaleFactor_ = (calculatedScale < MIN_SCALE_FACTOR) ? MIN_SCALE_FACTOR
                                                        : calculatedScale;
  } else if (needsScaleForSmallTop) {
    // Scale factor = (screenHeight - statusBarHeight) / (screenHeight -
    // topDistance_)
    float adjustedAvailableHeight = screenHeight - MIN_TOP_THRESHOLD;
    float adjustedContentHeight = screenHeight - topDistance_;
    float calculatedScale = (adjustedContentHeight > 0)
        ? adjustedAvailableHeight / adjustedContentHeight
        : 1.0f;
    calculatedScale =
        (calculatedScale < scaleFactor_) ? calculatedScale : scaleFactor_;
    // Scale factor cannot be less than MIN_SCALE_FACTOR (0.85)
    scaleFactor_ = (calculatedScale < MIN_SCALE_FACTOR) ? MIN_SCALE_FACTOR
                                                        : calculatedScale;
    needsScale = true;
  } else {
    // No scaling needed, reset to 1.0
    scaleFactor_ = 1.0f;
  }

  // Cache the calculated scale info and mark as scaled
  if (modalTag >= 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    ModalScaleInfo info;
    info.scaleFactor = scaleFactor_;
    info.topDistance = topDistance_;
    info.maxContentHeight = maxContentHeight;
    info.screenHeight = screenHeight;
    info.needsScale = needsScale;
    modalScaleCache_[modalTag] = info;
    if (needsScale) {
      scaledModalTags_.insert(modalTag);
    }
  }

  return needsScale;
}

// Check if a given Modal node has already been scaled.
// Returns true if the Modal's tag is in scaledModalTags_, indicating
// it was scaled in a previous layout pass.
bool GuideLayout::isModalAlreadyScaled(YGNodeRef modalNode) {
  if (!modalNode) {
    return false;
  }
  auto* node = toYogaNode(modalNode);
  void* context = node->getContext();
  if (!context) {
    return false;
  }
  auto* shadowNode = static_cast<ShadowNode*>(context);
  if (!shadowNode) {
    return false;
  }
  int32_t tag = shadowNode->getTag();
  std::lock_guard<std::mutex> lock(mutex_);
  return scaledModalTags_.find(tag) != scaledModalTags_.end();
}

// Restore original styles for all nodes in Modal's subtree.
// This is called before re-layout to ensure checkModalNeedsScaling reads
// accurate original maxContentHeight (not scaled values from previous frame).
// modalNode: The Modal node whose children need style restoration
void GuideLayout::restoreModalSubtreeStyles(YGNodeRef modalNode) {
  if (!modalNode) {
    return;
  }
  auto* node = toYogaNode(modalNode);
  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    if (child) {
      restoreOriginalStyles(child);
    }
  }
}

void GuideLayout::restoreOriginalStyles(YGNodeRef yogaNode) {
  if (!yogaNode) {
    return;
  }

  auto* node = toYogaNode(yogaNode);

  // Get the ShadowNode from context to access original props
  void* context = node->getContext();
  if (context) {
    auto* shadowNode = static_cast<ShadowNode*>(context);
    if (shadowNode) {
      auto props = shadowNode->getProps();
      if (props) {
        auto* yogaProps = dynamic_cast<const YogaStylableProps*>(props.get());
        if (yogaProps != nullptr) {
          // Restore the original yogaStyle from props
          // This undoes any previous scaling that was applied to yogaNode_
          node->setStyle(yogaProps->yogaStyle);
        }
      }
    }
  }

  // Recursively restore children
  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    if (child) {
      restoreOriginalStyles(child);
    }
  }
}

void GuideLayout::clearModalScalingState(YGNodeRef modalNode) {
  if (!modalNode) {
    return;
  }

  // Collect all Paragraph nodes in the Modal subtree so their content
  // caches get invalidated (needsContentRefreshTags_ is populated inside
  // collectModalSubtreeTags). After collection we move only the refresh
  // tags out, then clear the rest of the scaling bookkeeping.
  {
    std::lock_guard<std::mutex> lock(mutex_);
    modalSubtreeTags_.clear();
    needsContentRefreshTags_.clear();
  }

  // Re-collect subtree tags – this fills both modalSubtreeTags_ and
  // needsContentRefreshTags_ for every Paragraph under the Modal.
  auto* node = toYogaNode(modalNode);
  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    if (child) {
      collectModalSubtreeTags(child);
    }
  }

  // Now clear all scaling state but keep needsContentRefreshTags_ so that
  // ParagraphShadowNode::getContent() will invalidate its cached content
  // (which still holds the old scaled font sizes).
  {
    std::lock_guard<std::mutex> lock(mutex_);
    modalSubtreeTags_.clear();
    modalScaleCache_.clear();
    scaledModalTags_.clear();
    scaleFactor_ = 1.0f;
    topDistance_ = 0.0f;
  }
}

void GuideLayout::resetYogaTreeState(YGNodeRef nodeRef) {
  if (!nodeRef) {
    return;
  }

  auto* node = toYogaNode(nodeRef);

  node->setHasNewLayout(false);
  node->setDirty(true);

  size_t childCount = node->getChildCount();
  for (size_t i = 0; i < childCount; ++i) {
    YGNodeRef child = node->getChild(i);
    if (child) {
      resetYogaTreeState(child);
    }
  }
}

bool GuideLayout::isModalSubtreeStyleDirty(YGNodeRef modalNode) {
  if (!modalNode) {
    return false;
  }

  YGNodeRef contentNode = findContentNode(modalNode, rawScreenHeight_).second;
  if (!contentNode) {
    return false;
  }

  auto* node = toYogaNode(contentNode);
  auto* ctx = node->getContext();
  auto* shadowNode = ctx ? static_cast<ShadowNode*>(ctx) : nullptr;
  if (!shadowNode) {
    return false;
  }

  auto props = shadowNode->getProps();
  auto* yogaProps =
      props ? dynamic_cast<const YogaStylableProps*>(props.get()) : nullptr;
  if (!yogaProps) {
    return false;
  }

  // Compare content node's live yoga style dimensions against the authored
  // values from props. A mismatch means stale scaled styles were carried over
  // by ShadowNode cloning.
  const auto& cur = node->style();
  const auto& orig = yogaProps->yogaStyle;
  return cur.dimension(yoga::Dimension::Height) !=
      orig.dimension(yoga::Dimension::Height) ||
      cur.dimension(yoga::Dimension::Width) !=
      orig.dimension(yoga::Dimension::Width);
}

} // namespace react
} // namespace facebook