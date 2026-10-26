/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef native_ArkJS_H
#define native_ArkJS_H

#include <folly/Function.h>
#include <folly/dynamic.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <jserrorhandler/JsErrorHandler.h>
#include <jsi/jsi.h>
#include <react/bridging/CallbackWrapper.h>
#include <react/renderer/graphics/Color.h>
#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/RectangleCorners.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "RNOH/RNOHError.h"
#include "RNOH/Result.h"
#include "ThreadGuard.h"

class RNOHNapiObjectBuilder;
class RNOHNapiObject;
class NapiRef;

/**
 * @brief 管理 CallbackWrapper 从 LongLivedObjectCollection 的释放时机。
 *
 * release() 幂等，在以下时机触发：
 *   1. 回调被调用时（主动释放）
 *   2. 显式调用 release()（如互斥回调中未调用的兄弟）
 *   3. 析构时（GC finalizer 兜底：napi finalizer 销毁持有此 guard 的
 *      std::function → shared_ptr 引用计数归零 → ~CallbackGuard → release()）
 */
class CallbackGuard {
 public:
  explicit CallbackGuard(
      std::weak_ptr<facebook::react::CallbackWrapper> weakCallbackWrapper)
      : m_weakCallbackWrapper(std::move(weakCallbackWrapper)) {}

  ~CallbackGuard() { release(); }

  CallbackGuard(const CallbackGuard&) = delete;
  CallbackGuard& operator=(const CallbackGuard&) = delete;
  CallbackGuard(CallbackGuard&& other) noexcept
      : m_weakCallbackWrapper(std::move(other.m_weakCallbackWrapper)),
        m_released(other.m_released.load()) {
    other.m_released.store(true);
  }
  CallbackGuard& operator=(CallbackGuard&& other) noexcept {
    if (this != &other) {
      bool otherReleased = other.m_released.load();
      release();
      m_weakCallbackWrapper = std::move(other.m_weakCallbackWrapper);
      m_released.store(otherReleased);
      other.m_released.store(true);
    }
    return *this;
  }

  /// 从 LongLivedObjectCollection 释放 CallbackWrapper，幂等。
  void release() {
    bool expected = false;
    if (!m_released.compare_exchange_strong(expected, true)) {
      return;
    }
    auto callbackWrapper = m_weakCallbackWrapper.lock();
    if (callbackWrapper != nullptr) {
      callbackWrapper->allowRelease();
    }
    m_weakCallbackWrapper.reset();
  }

  bool isReleased() const { return m_released.load(); }

 private:
  std::weak_ptr<facebook::react::CallbackWrapper> m_weakCallbackWrapper;
  std::atomic<bool> m_released{false};
};

/**
 * @brief 同一方法调用的 CallbackGuard 组，支持互斥回调模式。
 *
 * exclusive=true（默认，如 promise resolve/reject）：调用任一回调时
 * 自动释放所有兄弟，解决 issue #2652 中未调用兄弟的 CallbackWrapper 泄漏。
 * exclusive=false：releaseSiblings() 为空操作，兄弟回调仍可能被调用。
 */
class CallbackGroup {
 public:
  explicit CallbackGroup(bool exclusive = true) : m_exclusive(exclusive) {}

  void addSibling(std::shared_ptr<CallbackGuard> guard) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_siblingGuards.push_back(std::move(guard));
  }

  /// 互斥模式下释放所有兄弟 CallbackWrapper，非互斥模式为空操作。
  /// 安全：调用者已通过 callbackGuard->release() 释放自身，allowRelease() 幂等。
  /// @return 实际释放的 wrapper 数量。
  size_t releaseSiblings() {
    if (!m_exclusive) {
      return 0;
    }
    std::vector<std::shared_ptr<CallbackGuard>> siblingGuards;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      siblingGuards = std::move(m_siblingGuards);
      m_siblingGuards.clear();
    }
    size_t count = 0;
    for (auto& guard : siblingGuards) {
      if (guard && !guard->isReleased()) {
        guard->release();
        count++;
      }
    }
    return count;
  }

  bool isExclusive() const { return m_exclusive; }

 private:
  bool m_exclusive;
  std::mutex m_mutex;
  std::vector<std::shared_ptr<CallbackGuard>> m_siblingGuards;
};

/**
 * @brief CallbackGuard 实例注册表，支持批量释放 TurboModule 的待处理 CallbackWrapper。
 *
 * releaseAll() 主动释放所有未调用的回调而无需等待 GC，
 * 对高频调用场景至关重要（GC 无法跟上 LongLivedObjectCollection 增长速度）。
 */
