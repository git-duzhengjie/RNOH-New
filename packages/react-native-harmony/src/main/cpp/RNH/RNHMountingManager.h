/**
 * RNOH-New: Fabric Mounting Manager
 *
 * Receives mutation instructions from the Fabric renderer (C++ side)
 * and applies them to the actual ArkUI component tree.
 * This is the bridge between React's virtual DOM and HarmonyOS native UI.
 *
 * Performance optimizations:
 *  - Batch mutation coalescing: same-tag / same-type mutations are merged
 *    before being applied, reducing redundant ArkUI node operations.
 *  - Props merge: multiple UPDATE_PROPS for the same tag collapse into
 *    one (last-write-wins), avoiding repeated JSON parsing on ArkUI side.
 *  - Layout merge: multiple UPDATE_LAYOUT for the same tag collapse into one.
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

namespace rnoh_new {

class RNHComponentInstance;
class RNHComponentDescriptorProvider;

/**
 * Mutation types that Fabric can send
 */
enum class RNHMutationType {
    CREATE,           // Create a new component instance
    DELETE,           // Delete a component instance
    INSERT,           // Insert a child into a parent
    REMOVE,           // Remove a child from a parent
    UPDATE_PROPS,     // Update component properties
    UPDATE_LAYOUT,    // Update layout metrics
    UPDATE_STATE,     // Update component state
    UPDATE_EVENT_EMITTER, // Update event emitter
};

/**
 * A single mutation instruction from Fabric
 */
struct RNHMutation {
    RNHMutationType type;
    int tag;              // Target component tag
    int parentTag;        // Parent component tag (for INSERT/REMOVE)
    int index;            // Child index (for INSERT/REMOVE)
    std::string componentName; // Component type name (for CREATE)
    std::string propsJson;     // Serialized props (for UPDATE_PROPS)
    float x, y, width, height; // Layout (for UPDATE_LAYOUT)
    std::string stateJson;     // Serialized state (for UPDATE_STATE)
};

/**
 * MountingManager processes a batch of mutations from Fabric's
 * ShadowTree commit and applies them to the native component tree.
 *
 * Before applying, mutations are optimized:
 *  - Duplicate UPDATE_PROPS / UPDATE_LAYOUT / UPDATE_STATE for the
 *    same tag are collapsed (last-write-wins).
 *  - CREATE immediately followed by DELETE for the same tag cancels out.
 *  - INSERT + REMOVE pairs (same parent, same child, same index) cancel out.
 */
class RNHMountingManager {
public:
    RNHMountingManager(
        std::shared_ptr<RNHComponentDescriptorProvider> descriptorProvider);
    ~RNHMountingManager();

    // Process a batch of mutations (called from Fabric scheduler)
    // Optimizes the batch internally before applying.
    void processMutations(const std::vector<RNHMutation>& mutations);

    // Get a component instance by tag
    std::shared_ptr<RNHComponentInstance> getComponentByTag(int tag) const;

    // Register a pre-created component (e.g., root view)
    void registerComponent(int tag,
                           std::shared_ptr<RNHComponentInstance> component);

    // Unregister a component
    void unregisterComponent(int tag);

    // Clear all components
    void clear();

    // Set the root component tag
    void setRootTag(int tag) { rootTag_ = tag; }
    int getRootTag() const { return rootTag_; }

private:
    // -----------------------------------------------------------------------
    // Batch optimization (internal, called with lock held)
    // -----------------------------------------------------------------------

    /**
     * Coalesce a raw mutation list into an optimized list.
     * Rules:
     *  - Same-tag UPDATE_PROPS: keep only the last one
     *  - Same-tag UPDATE_LAYOUT: keep only the last one
     *  - Same-tag UPDATE_STATE: keep only the last one
     *  - Same-tag UPDATE_EVENT_EMITTER: keep only the last one
     *  - CREATE + DELETE for same tag → both removed (component never existed)
     *  - INSERT + REMOVE for same (parentTag, tag) → both removed
     *    (but only if index matches; otherwise kept as-is)
     */
    std::vector<RNHMutation> coalesceMutations(
        const std::vector<RNHMutation>& mutations) const;

    // -----------------------------------------------------------------------
    // Individual mutation handlers (internal, called with lock held)
    // -----------------------------------------------------------------------

    void handleCreate(const RNHMutation& mutation);
    void handleDelete(const RNHMutation& mutation);
    void handleInsert(const RNHMutation& mutation);
    void handleRemove(const RNHMutation& mutation);
    void handleUpdateProps(const RNHMutation& mutation);
    void handleUpdateLayout(const RNHMutation& mutation);
    void handleUpdateState(const RNHMutation& mutation);
    void handleUpdateEventEmitter(const RNHMutation& mutation);

    // Component registry: tag -> instance
    std::unordered_map<int, std::shared_ptr<RNHComponentInstance>>
        componentRegistry_;

    // Descriptor provider for creating components
    std::shared_ptr<RNHComponentDescriptorProvider> descriptorProvider_;

    // Root component tag
    int rootTag_ = -1;

    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace rnoh_new
