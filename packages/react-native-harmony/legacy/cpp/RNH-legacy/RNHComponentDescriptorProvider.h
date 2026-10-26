/**
 * RNOH-New: Component Descriptor Provider
 *
 * Manages Fabric component descriptors that map JS component names
 * to native ArkUI component instances.
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

namespace rnoh_new {

// Forward declarations
class RNHComponentInstance;

/**
 * Describes a Fabric component: its name, props, events, and
 * how to create/mount the native counterpart.
 */
class RNHComponentDescriptor {
public:
    virtual ~RNHComponentDescriptor() = default;

    // Component name (e.g., "View", "Text", "Image")
    virtual std::string getComponentName() const = 0;

    // Component handle (unique type identifier)
    virtual std::string getComponentHandle() const = 0;

    // Create native component instance
    virtual std::shared_ptr<RNHComponentInstance> createInstance() = 0;
};

/**
 * Factory function for creating component descriptors
 */
using RNHComponentDescriptorFactory =
    std::function<std::shared_ptr<RNHComponentDescriptor>()>;

/**
 * Provider that manages all registered component descriptors
 */
class RNHComponentDescriptorProvider {
public:
    virtual ~RNHComponentDescriptorProvider() = default;

    // Register a component descriptor
    virtual void registerDescriptor(const std::string& name,
                                    RNHComponentDescriptorFactory factory) = 0;

    // Get a descriptor by component name
    virtual std::shared_ptr<RNHComponentDescriptor> getDescriptor(
        const std::string& name) = 0;

    // Check if a descriptor exists
    virtual bool hasDescriptor(const std::string& name) const = 0;

    // Get all registered component names
    virtual std::vector<std::string> getComponentNames() const = 0;
};

} // namespace rnoh_new
