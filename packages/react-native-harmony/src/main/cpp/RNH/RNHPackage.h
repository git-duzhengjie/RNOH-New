/**
 * RNOH-New: Package system
 *
 * A Package is a collection of related TurboModules and Component
 * Descriptors that can be registered together. Similar to ReactPackage
 * in React Native Android.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>

namespace rnoh_new {

class RNHTurboModuleProvider;
class RNHComponentDescriptorProvider;

/**
 * Base class for all RNOH-New packages.
 * Subclass this to create a package that provides native modules
 * and/or custom components.
 */
class RNHPackage {
public:
    virtual ~RNHPackage() = default;

    // Package name
    virtual std::string getName() const = 0;

    // Package version
    virtual std::string getVersion() const { return "0.1.0"; }

    // Register TurboModules into the provider
    virtual void registerTurboModules(
        RNHTurboModuleProvider* provider) {}

    // Register Component Descriptors into the provider
    virtual void registerComponentDescriptors(
        RNHComponentDescriptorProvider* provider) {}

    // Called when the package is registered
    virtual void onRegister() {}

    // Called when the package is unregistered
    virtual void onUnregister() {}
};

/**
 * Registry that manages all registered packages
 */
class RNHPackageRegistry {
public:
    RNHPackageRegistry();
    ~RNHPackageRegistry();

    // Register a package
    void registerPackage(std::shared_ptr<RNHPackage> package);

    // Unregister a package by name
    void unregisterPackage(const std::string& name);

    // Get all registered packages
    std::vector<std::shared_ptr<RNHPackage>> getPackages() const;

    // Check if a package is registered
    bool hasPackage(const std::string& name) const;

    // Register all packages into providers
    void registerAll(RNHTurboModuleProvider* turboModuleProvider,
                     RNHComponentDescriptorProvider* componentProvider);

private:
    std::vector<std::shared_ptr<RNHPackage>> packages_;
};

} // namespace rnoh_new
