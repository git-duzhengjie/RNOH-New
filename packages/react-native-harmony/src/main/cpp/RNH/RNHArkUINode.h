/**
 * RNOH-New: ArkUI Node abstraction
 *
 * Thin C++ wrapper around HarmonyOS ArkUI native node APIs.
 * Provides a type-safe interface for creating and manipulating
 * ArkUI components from C++.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>

// ArkUI Native API forward declarations
struct ArkUI_NodeHandle;
struct ArkUI_NativeNodeAPI_1;

namespace rnoh_new {

/**
 * ArkUI Node types that we support
 */
enum class RNHArkUINodeType {
    // Layout
    COLUMN,
    ROW,
    STACK,
    FLEX,
    LIST,
    GRID,
    SCROLL,

    // Basic
    TEXT,
    TEXT_INPUT,
    TEXT_AREA,
    IMAGE,
    BUTTON,
    CHECKBOX,
    RADIO,
    TOGGLE,
    SLIDER,
    PROGRESS,
    LOADING_PROGRESS,

    // Container
    SWIPER,
    TABS,
    NAVIGATION,

    // Custom (for user-defined components)
    CUSTOM,
};

/**
 * Thin wrapper around an ArkUI native node.
 * Handles node creation, property setting, and event registration.
 */
class RNHArkUINode {
public:
    RNHArkUINode(RNHArkUINodeType type);
    ~RNHArkUINode();

    // Non-copyable
    RNHArkUINode(const RNHArkUINode&) = delete;
    RNHArkUINode& operator=(const RNHArkUINode&) = delete;

    // Movable
    RNHArkUINode(RNHArkUINode&&) noexcept;
    RNHArkUINode& operator=(RNHArkUINode&&) noexcept;

    // --- Node Management ---

    // Get the raw ArkUI node handle
    ArkUI_NodeHandle* getHandle() const { return handle_; }

    // Get node type
    RNHArkUINodeType getType() const { return type_; }

    // --- Children ---

    // Add a child node at the given index
    void addChild(RNHArkUINode* child, int index = -1);

    // Remove a child node
    void removeChild(RNHArkUINode* child);

    // Remove all children
    void removeAllChildren();

    // Get child count
    int getChildCount() const { return static_cast<int>(children_.size()); }

    // --- Properties ---

    // Layout properties
    void setWidth(float width);
    void setHeight(float height);
    void setPosition(float x, float y);
    void setMargin(float left, float top, float right, float bottom);
    void setPadding(float left, float top, float right, float bottom);

    // Visual properties
    void setBackgroundColor(uint32_t color);
    void setOpacity(float opacity);
    void setBorderRadius(float radius);
    void setBorderWidth(float width);
    void setBorderColor(uint32_t color);
    void setClip(bool clip);
    void setVisibility(bool visible);

    // Flex properties
    void setFlexDirection(int direction);
    void setJustifyContent(int justify);
    void setAlignItems(int align);
    void setFlexGrow(float grow);
    void setFlexShrink(float shrink);
    void setFlexBasis(float basis);
    void setAlignSelf(int align);

    // Text properties (for TEXT nodes)
    void setText(const std::string& text);
    void setFontSize(float size);
    void setFontWeight(int weight);
    void setFontColor(uint32_t color);
    void setTextAlign(int align);
    void setMaxLines(int maxLines);
    void setLineHeight(float height);

    // Image properties (for IMAGE nodes)
    void setImageSource(const std::string& src);
    void setImageResizeMode(int mode);

    // --- Events ---

    // Touch event callback
    using TouchEventCallback = std::function<void(int action, float x, float y)>;
    void setOnTouchCallback(TouchEventCallback callback);

    // Click event callback
    using ClickEventCallback = std::function<void()>;
    void setOnClickCallback(ClickEventCallback callback);

    // --- Measurement ---

    // Measure the node's desired size
    void measure(float widthConstraint, float heightConstraint,
                 float& outWidth, float& outHeight);

    // Get measured size
    float getMeasuredWidth() const { return measuredWidth_; }
    float getMeasuredHeight() const { return measuredHeight_; }

private:
    ArkUI_NodeHandle* handle_;
    RNHArkUINodeType type_;
    std::vector<RNHArkUINode*> children_;
    float measuredWidth_ = 0.0f;
    float measuredHeight_ = 0.0f;

    // Create the native ArkUI node
    void createNode();

    // Destroy the native node
    void destroyNode();

    // Get the native node API
    static ArkUI_NativeNodeAPI_1* getNodeAPI();
};

} // namespace rnoh_new