class CallbackGuardRegistry {
 public:
  void addShared(std::shared_ptr<CallbackGuard> guard) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_guards.push_back(std::move(guard));
    // 超过阈值自动压缩，防止高频调用下内存无限增长。
    if (m_guards.size() > COMPACT_THRESHOLD) {
      doCompact();
    }
  }

  /**
   * @brief 释放所有已注册的 CallbackGuard，从 LongLivedObjectCollection 移除其 CallbackWrapper。
   * @return 本次实际释放的数量（已释放的 guard 为空操作）。
   *
   * 并发安全：m_mutex 串行化多个 releaseAll()，首个调用 std::move 走 m_guards，
   * 后续调用返回 0（回调已被其他路径抢先释放）。
   */
  size_t releaseAll() {
    std::vector<std::shared_ptr<CallbackGuard>> guards;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      guards = std::move(m_guards);
      m_guards.clear();
    }
    // 锁外释放，避免 allowRelease() 期间持锁。
    size_t released = 0;
    for (auto& guard : guards) {
      if (guard && !guard->isReleased()) {
        guard->release();
        ++released;
      }
    }
    return released;
  }

  void compact() {
    std::lock_guard<std::mutex> lock(m_mutex);
    doCompact();
  }

  size_t pendingCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& guard : m_guards) {
      if (guard && !guard->isReleased()) {
        count++;
      }
    }
    return count;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_guards.size();
  }

 private:
  static constexpr size_t COMPACT_THRESHOLD = 128;

  void doCompact() {
    // 必须持有 m_mutex
    m_guards.erase(
        std::remove_if(
            m_guards.begin(),
            m_guards.end(),
            [](const std::shared_ptr<CallbackGuard>& g) {
              return !g || g->isReleased();
            }),
        m_guards.end());
  }

  mutable std::mutex m_mutex;
  std::vector<std::shared_ptr<CallbackGuard>> m_guards;
};

/**
 * @actor RNOH_LIBRARY
 * ArkJS provides utility functions for interacting with ArkTS objects
 * environment through NAPI. this class provides a more convenient way
 * to interact with ArkJS (in the docs that was the name used to describe JS
 * engine used by ETS) than using NAPI functions directly. It supports creating
 * ArkTS values, calling ArkTS functions, and handling exceptions.
 */
class ArkJS {
 public:
  /**
   * @brief Type alias for a callback function that takes a vector of
   * folly::dynamic objects.
   */
  using IntermediaryCallback = std::function<void(std::vector<folly::dynamic>)>;

  /**
   * @brief Type alias for a variant that can hold either a `folly::dynamic` or
   * an `IntermediaryCallback`.
   *
   * This type was created to handle more types than folly::dynamic
   * handles. TurboModules can provide a callback as an argument and such
   * callback aren't supported by folly::dynamic so we have created a custom
   * type.
   */
  using IntermediaryArg = std::variant<folly::dynamic, IntermediaryCallback>;

  /**
   * @brief Constructs an ArkJS instance.
   *
   * Initializes the ArkJS class with the given NAPI environment. This object
   * provides methods to work with ArkTS objects and functions.
   *
   * @param env The NAPI environment used for ArkTS interactions.
   */
  ArkJS(napi_env env);

  /**
   * @brief Calls an ArkTS function with the specified arguments.
   *
   * This template function allows calling an ArkTS function with a fixed
   * number of arguments. The arguments are passed as a `std::array`.
   *
   * @param callback The ArkTS function to call.
   * @param args The arguments to pass to the ArkTS function.
   * @param thisObject Optional, this method can call functions or methods.
   * In case of methods, thisObject should be provided
   * @return The result from calling the ArkTS function.
   */
  template <size_t args_count>
  napi_value call(
      napi_value callback,
      std::array<napi_value, args_count> args,
      napi_value thisObject = nullptr) {
    return call(callback, args.data(), args.size(), thisObject);
  }

  /**
   * @brief Calls an ArkTS function with a vector of arguments.
   *
   * This overload allows passing arguments as a vector of NAPI values.
   *
   * @param callback The ArkTS function to call.
   * @param args The arguments to pass to the ArkTS function.
   * @param thisObject Optional, this method can call functions or methods.
   * In case of methods, thisObject should be provided.
   * @return The result from calling the ArkTS function.
   */
  napi_value call(
      napi_value callback,
      std::vector<napi_value> args,
      napi_value thisObject = nullptr);

