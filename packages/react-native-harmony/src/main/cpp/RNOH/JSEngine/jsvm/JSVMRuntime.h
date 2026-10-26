/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef JSVMRUNTIME_H
#define JSVMRUNTIME_H
#include <cxxreact/MessageQueueThread.h>
#include <jsi/instrumentation.h>
#include <react/debug/react_native_assert.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <iosfwd>
#include <unordered_map>
#include "JSVMUtil.h"
#include "ark_runtime/jsvm.h"
#include "common.h"
#include "folly/dynamic.h"

namespace jsvm {

class JSVMRuntime;
class JSVMPointerValue;
class JSVMConverter;

class JSVMRuntime : public Runtime {
 public:
  explicit JSVMRuntime(folly::dynamic initOptions);
  JSVMRuntime(
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
      folly::dynamic initOptions);
  ~JSVMRuntime();

  virtual Value evaluateJavaScript(
      const std::shared_ptr<const Buffer>& buffer,
      const std::string& sourceURL);

  virtual std::shared_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer>& buffer,
      std::string sourceURL);

  virtual Value evaluatePreparedJavaScript(
      const std::shared_ptr<const PreparedJavaScript>& js);

  void queueMicrotask(const Function& callback);
  bool drainMicrotasks(int maxMicrotasksHint = -1);

  Object global();

  std::string description();

  bool isInspectable();

  JSVM_Status getHeapStatistics(JSVM_HeapStatistics* result);

  void notifyMemoryPressure(JSVM_MemoryPressureLevel level);
  Instrumentation& instrumentation();

 protected:
  Runtime::PointerValue* cloneSymbol(const Runtime::PointerValue* pv);
  Runtime::PointerValue* cloneBigInt(const Runtime::PointerValue* pv);
  Runtime::PointerValue* cloneString(const Runtime::PointerValue* pv);
  Runtime::PointerValue* cloneObject(const Runtime::PointerValue* pv);
  Runtime::PointerValue* clonePropNameID(const Runtime::PointerValue* pv);
  inline Runtime::PointerValue* clone(const Runtime::PointerValue* pv);

  virtual PropNameID createPropNameIDFromAscii(const char* str, size_t length);
  virtual PropNameID createPropNameIDFromUtf8(
      const uint8_t* utf8,
      size_t length);
  PropNameID createPropNameIDFromString(const String& str);
  PropNameID createPropNameIDFromSymbol(const Symbol& sym);
  std::string utf8(const PropNameID&);
  bool compare(const PropNameID&, const PropNameID&);

  std::string symbolToString(const Symbol&);

  BigInt createBigIntFromInt64(int64_t);
  BigInt createBigIntFromUint64(uint64_t);
  bool bigintIsInt64(const BigInt&);
  bool bigintIsUint64(const BigInt&);
  uint64_t truncate(const BigInt&);
  String bigintToString(const BigInt&, int);

  String createStringFromAscii(const char* str, size_t length);
  String createStringFromUtf8(const uint8_t* utf8, size_t length);
  std::string utf8(const String&);

  // \return a \c Value created from a utf8-encoded JSON string. The default
  // implementation creates a \c String and invokes JSON.parse.
  virtual Value createValueFromJsonUtf8(const uint8_t* json, size_t length);

  Object createObject();
  Object createObject(std::shared_ptr<HostObject> ho);
  std::shared_ptr<HostObject> getHostObject(const Object&);
  HostFunctionType& getHostFunction(const Function&);

  bool hasNativeState(const Object&);
  std::shared_ptr<NativeState> getNativeState(const Object&);
  virtual void setNativeState(
      const Object&,
      std::shared_ptr<NativeState> state);

  Value getProperty(const Object&, const PropNameID& name);
  Value getProperty(const Object&, const String& name);
  bool hasProperty(const Object&, const PropNameID& name);
  bool hasProperty(const Object&, const String& name);
  virtual void
  setPropertyValue(const Object&, const PropNameID& name, const Value& value);
  virtual void
  setPropertyValue(const Object&, const String& name, const Value& value);

  bool isArray(const Object&) const;
  bool isArrayBuffer(const Object&) const;
  bool isFunction(const Object&) const;
  bool isHostObject(const Object&) const;
  bool isHostFunction(const Function&) const;
  Array getPropertyNames(const Object&);

  WeakObject createWeakObject(const Object&);
  Value lockWeakObject(const WeakObject&);

