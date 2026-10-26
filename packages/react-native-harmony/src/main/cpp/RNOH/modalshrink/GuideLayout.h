/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef GUIDE_LAYOUT_H
#define GUIDE_LAYOUT_H

#include <yoga/Yoga.h>
#include <yoga/node/Node.h>
#include <yoga/style/Style.h>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace facebook {
namespace react {

class GuideLayout {
 public:
  // Get singleton instance
  static GuideLayout& getInstance() {
    static GuideLayout instance; // Thread-safe singleton initialization
    return instance;
  }

  // Scaling function with percentage handling
  // parentHasPercentWidth: whether any ancestor node has percentage width
  // parentHasPercentHeight: whether any ancestor node has percentage height
  // If parent has percentage, child's percentage won't be scaled (it inherits
  // from parent's scaling)
  void scaleYogaNodeStyleWithPercentHandling(

      YGNodeRef yogaNode,
      bool parentHasPercentWidth,
      bool parentHasPercentHeight);

  // Scan Yoga tree, find Modal nodes and scale their subtrees
  // Returns whether Modal was found and processed
  bool scanAndScaleModalSubtrees(YGNodeRef rootYogaNode);

  // Find Modal node
  YGNodeRef findModalNode(YGNodeRef rootYogaNode);

  // Returns {parent, contentNode}: first child layer whose height differs from
  // screenHeight, parent tracked during traversal to avoid getOwner() which
  // may be a dangling pointer after a shadow tree commit.
  std::pair<YGNodeRef, YGNodeRef> findContentNode(
      YGNodeRef modalNode,
      float screenHeight);

  // Check if Modal needs scaling (based on post-layout height)
  // If scaling is needed, automatically calculates and saves scale factor
  // Returns: true indicates scaling is needed
  bool checkModalNeedsScaling(YGNodeRef rootYogaNode, float screenHeight);

  // Reset Yoga tree layout state (used for recalculation)
  void resetYogaTreeState(YGNodeRef node);

  // Check if a Modal has already been scaled (by its YGNodeRef)
  bool isModalAlreadyScaled(YGNodeRef modalNode);

  // Detect stale scaled styles carried over by ShadowNode cloning.
  bool isModalSubtreeStyleDirty(YGNodeRef modalNode);

  // Restore original styles for Modal's subtree (used before relayout
  // to get accurate original maxContentHeight)
  void restoreModalSubtreeStyles(YGNodeRef modalNode);