  /**
   * @brief Calls an ArkTS function with an array of arguments.
   *
   * This overload calls a function with arguments provided as a pointer to an
   * array and the number of arguments.
   *
   * @param callback The ArkTS function to call.
   * @param args The arguments to pass to the ArkTS function.
   * @param argsCount The number of arguments to pass.
   * @param thisObject Optional, this method can call functions or methods.
   * In case of methods, thisObject should be provided.
   * @return The result from calling the ArkTS function.
   */
  napi_value call(
      napi_value callback,
      const napi_value* args,
      int argsCount,
      napi_value thisObject);

  /**
   * @brief Creates an ArkTS boolean value from a C++ bool.
   *
   * This function creates an ArkTS boolean (`true` or `false`) based on the
   * C++ `bool` value provided.
   *
   * @param value The C++ boolean value to convert to an ArkTS boolean.
   * @return The created ArkTS boolean value.
   */
  napi_value createBoolean(bool value);

  /**
   * @brief Creates an ArkTS integer value from a C++ int.
   *
   * This function creates an ArkTS integer value (`int32_t`) from the provided
   * C++ integer. The value is cast to `int32_t` because ArkTS integers are
   * 32-bit.
   *
   * @param value The C++ integer value to convert to an ArkTS integer.
   * @return The created ArkTS integer value.
   */
  napi_value createInt(int value);

  /**
   * @brief Creates an ArkTS double value from a C++ double.
   *
   * This function creates an ArkTS `double` value from a C++ `double` value.
   * ArkTS uses 64-bit floating point values (doubles).
   *
   * @param value The C++ double value to convert to an ArkTS double.
   * @return The created ArkTS double value.
   */
  napi_value createDouble(double value);

  /**
   * @brief Creates an ArkTS String value from a C++ `std::string`.
   *
   * This method converts a C++ `std::string` to an ArkTS string (`napi_value`).
   *
   * @param str The C++ `std::string` value to convert to an ArkTS string.
   * @return The corresponding ArkTS string value.
   */
  napi_value createString(std::string const& str);

  /**
   * @brief Creates an ArkTS String value from a C++ `std::string_view`.
   *
   * This method converts a C++ `std::string_view` to an ArkTS string
   * (`napi_value`). It calls the overloaded method `createString` which
   * accepts a C-style string (`const char*`).
   *
   * @param str The C++ `std::string_view` value to convert to an ArkTS string.
   * @return The corresponding ArkTS string value.
   */
  napi_value createString(std::string_view str);

  /**
   * @brief Creates an ArkTS String value from a C-style string (`const char*`).
   *
   * This method creates an ArkTS string (`napi_value`) from a C-style string
   * (`const char*`). It allows the conversion of a null-terminated string to a
   * ArkTS string with a specified length.
   *
   * @param str The C-style string (`const char*`) to convert to an ArkTS
   * string.
   * @param length The length of the C-style string, excluding the null
   * terminator.
   * @return The corresponding ArkTS string value.
   *
   * @throws napi_status exception if the string creation fails.
   */
  napi_value createString(char const* str, size_t length);

  /**
   * @brief Creates a reference to an ArkTS value.
   *
   * This method creates a reference (`napi_ref`) to an ArkTS value
   * (`napi_value`).
   *
   * @param value The ArkTS value (`napi_value`) to create a reference to.
   * @return The `napi_ref` (reference) to the ArkTS value.
   *
   * @throws napi_status exception if creating the reference fails.
   * @note napi_ref needs to be manually removed (by calling deleteReference).
   * Suggest using createNapiRef instead to avoid dealing with memory
   * deallocation.
   */
  napi_ref createReference(napi_value value);

  /**
   * @brief Deletes a reference to an ArkTS value.
   *
   * This method deletes a reference (`napi_ref`) to an ArkTS value. Deleting a
   * reference decrements the reference count, and if the reference count
   * reaches zero, the ArkTS value may be garbage collected. It is important to
   * ensure that references are properly deleted when they are no longer needed
   * to avoid memory leaks.
   *
   * @param reference The reference (`napi_ref`) to the ArkTS value that is to
   * be deleted.
   *
   * @throws napi_status exception if deleting the reference fails.
   */
  void deleteReference(napi_ref reference);

  /**
   * @brief Creates a `NapiRef` from a given ArkTS value.
   *
   * @param value The ArkTS value (`napi_value`) to be referenced.
   * @return A `NapiRef` object which holds a reference to the provided ArkTS
   * value.
   * @note The `NapiRef` ensures that the referenced value will persist for as
   * long as the `NapiRef` is valid.
   */
  NapiRef createNapiRef(napi_value value);

