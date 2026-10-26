/**
 * RNOH-New: TurboModule provider interface
 *
 * This is the bridge between JS TurboModule calls and native implementations.
 * Each TurboModule exposes a set of methods that can be called from JS.
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

namespace rnoh_new {

/**
 * Base class for all TurboModule implementations
 */
class RNHTurboModule {
public:
    virtual ~RNHTurboModule() = default;

    // Module name (must match JS-side name)
    virtual std::string getName() const = 0;

    // Initialize the module
    virtual void initialize() {}

    // Cleanup
    virtual void invalidate() {}
};

/**
 * Factory function type for creating TurboModule instances
 */
using RNHTurboModuleFactory = std::function<std::shared_ptr<RNHTurboModule>()>;

/**
 * Provider that manages all registered TurboModules
 */
class RNHTurboModuleProvider {
public:
    virtual ~RNHTurboModuleProvider() = default;

    // Register a TurboModule factory
    virtual void registerModule(const std::string& name,
                                RNHTurboModuleFactory factory) = 0;

    // Get a TurboModule by name
    virtual std::shared_ptr<RNHTurboModule> getModule(
        const std::string& name) = 0;

    // Check if a module exists
    virtual bool hasModule(const std::string& name) const = 0;

    // Get all registered module names
    virtual std::vector<std::string> getModuleNames() const = 0;
};

} // namespace rnoh_new
