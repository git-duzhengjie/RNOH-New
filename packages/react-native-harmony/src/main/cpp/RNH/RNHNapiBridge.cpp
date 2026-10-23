/**
 * RNOH-New: RNHNapiBridge 实现
 *
 * 提供 ArkTS (ETS) 与 C++ 之间的 NAPI 桥接。
 * ArkTS 侧通过 import rnohNew from 'librnh.so' 调用本模块导出的方法。
 * 参考原 RNOH RNOHAppNapiBridge.cpp 的模块注册架构（非直接照抄）。
 */

#include "RNHNapiBridge.h"
#include "RNHInstance.h"

#include <cstring>

namespace rnoh_new {

// 全局单例实例（简化版本，后续可扩展为多实例管理）
static std::shared_ptr<RNHInstance> g_instance = nullptr;

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHNapiBridge::RNHNapiBridge(napi_env env, napi_value exports)
    : env_(env), exports_(exports) {
    // 构造函数：保存 env 和 exports 引用
}

RNHNapiBridge::~RNHNapiBridge() {
    // 析构函数：清理实例引用
    instance_.reset();
}

// ---------------------------------------------------------------------------
// 初始化
// ---------------------------------------------------------------------------

bool RNHNapiBridge::initialize() {
    registerMethods();
    return true;
}

void RNHNapiBridge::setInstance(std::shared_ptr<RNHInstance> instance) {
    instance_ = std::move(instance);
}

// ---------------------------------------------------------------------------
// 方法注册
// ---------------------------------------------------------------------------

void RNHNapiBridge::registerMethods() {
    // 定义所有导出到 ArkTS 的 NAPI 方法描述
    napi_property_descriptor descriptors[] = {
        // 实例生命周期
        {"createInstance", nullptr, createInstance, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"destroyInstance", nullptr, destroyInstance, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        // Bundle 加载
        {"loadBundle", nullptr, loadBundle, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"loadBundleFromUrl", nullptr, loadBundleFromUrl, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        // Surface 管理
        {"startSurface", nullptr, startSurface, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"stopSurface", nullptr, stopSurface, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"updateSurfaceSize", nullptr, updateSurfaceSize, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        // 事件
        {"emitEvent", nullptr, emitEvent, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"sendTouchEvent", nullptr, sendTouchEvent, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        // TurboModule
        {"getTurboModule", nullptr, getTurboModule, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"registerTurboModule", nullptr, registerTurboModule, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        // Inspector
        {"enableInspector", nullptr, enableInspector, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"disableInspector", nullptr, disableInspector, nullptr, nullptr,
         nullptr, napi_default, nullptr},
    };

    // 批量定义到 exports 对象上
    napi_define_properties(env_, exports_,
                           sizeof(descriptors) / sizeof(descriptors[0]),
                           descriptors);
}

// ---------------------------------------------------------------------------
// 辅助方法
// ---------------------------------------------------------------------------

napi_value RNHNapiBridge::createFunction(const char* name, napi_callback cb) {
    napi_value func = nullptr;
    napi_create_function(env_, name, NAPI_AUTO_LENGTH, cb, nullptr, &func);
    return func;
}

void RNHNapiBridge::setExport(const char* name, napi_value value) {
    napi_set_named_property(env_, exports_, name, value);
}

// ---------------------------------------------------------------------------
// 参数解析辅助
// ---------------------------------------------------------------------------

/**
 * 从 NAPI 回调信息中获取指定位置的字符串参数
 */
static std::string getStringArg(napi_env env, napi_callback_info info,
                                 size_t index, size_t total) {
    size_t argc = total;
    napi_value args[16] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (index >= argc) {
        return "";
    }

    size_t len = 0;
    napi_get_value_string_utf8(env, args[index], nullptr, 0, &len);
    std::string result(len, '\0');
    napi_get_value_string_utf8(env, args[index], &result[0], len + 1, &len);
    return result;
}

/**
 * 从 NAPI 回调信息中获取指定位置的 int32 参数
 */
static int32_t getInt32Arg(napi_env env, napi_callback_info info,
                            size_t index, size_t total) {
    size_t argc = total;
    napi_value args[16] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (index >= argc) {
        return 0;
    }

    int32_t value = 0;
    napi_get_value_int32(env, args[index], &value);
    return value;
}

/**
 * 从 NAPI 回调信息中获取指定位置的 double 参数
 */
static double getDoubleArg(napi_env env, napi_callback_info info,
                            size_t index, size_t total) {
    size_t argc = total;
    napi_value args[16] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (index >= argc) {
        return 0.0;
    }

    double value = 0.0;
    napi_get_value_double(env, args[index], &value);
    return value;
}

/**
 * 从 NAPI 回调信息中获取指定位置的 int64 参数
 */
static int64_t getInt64Arg(napi_env env, napi_callback_info info,
                            size_t index, size_t total) {
    size_t argc = total;
    napi_value args[16] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (index >= argc) {
        return 0;
    }

    int64_t value = 0;
    napi_get_value_int64(env, args[index], &value);
    return value;
}

/**
 * 创建并返回一个布尔值给 ArkTS
 */
static napi_value makeBool(napi_env env, bool value) {
    napi_value result = nullptr;
    napi_get_boolean(env, value, &result);
    return result;
}

/**
 * 创建并返回 undefined 给 ArkTS
 */
static napi_value makeUndefined(napi_env env) {
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

// ---------------------------------------------------------------------------
// NAPI 方法实现：实例生命周期
// ---------------------------------------------------------------------------

/**
 * createInstance(config?: object) -> boolean
 * ArkTS 调用以创建 RNH 实例（当前简化为全局单例）
 */
napi_value RNHNapiBridge::createInstance(napi_env env,
                                          napi_callback_info info) {
    // TODO: 从 ArkTS 传入的 config 对象中解析 RNHInstanceConfig
    // 当前简化实现：直接创建默认配置的实例
    if (!g_instance) {
        RNHInstanceConfig config;
        g_instance = std::make_shared<RNHInstance>();
        g_instance->setNapiEnv(env);
        g_instance->initialize(config);
    }
    return makeBool(env, g_instance != nullptr);
}

/**
 * destroyInstance() -> void
 * ArkTS 调用以销毁 RNH 实例
 */
napi_value RNHNapiBridge::destroyInstance(napi_env env,
                                           napi_callback_info info) {
    if (g_instance) {
        g_instance->destroy();
        g_instance.reset();
    }
    return makeUndefined(env);
}

// ---------------------------------------------------------------------------
// NAPI 方法实现：Bundle 加载
// ---------------------------------------------------------------------------

/**
 * loadBundle(bundlePath: string) -> boolean
 * ArkTS 调用以加载本地 JS bundle
 */
napi_value RNHNapiBridge::loadBundle(napi_env env, napi_callback_info info) {
    if (!g_instance) {
        return makeBool(env, false);
    }

    std::string bundlePath = getStringArg(env, info, 0, 1);
    bool ok = g_instance->loadBundle(bundlePath);
    return makeBool(env, ok);
}

/**
 * loadBundleFromUrl(url: string) -> boolean
 * ArkTS 调用以从 URL 加载 JS bundle
 */
napi_value RNHNapiBridge::loadBundleFromUrl(napi_env env,
                                             napi_callback_info info) {
    if (!g_instance) {
        return makeBool(env, false);
    }

    std::string url = getStringArg(env, info, 0, 1);
    bool ok = g_instance->loadBundleFromUrl(url);
    return makeBool(env, ok);
}

// ---------------------------------------------------------------------------
// NAPI 方法实现：Surface 管理
// ---------------------------------------------------------------------------

/**
 * startSurface(surfaceId: number, moduleName: string,
 *              width: number, height: number, pixelRatio: number) -> void
 * ArkTS 调用以启动一个 React Surface
 */
napi_value RNHNapiBridge::startSurface(napi_env env,
                                        napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    int32_t surfaceId = getInt32Arg(env, info, 0, 5);
    std::string moduleName = getStringArg(env, info, 1, 5);
    double width = getDoubleArg(env, info, 2, 5);
    double height = getDoubleArg(env, info, 3, 5);
    double pixelRatio = getDoubleArg(env, info, 4, 5);

    g_instance->startSurface(surfaceId, moduleName,
                              static_cast<float>(width),
                              static_cast<float>(height),
                              static_cast<float>(pixelRatio));
    return makeUndefined(env);
}

/**
 * stopSurface(surfaceId: number) -> void
 * ArkTS 调用以停止一个 React Surface
 */
napi_value RNHNapiBridge::stopSurface(napi_env env, napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    int32_t surfaceId = getInt32Arg(env, info, 0, 1);
    g_instance->stopSurface(surfaceId);
    return makeUndefined(env);
}

/**
 * updateSurfaceSize(surfaceId: number, width: number, height: number) -> void
 * ArkTS 调用以更新 Surface 尺寸
 */
napi_value RNHNapiBridge::updateSurfaceSize(napi_env env,
                                             napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    int32_t surfaceId = getInt32Arg(env, info, 0, 3);
    double width = getDoubleArg(env, info, 1, 3);
    double height = getDoubleArg(env, info, 2, 3);

    g_instance->updateSurfaceSize(surfaceId,
                                   static_cast<float>(width),
                                   static_cast<float>(height));
    return makeUndefined(env);
}

// ---------------------------------------------------------------------------
// NAPI 方法实现：事件
// ---------------------------------------------------------------------------

/**
 * emitEvent(surfaceId: number, eventName: string, payload: string) -> void
 * ArkTS 调用以向 JS 侧发送自定义事件
 */
napi_value RNHNapiBridge::emitEvent(napi_env env, napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    int32_t surfaceId = getInt32Arg(env, info, 0, 3);
    std::string eventName = getStringArg(env, info, 1, 3);
    std::string payload = getStringArg(env, info, 2, 3);

    g_instance->emitEvent(surfaceId, eventName, payload);
    return makeUndefined(env);
}

/**
 * sendTouchEvent(surfaceId: number, action: number,
 *                x: number, y: number, timestamp: number) -> void
 * ArkTS 调用以向 JS 侧发送触摸事件
 */
napi_value RNHNapiBridge::sendTouchEvent(napi_env env,
                                          napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    int32_t surfaceId = getInt32Arg(env, info, 0, 5);
    int32_t action = getInt32Arg(env, info, 1, 5);
    double x = getDoubleArg(env, info, 2, 5);
    double y = getDoubleArg(env, info, 3, 5);
    int64_t timestamp = getInt64Arg(env, info, 4, 5);

    g_instance->sendTouchEvent(surfaceId, action,
                                static_cast<float>(x),
                                static_cast<float>(y),
                                timestamp);
    return makeUndefined(env);
}

// ---------------------------------------------------------------------------
// NAPI 方法实现：TurboModule
// ---------------------------------------------------------------------------

/**
 * getTurboModule(name: string) -> boolean
 * ArkTS 调用以检查 TurboModule 是否存在（简化返回布尔值）
 */
napi_value RNHNapiBridge::getTurboModule(napi_env env,
                                          napi_callback_info info) {
    if (!g_instance) {
        return makeBool(env, false);
    }

    std::string name = getStringArg(env, info, 0, 1);
    void* module = g_instance->getTurboModule(name);
    return makeBool(env, module != nullptr);
}

/**
 * registerTurboModule(name: string) -> boolean
 * ArkTS 调用以注册 TurboModule（当前仅注册名称占位）
 */
napi_value RNHNapiBridge::registerTurboModule(napi_env env,
                                               napi_callback_info info) {
    if (!g_instance) {
        return makeBool(env, false);
    }

    std::string name = getStringArg(env, info, 0, 1);
    // TODO: 从 ArkTS 接收工厂函数引用并注册
    // 当前注册一个返回 nullptr 的占位工厂
    g_instance->registerTurboModule(name, []() -> void* { return nullptr; });
    return makeBool(env, true);
}

// ---------------------------------------------------------------------------
// NAPI 方法实现：Inspector
// ---------------------------------------------------------------------------

/**
 * enableInspector(port?: number) -> void
 * ArkTS 调用以启用 Chrome DevTools 调试
 */
napi_value RNHNapiBridge::enableInspector(napi_env env,
                                           napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    int32_t port = getInt32Arg(env, info, 0, 1);
    g_instance->enableInspector(static_cast<uint16_t>(port));
    return makeUndefined(env);
}

/**
 * disableInspector() -> void
 * ArkTS 调用以禁用 Chrome DevTools 调试
 */
napi_value RNHNapiBridge::disableInspector(napi_env env,
                                            napi_callback_info info) {
    if (!g_instance) {
        return makeUndefined(env);
    }

    g_instance->disableInspector();
    return makeUndefined(env);
}

} // namespace rnoh_new

// ---------------------------------------------------------------------------
// NAPI 模块注册入口（全局命名空间）
// ---------------------------------------------------------------------------

/**
 * NAPI 模块初始化函数：注册所有导出方法到 exports
 */
static napi_value Init(napi_env env, napi_value exports) {
    rnoh_new::RNHNapiBridge bridge(env, exports);
    bridge.initialize();
    return exports;
}

/**
 * NAPI 模块描述结构体
 * nm_modname 需与 ArkTS import 的 .so 文件名对应（librnh.so -> "rnh"）
 */
static napi_module rnhModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "rnh",
    .nm_priv = nullptr,
    .reserved = {nullptr},
};

/**
 * 模块注册：在动态库加载时自动调用
 */
extern "C" __attribute__((constructor)) void RegisterRNHModule() {
    napi_module_register(&rnhModule);
}
