/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ExceptionStackHandler.h"

#include <bundle/native_interface_bundle.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <hiappevent/hiappevent.h>
#include <hiappevent/hiappevent_cfg.h>
#include <hiappevent/hiappevent_event.h>
#include <jsi/jsi.h>
#include <unistd.h>
#include <array>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string_view>
#include "ApiVersionCheck.h"

#if USE_HERMES
#include <hermes/hermes.h>
#endif

namespace rnoh {

namespace {

constexpr size_t MAX_CRASH_JS_STACK_BYTES = 100 * 1024;
constexpr std::array<int, 4> HANDLED_CRASH_SIGNALS = {
    SIGSEGV,
    SIGABRT,
    SIGFPE,
    SIGBUS,
};
constexpr char CRASH_BANNER[] = "\n========================================\n";
constexpr char SIGNAL_PREFIX[] = "RNOH crash signal captured: ";
constexpr char SIGNAL_NUMBER_PREFIX[] = "signal_number: ";
constexpr char SIGNAL_CODE_PREFIX[] = "signal_code: ";
constexpr char FAULT_ADDR_PREFIX[] = "fault_address: ";
constexpr char CACHED_STACK_BYTES_PREFIX[] = "cached_js_stack_bytes: ";
constexpr char JS_STACK_HEADER[] = "\n=== Cached JavaScript stack ===\n";
constexpr char JS_STACK_EMPTY[] =
    "\nNo cached JavaScript stack is available.\n";
constexpr char CRASH_FOOTER[] = "========================================\n";
constexpr char STACK_TRUNCATED_SUFFIX[] =
    "\n...[truncated by RNOH crash handler]\n";
constexpr char CRASH_LOG_DIR[] = "/data/storage/el2/log";
constexpr char CRASH_LOG_FILE_SUFFIX[] = "_CppCrash_AppMerge.log";
constexpr char CAPTURE_JS_STACK_FUNCTION_NAME[] = "__captureNativeCrashJSStack";

std::once_flag g_crashSignalHandlersInstallOnce;
std::once_flag g_cppCrashAppLogMergeConfigOnce;
std::once_flag g_cachedJSStackFilePathInitOnce;
std::array<struct sigaction, HANDLED_CRASH_SIGNALS.size()>
    g_previousSignalActions{};
std::array<std::array<char, MAX_CRASH_JS_STACK_BYTES + 1>, 2>
    g_cachedJSStacks{};
volatile sig_atomic_t g_cachedJSStackLengths[2] = {0, 0};
volatile sig_atomic_t g_activeJSStackSlot = 0;
std::string g_cachedJSStackFilePath;

int findHandledSignalIndex(int signal) {
  for (size_t idx = 0; idx < HANDLED_CRASH_SIGNALS.size(); ++idx) {
    if (HANDLED_CRASH_SIGNALS[idx] == signal) {
      return static_cast<int>(idx);
    }
  }
  return -1;
}

const char* signalToName(int signal) {
  switch (signal) {
    case SIGSEGV:
      return "SIGSEGV";
    case SIGABRT:
      return "SIGABRT";
    case SIGFPE:
      return "SIGFPE";
    case SIGBUS:
      return "SIGBUS";
    default:
      return "UNKNOWN";
  }
}

size_t signalNameSize(int signal) {
  switch (signal) {
    case SIGSEGV:
      return sizeof("SIGSEGV") - 1;
    case SIGABRT:
      return sizeof("SIGABRT") - 1;
    case SIGFPE:
      return sizeof("SIGFPE") - 1;
    case SIGBUS:
      return sizeof("SIGBUS") - 1;
    default:
      return sizeof("UNKNOWN") - 1;
  }
}

void writeSignalSafe(const char* data, size_t size) {
  while (data != nullptr && size > 0) {
    const auto written = ::write(STDERR_FILENO, data, size);
    if (written <= 0) {
      return;
    }
    data += written;
    size -= static_cast<size_t>(written);
  }
}

size_t appendUnsignedDecimal(char* buffer, size_t bufferSize, uint64_t value) {
  if (buffer == nullptr || bufferSize == 0) {
    return 0;
  }

  char reversed[32];
  size_t reversedLength = 0;
  do {
    reversed[reversedLength++] = static_cast<char>('0' + (value % 10));
    value /= 10;
  } while (value > 0 && reversedLength < sizeof(reversed));

  size_t length = 0;
  while (reversedLength > 0 && length < bufferSize) {
    buffer[length++] = reversed[--reversedLength];
  }
  return length;
}

size_t appendHex(char* buffer, size_t bufferSize, uintptr_t value) {
  if (buffer == nullptr || bufferSize < 3) {
    return 0;
  }

  constexpr char HEX_DIGITS[] = "0123456789abcdef";
  char reversed[2 * sizeof(uintptr_t)];
  size_t reversedLength = 0;
  do {
    reversed[reversedLength++] = HEX_DIGITS[value & 0xF];
    value >>= 4;
  } while (value > 0 && reversedLength < sizeof(reversed));

  size_t length = 0;
  buffer[length++] = '0';
  buffer[length++] = 'x';
  while (reversedLength > 0 && length < bufferSize) {
    buffer[length++] = reversed[--reversedLength];
  }
  return length;
}

void writeSignalSafeLineWithUInt(
    const char* prefix,
    size_t prefixSize,
    uint64_t value) {
  char numberBuffer[32];
  const auto numberLength =
      appendUnsignedDecimal(numberBuffer, sizeof(numberBuffer), value);
  writeSignalSafe(prefix, prefixSize);
  writeSignalSafe(numberBuffer, numberLength);
  writeSignalSafe("\n", 1);
}

void writeSignalSafeLineWithHex(
    const char* prefix,
    size_t prefixSize,
    uintptr_t value) {
  char hexBuffer[2 + (2 * sizeof(uintptr_t))];
  const auto hexLength = appendHex(hexBuffer, sizeof(hexBuffer), value);
  writeSignalSafe(prefix, prefixSize);
  writeSignalSafe(hexBuffer, hexLength);
  writeSignalSafe("\n", 1);
}

std::string fitStackToCache(std::string_view stack) {
  if (stack.size() <= MAX_CRASH_JS_STACK_BYTES) {
    return std::string(stack);
  }

  constexpr auto truncatedSuffixSize = sizeof(STACK_TRUNCATED_SUFFIX) - 1;
  const auto prefixSize = MAX_CRASH_JS_STACK_BYTES > truncatedSuffixSize
      ? MAX_CRASH_JS_STACK_BYTES - truncatedSuffixSize
      : 0;

  std::string fitted(stack.substr(0, prefixSize));
  fitted.append(STACK_TRUNCATED_SUFFIX, truncatedSuffixSize);
  return fitted;
}

std::string readProcessNameFromProc() {
  std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
  if (!cmdline.is_open()) {
    return "";
  }

  std::string processName;
  std::getline(cmdline, processName, '\0');
  return processName;
}

std::string resolveBundleName() {
  auto appInfo = OH_NativeBundle_GetCurrentApplicationInfo();
  if (appInfo.bundleName != nullptr && std::strlen(appInfo.bundleName) > 0) {
    return appInfo.bundleName;
  }

  return readProcessNameFromProc();
}

void initializeCachedJSStackFilePath() {
  std::call_once(g_cachedJSStackFilePathInitOnce, []() {
    auto bundleName = resolveBundleName();
    if (bundleName.empty()) {
      bundleName = "unknown";
    }

    std::error_code ec;
    std::filesystem::create_directories(CRASH_LOG_DIR, ec);

    if (ec) {
      LOG(WARNING) << "[CrashStack] Failed to ensure crash log dir exists: "
                   << CRASH_LOG_DIR << ", error=" << ec.message();
    }

    g_cachedJSStackFilePath =
        std::string(CRASH_LOG_DIR) + "/" + bundleName + CRASH_LOG_FILE_SUFFIX;
  });
}

bool overwriteCachedJSStackFile(std::string_view stack) {
  initializeCachedJSStackFilePath();

  if (g_cachedJSStackFilePath.empty()) {
    LOG(ERROR) << "[CrashStack] JS stack merge log path is empty";
    return false;
  }

  const int fd = ::open(
      g_cachedJSStackFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    const auto errorCode = errno;
    LOG(ERROR) << "[CrashStack] Failed to open JS stack merge log file: "
               << g_cachedJSStackFilePath << ", errno=" << errorCode
               << ", error=" << std::strerror(errorCode);
    return false;
  }

  const char* data = stack.data();
  size_t remaining = stack.size();
  while (remaining > 0) {
    const auto written = ::write(fd, data, remaining);
    if (written <= 0) {
      const auto errorCode = errno;
      LOG(ERROR) << "[CrashStack] Failed to write JS stack merge log file: "
                 << g_cachedJSStackFilePath << ", errno=" << errorCode
                 << ", error=" << std::strerror(errorCode);
      ::close(fd);
      return false;
    }
    data += written;
    remaining -= static_cast<size_t>(written);
  }

  if (::close(fd) != 0) {
    const auto errorCode = errno;
    LOG(ERROR) << "[CrashStack] Failed to close JS stack merge log file: "
               << g_cachedJSStackFilePath << ", errno=" << errorCode
               << ", error=" << std::strerror(errorCode);
    return false;
  }

  return true;
}

bool isDefaultOrIgnoredSignalActionHandler(
    const struct sigaction& signalAction) {
  return signalAction.sa_handler == SIG_DFL ||
      signalAction.sa_handler == SIG_IGN;
}

bool isDefaultOrIgnoredSignalActionSiginfo(
    const struct sigaction& signalAction) {
  const auto handler = signalAction.sa_sigaction;
  return handler == nullptr ||
      handler == reinterpret_cast<void (*)(int, siginfo_t*, void*)>(SIG_DFL) ||
      handler == reinterpret_cast<void (*)(int, siginfo_t*, void*)>(SIG_IGN);
}

void dispatchPreviousSignalActionIfNeeded(
    int signal,
    siginfo_t* signalInfo,
    void* context) {
  const auto signalIndex = findHandledSignalIndex(signal);
  if (signalIndex < 0) {
    return;
  }

  const auto& previousSignalAction = g_previousSignalActions[signalIndex];
  if ((previousSignalAction.sa_flags & SA_SIGINFO) != 0) {
    if (!isDefaultOrIgnoredSignalActionSiginfo(previousSignalAction)) {
      previousSignalAction.sa_sigaction(signal, signalInfo, context);
    }
    return;
  }

  if (!isDefaultOrIgnoredSignalActionHandler(previousSignalAction)) {
    previousSignalAction.sa_handler(signal);
  }
}

[[noreturn]] void
rethrowSignalAndExit(int signal, siginfo_t* signalInfo, void* context) {
  dispatchPreviousSignalActionIfNeeded(signal, signalInfo, context);
  ::kill(::getpid(), signal);
  _exit(128 + signal);
}

void crashSignalHandler(int signal, siginfo_t* signalInfo, void* context) {
  writeSignalSafe(CRASH_BANNER, sizeof(CRASH_BANNER) - 1);
  writeSignalSafe(SIGNAL_PREFIX, sizeof(SIGNAL_PREFIX) - 1);
  writeSignalSafe(signalToName(signal), signalNameSize(signal));
  writeSignalSafe("\n", 1);
  writeSignalSafeLineWithUInt(
      SIGNAL_NUMBER_PREFIX,
      sizeof(SIGNAL_NUMBER_PREFIX) - 1,
      static_cast<uint64_t>(signal));

  if (signalInfo != nullptr) {
    writeSignalSafeLineWithUInt(
        SIGNAL_CODE_PREFIX,
        sizeof(SIGNAL_CODE_PREFIX) - 1,
        static_cast<uint64_t>(signalInfo->si_code));
    if ((signal == SIGSEGV || signal == SIGBUS) &&
        signalInfo->si_addr != nullptr) {
      writeSignalSafeLineWithHex(
          FAULT_ADDR_PREFIX,
          sizeof(FAULT_ADDR_PREFIX) - 1,
          reinterpret_cast<uintptr_t>(signalInfo->si_addr));
    }
  }

  const auto activeSlot = g_activeJSStackSlot;
  const auto stackLength = activeSlot >= 0 && activeSlot < 2
      ? g_cachedJSStackLengths[activeSlot]
      : 0;

  writeSignalSafeLineWithUInt(
      CACHED_STACK_BYTES_PREFIX,
      sizeof(CACHED_STACK_BYTES_PREFIX) - 1,
      stackLength > 0 ? static_cast<uint64_t>(stackLength) : 0);

  if (stackLength > 0) {
    writeSignalSafe(JS_STACK_HEADER, sizeof(JS_STACK_HEADER) - 1);
    writeSignalSafe(
        g_cachedJSStacks[activeSlot].data(), static_cast<size_t>(stackLength));
    writeSignalSafe("\n", 1);
  } else {
    writeSignalSafe(JS_STACK_EMPTY, sizeof(JS_STACK_EMPTY) - 1);
  }

  writeSignalSafe(CRASH_FOOTER, sizeof(CRASH_FOOTER) - 1);
  rethrowSignalAndExit(signal, signalInfo, context);
}

void installCrashSignalHandlersOnce() {
  std::call_once(g_crashSignalHandlersInstallOnce, []() {
    initializeCachedJSStackFilePath();
    for (size_t idx = 0; idx < HANDLED_CRASH_SIGNALS.size(); ++idx) {
      struct sigaction signalAction {};
      signalAction.sa_sigaction = &crashSignalHandler;
      ::sigemptyset(&signalAction.sa_mask);
      signalAction.sa_flags = SA_SIGINFO | SA_RESTART | SA_RESETHAND;

      if (::sigaction(
              HANDLED_CRASH_SIGNALS[idx],
              &signalAction,
              &g_previousSignalActions[idx]) != 0) {
        const auto errorCode = errno;
        LOG(ERROR) << "[CrashStack] Failed to install handler for "
                   << signalToName(HANDLED_CRASH_SIGNALS[idx])
                   << ", errno=" << errorCode
                   << ", error=" << std::strerror(errorCode);
      }
    }
  });
}

void storeProcessWideCppCrashJSStack(std::string_view stack) {
  const auto boundedStack = fitStackToCache(stack);
  const auto nextSlot = g_activeJSStackSlot == 0 ? 1 : 0;
  const auto bytesToCopy = boundedStack.size();

  if (bytesToCopy > 0) {
    std::memcpy(
        g_cachedJSStacks[nextSlot].data(), boundedStack.data(), bytesToCopy);
  }
  g_cachedJSStacks[nextSlot][bytesToCopy] = '\0';
  g_cachedJSStackLengths[nextSlot] = static_cast<sig_atomic_t>(bytesToCopy);
  std::atomic_signal_fence(std::memory_order_seq_cst);
  g_activeJSStackSlot = static_cast<sig_atomic_t>(nextSlot);

  overwriteCachedJSStackFile(boundedStack);
}

std::string getStackArgument(
    facebook::jsi::Runtime& rt,
    const facebook::jsi::Value* args,
    size_t count,
    size_t index) {
  if (index < count && args[index].isString()) {
    return args[index].getString(rt).utf8(rt);
  }
  return "";
}

std::string captureCurrentCppCrashJSStack(facebook::jsi::Runtime& rt) {
#if USE_HERMES
  if (auto* hermesRuntime =
          dynamic_cast<facebook::hermes::HermesRuntime*>(&rt)) {
    try {
      static const auto kCaptureScript =
          std::make_shared<facebook::jsi::StringBuffer>(
              R"((function() {
                try {
                  const err = new Error();
                  if (typeof err.stack === 'string' && err.stack.length > 0) {
                    return err.stack;
                  }
                  return 'Error.stack is unavailable.';
                } catch (e) {
                  return 'Failed to capture JS stack: ' + String(e && e.message ? e.message : e);
                }
              })())");

      auto result = hermesRuntime->evaluateJavaScript(
          kCaptureScript, "rnoh_cppcrash_stack_capture.js");
      if (result.isString()) {
        return result.getString(rt).utf8(rt);
      }

      return "Error.stack is unavailable.";
    } catch (const std::exception& err) {
      LOG(WARNING)
          << "[CrashStack] Hermes stack capture failed, fallback to standard JSI: "
          << err.what();
    } catch (...) {
      LOG(WARNING)
          << "[CrashStack] Hermes stack capture failed, fallback to standard JSI.";
    }
  }
#endif

