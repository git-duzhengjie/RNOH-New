/**
 * RNOH-New: JSI / NAPI 对象缓存实现
 *
 * 基于 ArkTS NAPI 的字符串与函数缓存，减少重复创建/查找开销。
 */

#include "RNHJsiCache.h"

#include <cassert>

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHJsiCache::RNHJsiCache(napi_env env)
    : env_(env) {
    assert(env_ != nullptr && "RNHJsiCache requires a valid napi_env");
}

RNHJsiCache::~RNHJsiCache() {
    clear();
}

// ---------------------------------------------------------------------------
// 字符串缓存
// ---------------------------------------------------------------------------

napi_value RNHJsiCache::getString(const std::string& str) {
    assert(env_ != nullptr);

    // 先查缓存
    auto it = stringCache_.find(str);
    if (it != stringCache_.end()) {
        napi_value result = nullptr;
        napi_get_reference_value(env_, it->second, &result);
        return result;
    }

    // 未命中：创建字符串
    napi_value jsStr = nullptr;
    napi_status status = napi_create_string_utf8(env_, str.c_str(), str.size(), &jsStr);
    if (status != napi_ok || jsStr == nullptr) {
        return nullptr;
    }

    // 创建持久化引用并加入缓存
    napi_ref ref = nullptr;
    status = napi_create_reference(env_, jsStr, 1, &ref);
    if (status != napi_ok || ref == nullptr) {
        return jsStr; // 引用创建失败，至少返回临时值
    }

    stringCache_.emplace(str, ref);
    return jsStr;
}

void RNHJsiCache::preloadStrings(const std::vector<std::string>& strings) {
    for (const auto& s : strings) {
        getString(s);
    }
}

// ---------------------------------------------------------------------------
// 函数缓存
// ---------------------------------------------------------------------------

napi_value RNHJsiCache::getGlobalFunction(const std::string& name) {
    assert(env_ != nullptr);

    // 先查缓存
    auto it = functionCache_.find(name);
    if (it != functionCache_.end()) {
        napi_value result = nullptr;
        napi_get_reference_value(env_, it->second, &result);
        return result;
    }

    // 从全局对象查找
    napi_value global = nullptr;
    napi_status status = napi_get_global(env_, &global);
    if (status != napi_ok || global == nullptr) {
        return nullptr;
    }

    napi_value jsName = nullptr;
    napi_create_string_utf8(env_, name.c_str(), name.size(), &jsName);

    napi_value func = nullptr;
    bool hasProperty = false;
    status = napi_has_property(env_, global, jsName, &hasProperty);
    if (status != napi_ok || !hasProperty) {
        return nullptr;
    }

    status = napi_get_property(env_, global, jsName, &func);
    if (status != napi_ok || func == nullptr) {
        return nullptr;
    }

    // 确认是函数类型
    napi_valuetype type = napi_undefined;
    napi_typeof(env_, func, &type);
    if (type != napi_function) {
        return nullptr;
    }

    // 创建持久化引用并加入缓存
    napi_ref ref = nullptr;
    status = napi_create_reference(env_, func, 1, &ref);
    if (status != napi_ok || ref == nullptr) {
        return func; // 引用创建失败，返回临时值
    }

    functionCache_.emplace(name, ref);
    return func;
}

napi_value RNHJsiCache::getObjectFunction(napi_value object,
                                            const std::string& name,
                                            const std::string& cacheKey) {
    assert(env_ != nullptr);
    assert(object != nullptr);

    // 先查缓存
    auto it = functionCache_.find(cacheKey);
    if (it != functionCache_.end()) {
        napi_value result = nullptr;
        napi_get_reference_value(env_, it->second, &result);
        return result;
    }

    // 从对象查找属性
    napi_value jsName = nullptr;
    napi_create_string_utf8(env_, name.c_str(), name.size(), &jsName);

    napi_value func = nullptr;
    bool hasProperty = false;
    napi_status status = napi_has_property(env_, object, jsName, &hasProperty);
    if (status != napi_ok || !hasProperty) {
        return nullptr;
    }

    status = napi_get_property(env_, object, jsName, &func);
    if (status != napi_ok || func == nullptr) {
        return nullptr;
    }

    // 确认是函数类型
    napi_valuetype type = napi_undefined;
    napi_typeof(env_, func, &type);
    if (type != napi_function) {
        return nullptr;
    }

    // 创建持久化引用并加入缓存
    napi_ref ref = nullptr;
    status = napi_create_reference(env_, func, 1, &ref);
    if (status != napi_ok || ref == nullptr) {
        return func;
    }

    functionCache_.emplace(cacheKey, ref);
    return func;
}

// ---------------------------------------------------------------------------
// 清理
// ---------------------------------------------------------------------------

void RNHJsiCache::clear() {
    if (env_ == nullptr) {
        return;
    }

    // 释放所有字符串引用
    for (auto& [key, ref] : stringCache_) {
        if (ref != nullptr) {
            napi_delete_reference(env_, ref);
        }
    }
    stringCache_.clear();

    // 释放所有函数引用
    for (auto& [key, ref] : functionCache_) {
        if (ref != nullptr) {
            napi_delete_reference(env_, ref);
        }
    }
    functionCache_.clear();
}

} // namespace rnoh_new