  /**
   * @deprecated
   * @brief Creates a single-use callback function.
   *
   * This deprecated method creates a callback that can only be invoked once.
   * Once invoked, the callback is nullified to prevent further calls.
   *
   * @param callback The callback to invoke once.
   * @return The created callback function.
   */
  napi_value createSingleUseCallback(
      std::function<void(std::vector<folly::dynamic>)>&& callback) {
    return createCallback([cb = std::move(callback)](auto args) mutable {
      RNOH_ASSERT_MSG(
          cb != nullptr,
          "Callback returned from `createSingleUseCallback` may only be called once");
      cb(std::move(args));
      cb = nullptr;
    });
  }

  /**
   * @brief Creates a callback function and manages its lifetime.
   * @details This template function takes a callback function and creates a
   * corresponding ArkTS callback in the environment. It uses
   * `napi_add_finalizer` to allocate memory for the callback function
   * and ensure it is properly cleaned up when no longer needed.
   *
   * @param F The type of the callback function.
   * @param callback The callback function.
   * @return The created ArkTS function (napi_value), which can be called in
   * the JS environment.
   */
  template <typename F>
  napi_value createCallback(F&& callback) {
    auto* allocatedCallback = new F(std::forward<F>(callback));
    try {
      auto function = createFunction(
          "callback",
          [](napi_env env, napi_callback_info info) {
            void* data;
            napi_get_cb_info(env, info, nullptr, nullptr, nullptr, &data);
            auto callback = static_cast<F*>(data);
            ArkJS arkJS(env);
            (*callback)(arkJS.getDynamics(arkJS.getCallbackArgs(info)));
            return arkJS.getUndefined();
          },
          allocatedCallback);

      napi_add_finalizer(
          m_env,
          function,
          allocatedCallback,
          [](napi_env /*env*/, void* data, void* /*hint*/) {
            auto callback = static_cast<F*>(data);
            delete callback;
          },
          nullptr,
          nullptr);
      return function;
    } catch (...) {
      delete allocatedCallback;
      throw;
    }
  }

  /**
   * @brief Creates a new ArkTS function.
   *
   * This method creates an ArkTS function in the NAPI environment. The function
   * is associated with a callback that will be invoked when the function is
   * called from ArkTS. The function is given a name and an optional `data`
   * pointer that can be used to pass extra information to the callback.
   *
   * @param name The name of the ArkTS function being created.
   * @param callback The callback function that will be executed when the ArkTS
   * function is called.
   * @param data Optional user data to be passed to the callback function.
   *
   * @return The newly created ArkTS function (`napi_value`).
   *
   * @throws napi_status exception if creating the function fails.
   */
  napi_value createFunction(
      std::string const& name,
      napi_callback callback,
      void* data = nullptr);

  /**
   * @brief Creates a new empty ArkTS array.
   *
   * This method creates a new ArkTS array in the NAPI environment. The array
   * is initially empty, and its elements can be added later using other NAPI
   * methods.
   *
   * @return The newly created empty ArkTS array (`napi_value`).
   */
  napi_value createArray();

  /**
   * @brief Creates an ArkTS array from a given set of values.
   *
   * This method creates a new ArkTS array and populates it with the values
   * provided in the input vector. Each element from the vector is added to the
   * array at the corresponding index.
   *
   * @param values A vector of `napi_value` elements to be added to the array.
   *
   * @return The newly created ArkTS array (`napi_value`) containing the input
   * values.
   */
  napi_value createArray(std::vector<napi_value>);

  /**
   * @brief Converts a vector of `folly::dynamic` values to a vector of
   * `napi_value` values.
   *
   * This function takes a vector of `folly::dynamic` objects (which can
   * represent various types such as strings, numbers, objects, etc.) and
   * converts each item into its corresponding NAPI representation
   * (`napi_value`). The result is a vector of `napi_value` that can be used in
   * NAPI calls.
   *
   * @param dynamics A vector of `folly::dynamic` objects to be converted.
   * @return A vector of `napi_value` corresponding to the input
   * `folly::dynamic` values.
   */
  std::vector<napi_value> createFromDynamics(
      std::vector<folly::dynamic> const&);

  /**
   * @brief Converts a `folly::dynamic` to a corresponding NAPI value.
   *
   * This method takes a `folly::dynamic` object, which can represent various
   * data types such as strings, objects, arrays, integers, doubles, and
   * booleans, and converts it into a corresponding NAPI value (`napi_value`).
   * It recursively handles nested objects and arrays.
   *
   * @param The `folly::dynamic` object to be converted into a NAPI value.
   *
   * @return The corresponding `napi_value` representing the dynamic value.
   *         If the dynamic type is not supported, it returns `undefined`.
   */
  napi_value createFromDynamic(folly::dynamic const&);

