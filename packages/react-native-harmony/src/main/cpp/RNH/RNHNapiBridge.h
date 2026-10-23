/**
 * RNOH-New: NAPI Bridge
 *
 * Provides the NAPI (Node API) bridge between ArkTS (ETS) and C++.
 * This is the primary communication channel between the ArkTS UI layer
 * and the C++ React Native runtime.
 */

#pragma once

#include <js_native_api.h>
#include <js_native_api_types.h>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace rnoh_new {

class RNHInstance;

/**
 * NAPI Bridge - handles all communication between ArkTS and C++
 *
 * Registered methods are callable from ArkTS via:
 *   import rnohNew from 'librnh.so'
 *   rnohNew.methodName(...args)
 */
class RNHNapiBridge {
public:
    RNHNapiBridge(napi_env env, napi_value exports);
    ~RNHNapiBridge();

    // Initialize the bridge
    bool initialize();

    // Get the NAPI exports object
    napi_value getExports() const { return exports_; }

    // Set the RNH instance
    void setInstance(std::shared_ptr<RNHInstance> instance);

    // --- NAPI Methods (callable from ArkTS) ---

    // Instance lifecycle
    static napi_value createInstance(napi_env env, napi_callback_info info);
    static napi_value destroyInstance(napi_env env, napi_callback_info info);

    // Bundle loading
    static napi_value loadBundle(napi_env env, napi_callback_info info);
    static napi_value loadBundleFromUrl(napi_env env, napi_callback_info info);

    // Surface management
    static napi_value startSurface(napi_env env, napi_callback_info info);
    static napi_value stopSurface(napi_env env, napi_callback_info info);
    static napi_value updateSurfaceSize(napi_env env, napi_callback_info info);

    // Events
    static napi_value emitEvent(napi_env env, napi_callback_info info);
    static napi_value sendTouchEvent(napi_env env, napi_callback_info info);

    // TurboModule
    static napi_value getTurboModule(napi_env env, napi_callback_info info);
    static napi_value registerTurboModule(napi_env env,
                                           napi_callback_info info);

    // Inspector
    static napi_value enableInspector(napi_env env, napi_callback_info info);
    static napi_value disableInspector(napi_env env, napi_callback_info info);

private:
    napi_env env_;
    napi_value exports_;
    std::shared_ptr<RNHInstance> instance_;

    // Method registry
    void registerMethods();

    // Helper: create a NAPI function
    napi_value createFunction(const char* name, napi_callback cb);

    // Helper: set a property on exports
    void setExport(const char* name, napi_value value);
};

} // namespace rnoh_new