  Array createArray(size_t length);
  virtual ArrayBuffer createArrayBuffer(std::shared_ptr<MutableBuffer> buffer);
  size_t size(const Array&);
  size_t size(const ArrayBuffer&);
  uint8_t* data(const ArrayBuffer&);
  Value getValueAtIndex(const Array&, size_t i);
  virtual void setValueAtIndexImpl(const Array&, size_t i, const Value& value);

  virtual Function createFunctionFromHostFunction(
      const PropNameID& name,
      unsigned int paramCount,
      HostFunctionType func);
  virtual Value
  call(const Function&, const Value& jsThis, const Value* args, size_t count);
  virtual Value
  callAsConstructor(const Function&, const Value* args, size_t count);

  bool strictEqualsCommon(const Pointer& a, const Pointer& b) const;
  bool strictEquals(const Symbol& a, const Symbol& b) const;
  bool strictEquals(const BigInt& a, const BigInt& b) const;
  bool strictEquals(const String& a, const String& b) const;
  bool strictEquals(const Object& a, const Object& b) const;

  bool instanceOf(const Object& o, const Function& f);

  std::vector<std::pair<std::string, Value>> getObjectProperties(
      const Object& object);

  void setExternalMemoryPressure(const Object& obj, size_t amount);

 protected:
  void ThrowError();
  inline void ClearException() const;
  inline void ClearException(JSVM_Status status) const;
  inline std::string GetValueString(JSVM_Value value);
  inline JSVM_Value GetGlobalProperty(const char* str);
  inline JSVM_Value CallGlobalFunction(
      const char* funcName,
      size_t length,
      JSVM_Value* args,
      uint32_t argc);
  std::vector<uint8_t> GetCodeCache(const std::string& sourceURL);
  void UpdateCodeCache(
      const std::string& sourceURL,
      const std::vector<uint8_t>& buffer);

 private:
  JSVM_CreateVMOptions options;
  JSVM_VM vm;
  JSVM_VMScope vmScope;
  JSVM_Env env;
  JSVM_EnvScope envScope;
  static std::mutex codeCacheMtx;
  // L1 code cahce is in the disk
  // L2 code cache is in the memory
  static std::unordered_map<std::string, std::vector<uint8_t>> codeCacheL2;
  std::vector<uint8_t> GetCodeCacheL1(const std::string& sourceURL);
  void UpdateCodeCacheL1(
      const std::string& sourceURL,
      const std::vector<uint8_t>& buffer);
  std::vector<uint8_t> GetCodeCacheL2(const std::string& sourceURL);
  void UpdateCodeCacheL2(
      const std::string& sourceURL,
      const std::vector<uint8_t>& buffer);
  const char* cachePath = "/data/storage/el2/base/cache/js";
  static bool initialized;
  std::shared_ptr<facebook::react::MessageQueueThread> jsQueue;
  JSVM_Ref hostObjectClass;
  std::deque<Function> microtaskQueue_;

  class JSVMInstrumentation : public Instrumentation {
   public:
    explicit JSVMInstrumentation(JSVMRuntime& runtime) : runtime_(runtime) {}

    std::string getRecordedGCStats() override {
      return "";
    }

    std::unordered_map<std::string, int64_t> getHeapInfo(bool) override {
      return std::unordered_map<std::string, int64_t>{};
    }

    void collectGarbage(std::string cause) override;

    void startTrackingHeapObjectStackTraces(
        std::function<void(
            uint64_t,
            std::chrono::microseconds,
            std::vector<HeapStatsUpdate>)>) override {}
    void stopTrackingHeapObjectStackTraces() override {}

    void startHeapSampling(size_t) override {}
    void stopHeapSampling(std::ostream&) override {}

    void createSnapshotToFile(const std::string&, const HeapSnapshotOptions&)
        override {}
    void createSnapshotToStream(std::ostream&, const HeapSnapshotOptions&)
        override {}

    std::string flushAndDisableBridgeTrafficTrace() override {
      return "";
    }

    void writeBasicBlockProfileTraceToFile(const std::string&) const override {}
    void dumpProfilerSymbolsToFile(const std::string&) const override {}
    void dumpOpcodeStats(std::ostream& os) const override {}

   private:
    JSVMRuntime& runtime_;
  };

  JSVMInstrumentation instrumentation_;

 private:
  enum { RADIX_MIN = 2, RADIX_MAX = 36 };
  friend class JSVMPointerValue;
  friend class JSVMConverter;
};