  /**
   * @brief Creates a NAPI value from a C++ exception.
   *
   * This method converts a C++ exception (`std::exception`) into a NAPI value.
   * The exception's message is wrapped inside a `folly::dynamic` object, which
   * is then converted into a NAPI value.
   *
   * @param The exception to be converted.
   * @return The corresponding `napi_value` representing the exception.
   */
  napi_value createFromException(std::exception const&);

  /**
   * @brief Creates a NAPI value from a JavaScript error (`JSError`).
   *
   * This method converts a JavaScript error (`JSError`) from the Facebook JSI
   * library into a NAPI value. The error message, stack trace, and nested
   * errors (if any) are included in the resulting NAPI value.
   *
   * @param e The `JSError` object to be converted.
   * @return The corresponding `napi_value` representing the JavaScript error.
   */
  napi_value createFromJSError(facebook::jsi::JSError const&);

  /**
   * @brief Creates a NAPI value from a `RNOHError`.
   *
   * This method converts a `RNOHError` into a NAPI value. The error message,
   * stack trace, and suggestions (if any) are included in the resulting NAPI
   * value.
   *
   * @param e The `RNOHError` object to be converted.
   * @return The corresponding `napi_value` representing the RNOH error.
   */
  napi_value createFromRNOHError(rnoh::RNOHError const&);

  /**
   * @internal
   * @brief Creates NAPI value from ParsedError
   * @note Returned object structure is different depending on whether debug
   * mode is enabled. When the `stackFrames` attribute is present in the object,
   * created ets error object would attempt to symbolicate the stack trace.
   *
   * @param e The ParsedError to be converted
   * @return Corresponding `napi_value` for the ParsedError
   */
  napi_value createFromParsedError(
      const facebook::react::JsErrorHandler::ProcessedError&);

  /**
   * @brief Creates a NAPI value from a `rnoh::Result<napi_value>`.
   *
   * This method converts a `rnoh::Result` object into a NAPI value. If the
   * result is an error, the error is wrapped in an ArkTS object with an `err`
   * property, and the success value is placed in an `ok` property. If the
   * result is successful, the `ok` property contains the value, and the `err`
   * property is `null`.
   *
   * @param result The `rnoh::Result<napi_value>` to be converted into a NAPI
   * value.
   *
   * @return A NAPI value representing the result: either a success (`ok`) or an
   * error (`err`).
   */
  napi_value createResult(rnoh::Result<napi_value> const&);

  /**
   * @brief Creates an `RNOHNapiObjectBuilder` instance for constructing ArkTS
   * objects.
   *
   * @return An instance of `RNOHNapiObjectBuilder`.
   */
  RNOHNapiObjectBuilder createObjectBuilder();

  /**
   * @brief Checks if the given value is an ArkTS promise.
   *
   * This method checks whether the given value is an ArkTS promise.
   *
   * @param value The value to check.
   * @return `true` if the value is a promise, otherwise `false`.
   */
  bool isPromise(napi_value);

  /**
   * @brief Checks if the given value is an ArkTS undefined.
   *
   * This method checks whether the given value is an ArkTS undefined.
   *
   * @param value The value to check.
   * @return `true` if the value is a undefined, otherwise `false`.
   */
  bool isUndefined(napi_value value);

  /**
   * @brief Retrieves an `RNOHNapiObjectBuilder` instance for an existing ArkTS
   * object.
   *
   * This method returns an `RNOHNapiObjectBuilder` initialized with a
   * pre-existing ArkTS object (`napi_value`).
   *
   * @param object The existing ArkTS object (`napi_value`).
   * @return An instance of `RNOHNapiObjectBuilder`.
   */
  RNOHNapiObjectBuilder getObjectBuilder(napi_value object);

  /**
   * @brief Creates an ArkTS `undefined` value.
   *
   * This method creates an ArkTS `undefined` value, which can be used when a
   * function does not return a value or when a variable is not defined in
   * ArkTS.
   *
   * @return The ArkTS `undefined` value.
   */
  napi_value getUndefined();

  /**
   * @brief Creates an ArkTS `null` value.
   *
   * This method creates an ArkTS `null` value, which is typically used to
   * explicitly represent an empty or uninitialized value in ArkTS.
   *
   * @return The ArkTS `null` value.
   */
  napi_value getNull();

  /**
   * @brief Retrieves the value from a reference.
   *
   * This method is used to fetch the actual `napi_value` that is associated
   * with a given `napi_ref` (reference).
   *
   * @param ref The `napi_ref` reference that holds an ArkTS value.
   * @return The `napi_value` associated with the given reference.
   *
   * @throws napi_status exception if retrieving the value from the reference
   * fails.
   */
  napi_value getReferenceValue(napi_ref ref);