  try {
    auto errorCtorValue = rt.global().getProperty(rt, "Error");
    if (!errorCtorValue.isObject() ||
        !errorCtorValue.asObject(rt).isFunction(rt)) {
      return "Error constructor is unavailable.";
    }

    auto errorObject = errorCtorValue.asObject(rt)
                           .asFunction(rt)
                           .callAsConstructor(rt)
                           .asObject(rt);
    auto stackValue = errorObject.getProperty(rt, "stack");
    if (stackValue.isString()) {
      return stackValue.getString(rt).utf8(rt);
    }

    return "Error.stack is unavailable.";
  } catch (const std::exception& err) {
    return std::string("Failed to capture JS stack: ") + err.what();
  } catch (...) {
    return "Failed to capture JS stack: unknown error.";
  }
}

std::string buildCppCrashJSStackRecord(
    std::string_view stack,
    const std::optional<ExceptionStackHandler::Context>& context) {
  std::string record;
  record.reserve(stack.size() + 256);

  if (context.has_value()) {
    record.append("react_native_instance_id=");
    record.append(std::to_string(context->rnInstanceId));
    record.push_back('\n');

    if (!context->hspModuleName.empty()) {
      record.append("hsp_module_name=");
      record.append(context->hspModuleName);
      record.push_back('\n');
    }

    if (!context->bundlePath.empty()) {
      record.append("bundle_path=");
      record.append(context->bundlePath);
      record.push_back('\n');
    }

    record.push_back('\n');
  }

  record.append(stack.data(), stack.size());
  if (!record.empty() && record.back() != '\n') {
    record.push_back('\n');
  }

  return record;
}