class JSVMPointerValue : public JSVMRuntime::PointerValue {
 public:
  template <bool isWeak>
  static JSVMPointerValue* New(JSVM_Env env, const JSVM_Value value) {
    JSVMPointerValue* ptr = new JSVMPointerValue(env);
    OH_JSVM_CreateReference(env, value, isWeak ? 0 : 1, &(ptr->reference));

    ptr->next = head->next;
    ptr->prev = head;
    if (head->next) {
      head->next->prev = ptr;
    }
    head->next = ptr;

    return ptr;
  }

  static void init() {
    head = new JSVMPointerValue(nullptr);
    isJsThread = true;
  }

  static void ReleasePointerValueList() {
    if (!head) {
      return;
    }

    // Walk the list starting from the first real node after the sentinel,
    // unlink each node and delete it. We intentionally do NOT call any JSVM
    // APIs here (such as OH_JSVM_DeleteReference). JSVM environment teardown
    // is responsible for cleaning up VM-side references.
    JSVMPointerValue* current = head->next;
    while (current) {
      JSVMPointerValue* nextNode = current->next;

      // Detach from the list.
      current->prev = nullptr;
      current->next = nullptr;

      delete current;
      current = nextNode;
    }

    delete head;
    head = nullptr;
  }

  explicit JSVMPointerValue(JSVM_Env env)
      : reference(nullptr),
        env(env),
        refcount(1),
        prev(nullptr),
        next(nullptr) {}

  ~JSVMPointerValue() {
    // env and reference are set to nullptr here to avoid double free.
    // CleanUp() calls OH_JSVM_DeleteReference and then delete this (which
    // triggers this destructor), so we must not free them again.
    env = nullptr;
    reference = nullptr;
    next = nullptr;
    prev = nullptr;
  }

  JSVM_Value GetValue() const {
    JSVM_Value value = nullptr;
    auto status = OH_JSVM_GetReferenceValue(env, reference, &value);
    if (status != JSVM_OK) {
      LOG(ERROR) << "JSVM GetReferenceValue Failed";
    }
    return value;
  }

  JSVMPointerValue* Ref() {
    ++refcount;
    return this;
  }

 private:
  enum { WEAK_REF_COUNT = 0, STRONG_REF_COUNT = 1, DUPLICATE_REF_COUNT = 2 };

  void CleanUp() {
    react_native_assert(isJsThread && "Should be called in js thread");
    // Has been cleaned up
    if (unlikely(!env)) {
      return;
    }

    // Unlink
    react_native_assert(prev && "prev must not be null");
    prev->next = next;

    if (next) {
      next->prev = prev;
    }

    // Clean up js resource
    OH_JSVM_DeleteReference(env, reference);
    next = nullptr;
    prev = nullptr;

    delete this;
  }

  void invalidate() noexcept override {
    if (--refcount == 0) {
      // Case1: jsi::Value is deconstructed after its corresponding JSVMRuntime
      // has been deconstructed. Resource has already been released, just
      // delete.
      if (unlikely(!env)) {
        delete this;
        return;
      }
      if (!unlikely(!isJsThread)) {
        CleanUp();
      } else {
        void* data = nullptr;
        OH_JSVM_GetInstanceData(env, &data);
        facebook::react::MessageQueueThread* jsQueue =
            static_cast<facebook::react::MessageQueueThread*>(data);
        jsQueue->runOnQueue([this]() {
          if (head) {
            CleanUp();
          }
        });
      }
    }
  }

  JSVM_Ref reference;
  JSVM_Env env;
  std::atomic<uint64_t> refcount;
  JSVMPointerValue* prev;
  JSVMPointerValue* next;

 private:
  friend class JSVMRuntime;
  friend class JSVMConverter;
  static thread_local JSVMPointerValue* head;
  static thread_local bool isJsThread;
};

class JSVMPreparedJavaScript final : public PreparedJavaScript {
  const uint8_t* codeCachePtr_;
  const size_t codeCacheLength_;
  const std::shared_ptr<const Buffer> buf_;

 public:
  explicit JSVMPreparedJavaScript(
      const uint8_t* codeCachePtr,
      size_t codeCacheLength,
      const std::shared_ptr<const Buffer> buf)
      : codeCachePtr_(codeCachePtr),
        codeCacheLength_(codeCacheLength),
        buf_(std::move(buf)) {}

  ~JSVMPreparedJavaScript() {
    delete[] codeCachePtr_;
  }

  const uint8_t* codeCache() const {
    return codeCachePtr_;
  }

  const size_t codeLength() const {
    return codeCacheLength_;
  }

  size_t sourceLength() const {
    return buf_->size();
  }

  const uint8_t* source() const {
    return buf_->data();
  }
};
} // namespace jsvm

#endif