  /**
   * @brief Retrieves the value from a `NapiRef`.
   *
   * This function is designed to retrieve the actual `napi_value` associated
   * with a `NapiRef`.
   *
   * @param ref The `NapiRef` containing a reference to the ArkTS object.
   * @return The `napi_value` that corresponds to the ArkTS object referenced
   * by `ref`.
   */
  napi_value getReferenceValue(NapiRef const& ref);

  /**
   * @brief Retrieves the arguments passed to an ArkTS function callback.
   *
   * @param info The `napi_callback_info` structure which contains the
   * information about the callback.
   * @return A vector containing the arguments passed to the ArkTS function.
   */
  std::vector<napi_value> getCallbackArgs(napi_callback_info info);

  /**
   * @brief Retrieves the arguments passed to an ArkTS function callback.
   *
   * @param info The `napi_callback_info` structure containing the information
   * about the callback.
   * @param args_count The number of arguments passed to the callback.
   * @return A vector containing the arguments passed to the ArkTS function.
   */
  std::vector<napi_value> getCallbackArgs(
      napi_callback_info info,
      size_t args_count);

  /**
   * @brief Retrieves a wrapped `RNOHNapiObject` from a `napi_value`.
   *
   * @param object The `napi_value` representing the ArkTS object to be wrapped.
   * @return An `RNOHNapiObject` that wraps the ArkTS object.
   */
  RNOHNapiObject getObject(napi_value object);

  /**
   * @brief Retrieves a wrapped `RNOHNapiObject` from a `napi_ref`.
   *
   * @param objectRef The `napi_ref` representing a reference to the ArkTS
   * object to be wrapped.
   * @return An `RNOHNapiObject` that wraps the ArkTS object referenced by
   * `objectRef`.
   */
  RNOHNapiObject getObject(napi_ref objectRef);

  /**
   * @brief Retrieves a wrapped `RNOHNapiObject` from a `NapiRef`.
   *
   * @param objectRef The `NapiRef` representing a reference to the ArkTS
   * object to be wrapped.
   * @return An `RNOHNapiObject` that wraps the ArkTS object referenced by
   * `objectRef`.
   */
  RNOHNapiObject getObject(NapiRef const& objectRef);
  /**
   * @brief Retrieves the property of an object using a string key.
   *
   * This method fetches a property of an ArkTS object identified by a string
   * key.
   *
   * @param object The ArkTS object whose property is to be retrieved.
   * @param key The property key, provided as a `std::string`.
   * @return The `napi_value` representing the property value.
   */
  napi_value getObjectProperty(napi_value object, std::string const& key);

  /**
   * @brief Retrieves the property of an object using a `napi_value` key.
   *
   * This method fetches a property from the given ArkTS object using the
   * provided `napi_value` key.
   *
   * @param object The ArkTS object whose property is to be retrieved.
   * @param key The property key, provided as a `napi_value`.
   * @return The `napi_value` representing the property value.
   *
   * @throws napi_status exception If retrieving the property fails.
   */
  napi_value getObjectProperty(napi_value object, napi_value key);

  /**
   * @brief Checks if the given object has the specified property by key
   * (string).
   *
   * This is a wrapper function that internally calls the overloaded version
   * which takes a `napi_value` as the key. It converts the string key to a
   * `napi_value` and checks for the property.
   *
   * @param object The object whose properties are being checked.
   * @param key The string key representing the property name.
   * @return True if the property exists on the object, false otherwise.
   */
  bool hasProperty(napi_value object, std::string const& key);

  /**
   * @brief Checks if the given object has the specified property by key
   * (napi_value).
   *
   * @param object The object whose properties are being checked.
   * @param key The key (property name) in the form of a `napi_value`.
   * @return True if the property exists on the object, false otherwise.
   */
  bool hasProperty(napi_value object, napi_value key);

  /**
   * @brief Converts a `napi_value` to a boolean.
   *
   * This method extracts the boolean value from the provided `napi_value`.
   *
   * @param value The `napi_value` to be converted to a boolean.
   * @return The boolean value represented by the `napi_value`.
   *
   * @throws napi_status exception If the conversion fails.
   */
  bool getBoolean(napi_value value);

  /**
   * @brief Converts a `napi_value` to a double.
   *
   * This method extracts the double value from the provided `napi_value`.
   *
   * @param value The `napi_value` to be converted to a double.
   * @return The double value represented by the `napi_value`.
   * @throws napi_status exception If the conversion fails.
   */
  double getDouble(napi_value value);