  // Status bar height (set from ArkTS via postMessageToCpp)
  void setStatusBarHeight(float height) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (statusBarHeight_ != height) {
      // Height changed: clear cache so next layout re-evaluates with new value
      modalScaleCache_.clear();
      scaledModalTags_.clear();
      statusBarHeight_ = height;
    }
    statusBarHeightReceived_ = true;
  }
  float getStatusBarHeight() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return statusBarHeight_;
  }

  // Keyboard height (set from ArkTS via postMessageToCpp)
  void setKeyboardHeight(float height) {
    std::lock_guard<std::mutex> lock(mutex_);
    keyboardHeight_ = height;
    lastKeyboardChangeTime_ = std::chrono::steady_clock::now();
  }
  bool isKeyboardVisible() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return keyboardHeight_ > 0;
  }
  float getKeyboardHeight() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return keyboardHeight_;
  }

  // Device type (set from ArkTS via setDeviceInfo; used to skip scaling on PC)
  void setDeviceType(const std::string& deviceType) {
    std::lock_guard<std::mutex> lock(mutex_);
    deviceType_ = deviceType;
  }

  // Set Modal content scaling feature toggle
  void setModalContentShrinkEnabled(bool enabled) {
    enableModalContentShrink_ = enabled;
  }

  // Check if Modal scaling is enabled
  bool isModalContentShrinkEnabled() const {
    return enableModalContentShrink_;
  }

  // Get scale factor (automatically calculated based on content overflow)
  float getScaleFactor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return scaleFactor_;
  }

  // Get cached screen height
  float getScreenHeight() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return screenHeight_;
  }

  // Get top distance (contentNode.top + firstChild.top in Modal subtree)
  float getTopDistance() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return topDistance_;
  }

  // Check if a node is in Modal subtree (used for font scaling)
  bool isInModalSubtree(int32_t tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return modalSubtreeTags_.find(tag) != modalSubtreeTags_.end();
  }

  // Check if a node needs content cache refresh (used for Paragraph font
  // scaling)
  bool needsContentRefresh(int32_t tag) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return needsContentRefreshTags_.find(tag) != needsContentRefreshTags_.end();
  }

  // Mark that a node's content cache has been refreshed
  void markContentRefreshed(int32_t tag) {
    std::lock_guard<std::mutex> lock(mutex_);
    needsContentRefreshTags_.erase(tag);
  }

  // Clear all records when Modal no longer exists
  void clearAllModalState() {
    std::lock_guard<std::mutex> lock(mutex_);
    modalSubtreeTags_.clear();
    needsContentRefreshTags_.clear();
    modalScaleCache_.clear();
    scaledModalTags_.clear();
    topDistance_ = 0.0f;
  }

  // Reset Modal subtree records (called before rescaling)
  void resetModalSubtreeTags() {
    std::lock_guard<std::mutex> lock(mutex_);
    modalSubtreeTags_.clear();
    needsContentRefreshTags_.clear();
  }

  // Clear Modal scaling state when transitioning from scaled to unscaled.
  // Forces Paragraph nodes to refresh their cached content (which contains
  // scaled font sizes) and resets all scaling-related tracking.
  void clearModalScalingState(YGNodeRef modalNode);

  // Delete copy constructor and assignment operator
  GuideLayout(const GuideLayout&) = delete;
  GuideLayout& operator=(const GuideLayout&) = delete;

 private:
  // Mutex for thread-safe access to shared data
  mutable std::mutex mutex_;

  // Minimum scale factor limit
  static constexpr float MIN_SCALE_FACTOR = 0.85f;

  // Auto-calculated scale factor (default 1.0 means no scaling)
  float scaleFactor_ = 1.0f;

  // Cached screen height
  float screenHeight_ = 0.0f;

  // Raw (full) screen height — stored before SYSTEM_BARS_HEIGHT is subtracted.
  // Used by findContentNode when screenHeight is not passed as a parameter.
  float rawScreenHeight_ = 0.0f;

  // Top distance: contentNode.top + firstChild.top in Modal subtree
  float topDistance_ = 0.0f;

  bool enableModalContentShrink_ = false;

  // Track all node Tags in Modal subtree (used for font scaling judgment)
  std::unordered_set<int32_t> modalSubtreeTags_;

  // Track Paragraph nodes that need content cache refresh
  std::unordered_set<int32_t> needsContentRefreshTags_;

  // Cached scale info for each Modal (key: Modal Tag)
  struct ModalScaleInfo {
    float scaleFactor = 1.0f;
    float topDistance = 0.0f;
    float maxContentHeight = 0.0f;
    float screenHeight = 0.0f;
    bool needsScale = false;
  };
  std::unordered_map<int32_t, ModalScaleInfo> modalScaleCache_;

  // Track which Modals have already been scaled (by Modal Tag)
  // Prevents repeated scaling on subsequent layoutTree calls
  std::unordered_set<int32_t> scaledModalTags_;

  // Status bar height from ArkTS layer (vp units)
  float statusBarHeight_ = 0.0f;
  bool statusBarHeightReceived_ = false;

  // Device type string (e.g. "phone", "tablet", "pc", "2in1")
  std::string deviceType_;

  // Restore original yoga styles from ShadowNode props
  // Used before re-scaling to ensure consistent state
  void restoreOriginalStyles(YGNodeRef yogaNode);

  // Collect all node Tags in Modal subtree
  void collectModalSubtreeTags(YGNodeRef yogaNode);

  // Calculate top distance: contentNode.top + firstChild.top
  float calculateTopDistance(YGNodeRef contentNode);

  // Keyboard height from ArkTS layer (0 = hidden)
  float keyboardHeight_ = 0.0f;

  // Guards against cross-thread races during keyboard transitions
  std::chrono::steady_clock::time_point lastKeyboardChangeTime_{};

  // Private constructor
  GuideLayout() = default;
};

} // namespace react
} // namespace facebook

#endif // GUIDE_LAYOUT_H