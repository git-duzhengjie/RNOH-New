/**
 * RNOH-New: JSI / NAPI 对象缓存
 *
 * 缓存常用的 JS 对象，避免重复创建/查找，提升 TurboModule 与事件派发性能。
 *
 * 缓存两类对象：
 *  - 字符串缓存：常用字符串（"onPress"、"data"、"target" 等）预创建
 *    并以 napi_ref 持久化持有，使用时直接返回 napi_value。
 *  - 函数缓存：按名称缓存全局对象上的函数（或指定对象上的函数），
 *    避免重复属性查找。
 *
 * 与 RNHInstance 集成：每个实例持有一个 RNHJsiCache，
 * 在实例初始化时预填充常用字符串。
 *
 * 注意：
 *  - 所有缓存操作必须在 JS 线程（持有 napi_env 的线程）执行
 *  - napi_ref 随 RNHJsiCache 析构时统一释放
 *  - 本类不保证线程安全（NAPI 对象不可跨线程使用）
 *
 * 命名说明：沿用 "Jsi" 命名以匹配 React Native 生态习惯，
 * 底层基于 ArkTS NAPI（napi_env / napi_value）实现。
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include <js_native_api.h>
#include <js_native_api_types.h>

namespace rnoh_new {

/**
 * JSI / NAPI 缓存
 *
 * 持有 napi_env 并缓存常用字符串与函数引用。
 */
class RNHJsiCache {
public:
    /**
     * 构造函数
     * @param env NAPI 环境（必须有效，且在本缓存生命周期内保持有效）
     */
    explicit RNHJsiCache(napi_env env);

    ~RNHJsiCache();

    // 禁止拷贝（持有 napi_ref，拷贝语义不明确）
    RNHJsiCache(const RNHJsiCache&) = delete;
    RNHJsiCache& operator=(const RNHJsiCache&) = delete;

    // -----------------------------------------------------------------------
    // 字符串缓存
    // -----------------------------------------------------------------------

    /**
     * 获取缓存的字符串
     * - 已缓存 → 直接返回 napi_value
     * - 未缓存 → 创建字符串并缓存后返回
     * @param str 字符串内容
     * @return napi_value（字符串对象），失败返回 nullptr
     */
    napi_value getString(const std::string& str);

    /**
     * 预注册一批常用字符串（建议在初始化时调用）
     * @param strings 字符串列表
     */
    void preloadStrings(const std::vector<std::string>& strings);

    /**
     * 获取缓存的字符串数量
     */
    size_t stringCount() const { return stringCache_.size(); }

    // -----------------------------------------------------------------------
    // 函数缓存
    // -----------------------------------------------------------------------

    /**
     * 获取缓存的全局函数
     * - 已缓存 → 直接返回 napi_value
     * - 未缓存 → 从全局对象查找并缓存
     * @param name 函数名（全局属性名）
     * @return napi_value（函数对象），未找到返回 nullptr
     */
    napi_value getGlobalFunction(const std::string& name);

    /**
     * 获取指定对象上的函数并缓存
     * @param object 宿主对象
     * @param name   属性名（函数）
     * @param cacheKey 缓存键（同一函数在不同对象上应使用不同 key）
     * @return napi_value（函数对象），未找到返回 nullptr
     */
    napi_value getObjectFunction(napi_value object,
                                  const std::string& name,
                                  const std::string& cacheKey);

    /**
     * 获取缓存的函数数量
     */
    size_t functionCount() const { return functionCache_.size(); }

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /**
     * 清空所有缓存（释放所有 napi_ref）
     */
    void clear();

    /**
     * 获取底层 napi_env
     */
    napi_env env() const { return env_; }

private:
    napi_env env_;  ///< NAPI 环境（弱引用，不负责生命周期）

    /// 字符串缓存：key = 字符串内容，value = napi_ref（持久化引用）
    std::unordered_map<std::string, napi_ref> stringCache_;

    /// 函数缓存：key = 缓存键，value = napi_ref（持久化引用）
    std::unordered_map<std::string, napi_ref> functionCache_;
};

} // namespace rnoh_new