  /**
   * @brief Converts a `napi_value` to an integer.
   *
   * This method extracts the integer value from the provided `napi_value`.
   *
   * @param value The `napi_value` to be converted to an integer.
   * @return The integer value represented by the `napi_value`.
   * @throws napi_status exception If the conversion fails.
   */
  int getInteger(napi_value value);

  /**
   * @brief Retrieves an element from an array at the specified index.
   *
   * This function retrieves the value at a given index from an ArkTS array.
   *
   * @param array The `napi_value` representing the ArkTS array.
   * @param index The index of the element to retrieve.
   * @return The `napi_value` representing the element at the specified index.
   * @throws napi_status exception If the element retrieval fails.
   */
  napi_value getArrayElement(napi_value array, uint32_t index);

  /**
   * @brief Retrieves the length of an ArkTS array.
   *
   * This function returns the number of elements in an ArkTS array.
   *
   * @param array The `napi_value` representing the ArkTS array.
   * @return The number of elements in the array.
   * @throws napi_status exception If the length retrieval fails.
   */
  uint32_t getArrayLength(napi_value array);

  /**
   * @brief Retrieves the contents of an ArrayBuffer as a vector of bytes.
   *
   * This function extracts the binary data from a `napi_value` representing an
   * ArrayBuffer and returns it as a vector of `uint8_t` values.
   *
   * @param array The `napi_value` representing the ArrayBuffer.
   * @return A vector of bytes representing the ArrayBuffer's contents.
   * @throws napi_status exception If the ArrayBuffer retrieval fails.
   */
  std::vector<uint8_t> getArrayBuffer(napi_value array);

  /**
   * @brief Checks if a `napi_value` is an ArrayBuffer.
   *
   * This function checks whether the provided `napi_value` represents an
   * ArrayBuffer. It returns `true` if the value is an ArrayBuffer, and `false`
   * otherwise.
   *
   * @param value The `napi_value` to check.
   * @return `true` if the value is an ArrayBuffer, otherwise `false`.
   * @throws napi_status exception If the operation to check the type fails.
   */
  bool isArrayBuffer(napi_value value);
  /**
   * @brief Retrieves all properties of an ArkTS object as key-value pairs.
   *
   * This function retrieves all the properties (keys and values) of an ArkTS
   * object and returns them as a vector of pairs.
   *
   * @param object The `napi_value` representing the ArkTS object.
   * @return A vector of key-value pairs representing the object's properties.
   * @throws napi_status exception If the property retrieval fails.
   */
  std::vector<std::pair<napi_value, napi_value>> getObjectProperties(
      napi_value object);
  /**
   * @brief Retrieves the string value from a `napi_value`.
   *
   * This function extracts the string value from a `napi_value` and returns it
   * as a `std::string`.
   *
   * @param value The `napi_value` representing the ArkTS string.
   * @return The string value represented by the `napi_value`.
   * @throws napi_status exception If the string retrieval fails.
   */
  std::string getString(napi_value value);

  /**
   * @brief Converts a NAPI value to a `folly::dynamic` object.
   *
   * This function converts a given NAPI value into a `folly::dynamic` object by
   * checking its type and then calling the appropriate conversion function. It
   * handles various types like undefined, null, boolean, number, string,
   * object, and array.
   *
   * - If the value is an object, it recursively converts its properties.
   * - If the value is an array, it recursively converts its elements.
   *
   * @param value The NAPI value to be converted.
   * @return A `folly::dynamic` representation of the NAPI value.
   */
  folly::dynamic getDynamic(napi_value value);

  /**
   * @brief Converts a vector of NAPI values into a vector of `folly::dynamic`
   * objects.
   *
   * This function processes a vector of `napi_value`s and converts each into a
   * `folly::dynamic` object. It returns a vector of `folly::dynamic` objects
   * corresponding to the provided values.
   *
   * @param values A vector of NAPI values to be converted.
   * @return A vector of `folly::dynamic` objects.
   */
  std::vector<folly::dynamic> getDynamics(std::vector<napi_value> values);

  /**
   * @brief Gets the NAPI environment associated with this instance.
   *
   * This function retrieves the `napi_env` (NAPI environment) which is a handle
   * to the execution environment for NAPI operations. It provides the context
   * in which ArkTS code is executed and allows interaction with ArkTS
   * values and functions.
   *
   * @return The NAPI environment (`napi_env`) associated with this instance.
   */
  napi_env getEnv();

  /**
   * @brief Gets the type of a NAPI value.
   *
   * This function retrieves the type of a given `napi_value` by calling the
   * NAPI `napi_typeof` function.
   * It returns the type as an `napi_valuetype` enum value.
   *
   * @param value The NAPI value whose type is to be determined.
   * @return The type of the value as an `napi_valuetype` enum.
   * @throws napi_status exception If the operation to retrieve the type fails.
   */
  napi_valuetype getType(napi_value value);