void setGlobalFunctionIfMissing(
    facebook::jsi::Runtime& rt,
    const char* functionName,
    facebook::jsi::Function function) {
  auto globalObject = rt.global();
  if (globalObject.hasProperty(rt, functionName)) {
    return;
  }

  globalObject.setProperty(rt, functionName, std::move(function));
}

} // namespace

void ExceptionStackHandler::configureCppCrashAppLogMergeOnce() {
  std::call_once(g_cppCrashAppLogMergeConfigOnce, []() {
#ifdef OH_APP_CRASH_PARAM_MERGE_CPPCRASH_APP_LOG
    if (!IsAtLeastApi15()) {
      return;
    }
    auto* config = OH_HiAppEvent_CreateConfig();
    if (config == nullptr) {
      LOG(WARNING)
          << "[CrashStack] Failed to create HiAppEvent config for APP_CRASH merge";
      return;
    }

    const auto setItemResult = OH_HiAppEvent_SetConfigItem(
        config, OH_APP_CRASH_PARAM_MERGE_CPPCRASH_APP_LOG, "true");
    if (setItemResult != HIAPPEVENT_SUCCESS) {
      LOG(WARNING)
          << "[CrashStack] Failed to set APP_CRASH merge config item, ret="
          << setItemResult;
      OH_HiAppEvent_DestroyConfig(config);
      return;
    }

    const auto setEventConfigResult =
        OH_HiAppEvent_SetEventConfig(EVENT_APP_CRASH, config);
    if (setEventConfigResult != HIAPPEVENT_SUCCESS) {
      LOG(WARNING)
          << "[CrashStack] Failed to apply APP_CRASH merge config, ret="
          << setEventConfigResult;
    }

    OH_HiAppEvent_DestroyConfig(config);
#endif
  });
}

void ExceptionStackHandler::installCppCrashStackBindings(
    facebook::jsi::Runtime& rt,
    ContextProvider contextProvider) {
  initializeCachedJSStackFilePath();
  installCrashSignalHandlersOnce();

  auto captureJSStack = facebook::jsi::Function::createFromHostFunction(
      rt,
      facebook::jsi::PropNameID::forAscii(rt, CAPTURE_JS_STACK_FUNCTION_NAME),
      1,
      [contextProvider = std::move(contextProvider)](
          facebook::jsi::Runtime& rt,
          const facebook::jsi::Value&,
          const facebook::jsi::Value* args,
          size_t count) -> facebook::jsi::Value {
        auto stack = getStackArgument(rt, args, count, 0);
        if (stack.empty()) {
          stack = captureCurrentCppCrashJSStack(rt);
        }

        const auto context = contextProvider ? contextProvider() : std::nullopt;
        auto record = buildCppCrashJSStackRecord(stack, context);
        storeProcessWideCppCrashJSStack(record);
        return facebook::jsi::Value::undefined();
      });

  setGlobalFunctionIfMissing(
      rt, CAPTURE_JS_STACK_FUNCTION_NAME, std::move(captureJSStack));
}

} // namespace rnoh
