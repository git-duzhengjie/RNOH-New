/**
 * RNOH-New: Component Instance
 *
 * Represents a mounted native component (ArkUI node) that corresponds
 * to a React Fabric component. Handles props updates, layout, and events.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace rnoh_new {

// Forward declarations
class RNHArkUINode;
class RNHEventDispatcher;

/**
 * Base class for all native component instances.
 * Each subclass corresponds to one Fabric component type.
 */
class RNHComponentInstance {
public:
    virtual ~RNHComponentInstance() = default;

    // --- Lifecycle ---

    // Called when the component is created (before first mount)
    virtual void onCreate() {}

    // Called when the component is about to be destroyed
    virtual void onDestroy() {}

    // Called after the component is attached to the tree
    virtual void onAttach() {}

    // Called after the component is detached from the tree
    virtual void onDetach() {}

    // --- Props ---

    // Update component props (called when JS props change)
    virtual void updateProps(const std::string& propsJson) = 0;

    // --- Layout ---

    // Measure the component's desired size
    virtual void measure(float widthConstraint, float heightConstraint,
                         float& outWidth, float& outHeight) {}

    // Layout the component at the given position
    virtual void layout(float x, float y, float width, float height) {}

    // --- Children ---

    // Add a child component
    virtual void addChild(std::shared_ptr<RNHComponentInstance> child,
                          int index) {}

    // Remove a child component
    virtual void removeChild(std::shared_ptr<RNHComponentInstance> child) {}

    // --- Events ---

    // Dispatch an event to JS
    virtual void dispatchEvent(const std::string& eventName,
                               const std::string& payload) {}

    // --- ArkUI Node ---

    // Get the underlying ArkUI node handle
    virtual RNHArkUINode* getArkUINode() { return nullptr; }

    // --- State ---

    // Get the component's tag (unique ID from Fabric)
    virtual int getTag() const { return tag_; }
    virtual void setTag(int tag) { tag_ = tag; }

    // Get the component name
    virtual std::string getComponentName() const = 0;

protected:
    int tag_ = 0;
};

/**
 * Factory function for creating component instances
 */
using RNHComponentInstanceFactory =
    std::function<std::shared_ptr<RNHComponentInstance>()>;

} // namespace rnoh_new