  /**
   * @brief Converts intermediary arguments to NAPI values.
   *
   * Converts intermediary arguments, such as `folly::dynamic` or callback
   * functions, to their corresponding NAPI values.
   *
   * @param arg The intermediary argument.
   * @return The corresponding NAPI value.
   */
  napi_value convertIntermediaryValueToNapiValue(IntermediaryArg arg);

  /**
   * @brief Converts multiple intermediary arguments to NAPI values.
   *
   * Converts a collection of intermediary arguments to their corresponding NAPI
   * values.
   *
   * @param args The intermediary arguments.
   * @return The corresponding NAPI values.
   */
  std::vector<napi_value> convertIntermediaryValuesToNapiValues(
      std::vector<IntermediaryArg> args);

 private:
  napi_env m_env;

  void maybeThrowFromStatus(napi_status status, const char* message);

  void maybeRethrowAsCpp(napi_status status);
};

class RNOHNapiObject {
 public:
  RNOHNapiObject(ArkJS arkJS, napi_value object);

  template <size_t args_count>
  napi_value call(
      std::string const& key,
      std::array<napi_value, args_count> args) {
    return m_arkJS.call(this->getProperty(key), args, m_object);
  }

  napi_value call(std::string const& key, std::vector<napi_value> args) {
    return m_arkJS.call(this->getProperty(key), args, m_object);
  }

  napi_value
  call(std::string const& key, const napi_value* args, int argsCount) {
    return m_arkJS.call(this->getProperty(key), args, argsCount, m_object);
  }

  napi_value getProperty(std::string const& key);

  napi_value getProperty(napi_value key);

  std::vector<std::pair<napi_value, napi_value>> getKeyValuePairs();

 private:
  ArkJS m_arkJS;
  napi_value m_object;

  friend class RNOHNapiObjectBuilder;
};

class RNOHNapiObjectBuilder {
 public:
  RNOHNapiObjectBuilder(napi_env env, ArkJS arkJS);

  RNOHNapiObjectBuilder(napi_env env, ArkJS arkJS, napi_value object);

  RNOHNapiObjectBuilder& addProperty(const char* name, napi_value value);

  RNOHNapiObjectBuilder& addProperty(const char* name, bool value);

  RNOHNapiObjectBuilder& addProperty(const char* name, int value);

  RNOHNapiObjectBuilder& addProperty(
      const char* name,
      std::array<facebook::react::Float, 16> matrix);

  RNOHNapiObjectBuilder& addProperty(
      const char* name,
      facebook::react::Float value);

  RNOHNapiObjectBuilder& addProperty(const char* name, char const* value);

  RNOHNapiObjectBuilder& addProperty(
      const char* name,
      facebook::react::SharedColor value);

  RNOHNapiObjectBuilder& addProperty(
      const char* name,
      facebook::react::RectangleCorners<facebook::react::Float> value);

  RNOHNapiObjectBuilder& addProperty(const char* name, std::string value);

  RNOHNapiObjectBuilder& addProperty(const char* name, folly::dynamic value);

  napi_value build();

 private:
  ArkJS m_arkJS;
  napi_env m_env;
  napi_value m_object;
  std::vector<std::pair<std::string, napi_value>> m_properties;
};

class Promise {
 public:
  Promise(napi_env env, napi_value value);

  Promise& then(std::function<void(std::vector<folly::dynamic>)>&& callback);
  Promise& catch_(std::function<void(std::vector<folly::dynamic>)>&& callback);

 private:
  ArkJS m_arkJS;
  napi_value m_value;
};

class NapiRef final {
 public:
  NapiRef() = default;

  // rule of five constructors
  NapiRef(NapiRef const&) = default;
  NapiRef& operator=(NapiRef const&) = default;

  NapiRef(NapiRef&&) = default;
  NapiRef& operator=(NapiRef&&) = default;

  ~NapiRef() = default;

  operator bool() const {
    return m_ref != nullptr;
  }

 private:
  class Deleter {
   public:
    void operator()(napi_ref ref) const {
      m_threadGuard.assertThread();
      napi_delete_reference(m_env, ref);
    }

    napi_env m_env;
    rnoh::ThreadGuard m_threadGuard;
  };

  NapiRef(napi_env env, napi_ref ref) : m_env(env), m_ref(ref, Deleter{env}) {}

  napi_env m_env{};
  std::shared_ptr<napi_ref__> m_ref{nullptr};

  friend class ArkJS;
};

#endif // native_ArkJS_H