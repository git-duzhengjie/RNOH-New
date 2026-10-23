/**
 * RNOH-New: React Native for HarmonyOS
 * Core instance management - RNHInstance
 *
 * Each RNHInstance corresponds to one JS bundle execution context.
 * It owns the React renderer, scheduler, and native module registry.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <js_native_api.h>
#include <js_native_api_types.h>

namespace rnoh_new {

class RNHInstance;
class RNHTurboModuleProvider;
class RNHComponentDescriptorProvider;

using RNHInstanceRef = std::shared_ptr<RNHInstance>;
using RNHInstanceId = uint32_t;

/**
 * Configuration for creating a new RNH instance
 */
struct RNHInstanceConfig {
    // JS bundle source
    std::string bundlePath;
    std::string bundleUrl;

    // Feature flags
    bool enableHermes = true;
    bool enableFabric = true;
    bool enableTurboModules = true;
    bool enableBridgeless = true;

    // Debugging
    bool enableInspector = false;
    bool enableRemoteDebugging = false;
    uint16_t inspectorPort = 9229;

    // Surface config
    float viewportWidth = 0.0f;
    float viewportHeight = 0.0f;
    float pixelRatio = 1.0f;

    // Custom providers
    std::shared_ptr<RNHTurboModuleProvider> turboModuleProvider;
    std::shared_ptr<RNHComponentDescriptorProvider> componentDescriptorProvider;
};

/**
 * Core instance that manages the lifecycle of a React Native runtime.
 * This is the central hub that connects:
 * - JS engine (Hermes)
 * - Fabric renderer (MountingManager, Scheduler)
 * - TurboModule registry
 * - NAPI bridge (ArkTS <-> C++)
 */
class RNHInstance {
public:
    RNHInstance();
    ~RNHInstance();

    // Lifecycle
    bool initialize(const RNHInstanceConfig& config);
    void destroy();
    bool isInitialized() const { return initialized_; }

    // Bundle management
    bool loadBundle(const std::string& bundlePath);
    bool loadBundleFromUrl(const std::string& url);
    void reloadBundle();

    // Surface management
    void startSurface(int surfaceId, const std::string& moduleName,
                      float width, float height, float pixelRatio);
    void stopSurface(int surfaceId);
    void updateSurfaceSize(int surfaceId, float width, float height);

    // Event handling
    void emitEvent(int surfaceId, const std::string& eventName,
                   const std::string& payload);
    void sendTouchEvent(int surfaceId, int action, float x, float y,
                        int64_t timestamp);

    // TurboModule
    void registerTurboModule(const std::string& name,
                             std::function<void*()> factory);
    void* getTurboModule(const std::string& name) const;

    // NAPI bridge
    void setNapiEnv(napi_env env) { napiEnv_ = env; }
    napi_env getNapiEnv() const { return napiEnv_; }

    // Inspector
    void enableInspector(uint16_t port = 9229);
    void disableInspector();

    // Memory management
    void notifyMemoryPressure(int level);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool initialized_ = false;
    napi_env napiEnv_ = nullptr;
    RNHInstanceConfig config_;
};

/**
 * Factory function to create a new RNH instance
 */
RNHInstanceRef createRNHInstance(const RNHInstanceConfig& config);

} // namespace rnoh_new
