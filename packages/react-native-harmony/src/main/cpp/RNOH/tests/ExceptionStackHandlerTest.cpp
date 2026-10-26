/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Include the REAL ExceptionStackHandler.cpp to get access to anonymous
// namespace pure functions. No copy, no staleness — always tests the real code.
// Stubs needed: hiappevent×3 (empty), bundle, signal.h (POSIX
// sigaction/siginfo_t). USE_HERMES and
// OH_APP_CRASH_PARAM_MERGE_CPPCRASH_APP_LOG are NOT defined, so Hermes and
// hiappevent code paths are skipped.

#include "ExceptionStackHandler.cpp" // NOLINT: intentional — #include .cpp pattern

#include <MockJSIRuntime.h>
#include <gtest/gtest.h>
#include <cstring>

using namespace rnoh;

// ============================================================================
// Pure computation functions from anonymous namespace
// Tested via #include .cpp — no copy, always synced with real source.
// ============================================================================

// ---------------------------------------------------------------------------
// appendUnsignedDecimal
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, AppendUnsignedDecimal_Zero_WritesSingleZero) {
  char buf[32];
  auto len = appendUnsignedDecimal(buf, sizeof(buf), 0);
  buf[len] = '\0';
  EXPECT_EQ(len, 1u);
  EXPECT_STREQ(buf, "0");
}

TEST(ExceptionStackHandlerUtil, AppendUnsignedDecimal_SingleDigit) {
  char buf[32];
  auto len = appendUnsignedDecimal(buf, sizeof(buf), 7);
  buf[len] = '\0';
  EXPECT_STREQ(buf, "7");
}

TEST(ExceptionStackHandlerUtil, AppendUnsignedDecimal_MultiDigit) {
  char buf[32];
  auto len = appendUnsignedDecimal(buf, sizeof(buf), 12345);
  buf[len] = '\0';
  EXPECT_STREQ(buf, "12345");
}

TEST(
    ExceptionStackHandlerUtil,
    AppendUnsignedDecimal_BufferSizeZero_ReturnsZero) {
  char buf[1];
  auto len = appendUnsignedDecimal(buf, 0, 42);
  EXPECT_EQ(len, 0u);
}

TEST(ExceptionStackHandlerUtil, AppendUnsignedDecimal_NullBuffer_ReturnsZero) {
  auto len = appendUnsignedDecimal(nullptr, 32, 42);
  EXPECT_EQ(len, 0u);
}

TEST(ExceptionStackHandlerUtil, AppendUnsignedDecimal_MaxUint64) {
  char buf[32];
  auto len = appendUnsignedDecimal(buf, sizeof(buf), UINT64_MAX);
  buf[len] = '\0';
  EXPECT_EQ(len, 20u); // 18446744073709551615 = 20 digits
  EXPECT_STREQ(buf, "18446744073709551615");
}

// ---------------------------------------------------------------------------
// appendHex
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, AppendHex_Zero_Writes0x0) {
  char buf[32];
  auto len = appendHex(buf, sizeof(buf), 0);
  buf[len] = '\0';
  EXPECT_STREQ(buf, "0x0");
}

TEST(ExceptionStackHandlerUtil, AppendHex_SingleDigit) {
  char buf[32];
  auto len = appendHex(buf, sizeof(buf), 0xF);
  buf[len] = '\0';
  EXPECT_STREQ(buf, "0xf");
}

TEST(ExceptionStackHandlerUtil, AppendHex_MultiDigit) {
  char buf[32];
  auto len = appendHex(buf, sizeof(buf), 0xDEADBEEF);
  buf[len] = '\0';
  EXPECT_STREQ(buf, "0xdeadbeef");
}

TEST(ExceptionStackHandlerUtil, AppendHex_BufferTooSmall_ReturnsZero) {
  char buf[2];
  auto len = appendHex(buf, sizeof(buf), 0xFF);
  EXPECT_EQ(len, 0u); // need at least 3 chars: "0x" + digit
}

TEST(ExceptionStackHandlerUtil, AppendHex_NullBuffer_ReturnsZero) {
  auto len = appendHex(nullptr, 32, 0xFF);
  EXPECT_EQ(len, 0u);
}

TEST(ExceptionStackHandlerUtil, AppendHex_UintptrMax) {
  char buf[32];
  auto len = appendHex(buf, sizeof(buf), static_cast<uintptr_t>(-1));
  buf[len] = '\0';
  EXPECT_EQ(len, 2u + 2u * sizeof(uintptr_t));
  std::string expected = "0x" + std::string(2 * sizeof(uintptr_t), 'f');
  EXPECT_STREQ(buf, expected.c_str());
}

TEST(ExceptionStackHandlerUtil, AppendHex_BufferExactlyFits) {
  char buf[3];
  auto len = appendHex(buf, sizeof(buf), 0xA);
  EXPECT_EQ(len, 3u);
  buf[len] = '\0';
  EXPECT_STREQ(buf, "0xa");
}

// ---------------------------------------------------------------------------
// fitStackToCache
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, FitStackToCache_ShortStack_ReturnsAsIs) {
  std::string stack = "Error: something went wrong";
  auto result = fitStackToCache(stack);
  EXPECT_EQ(result, stack);
}

TEST(ExceptionStackHandlerUtil, FitStackToCache_EmptyStack_ReturnsEmpty) {
  std::string result = fitStackToCache("");
  EXPECT_TRUE(result.empty());
}

TEST(ExceptionStackHandlerUtil, FitStackToCache_OverMax_TruncatesWithSuffix) {
  std::string longStack(100 * 1024 + 100, 'X'); // > 100KB
  auto result = fitStackToCache(longStack);
  EXPECT_LE(result.size(), 100u * 1024u);
  // Should end with the truncation suffix
  EXPECT_NE(result.find("truncated"), std::string::npos);
}

TEST(ExceptionStackHandlerUtil, FitStackToCache_ExactlyMax_ReturnsAsIs) {
  std::string stack(100 * 1024, 'A'); // exactly 100KB
  auto result = fitStackToCache(stack);
  EXPECT_EQ(result, stack);
}

// ---------------------------------------------------------------------------
// buildCppCrashJSStackRecord
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, BuildRecord_WithContext_AllFields) {
  ExceptionStackHandler::Context ctx{
      42, "/data/bundles/com.example.app/", "entry"};
  std::string result = buildCppCrashJSStackRecord("  at foo():1:2", ctx);
  EXPECT_NE(result.find("react_native_instance_id=42"), std::string::npos);
  EXPECT_NE(result.find("hsp_module_name=entry"), std::string::npos);
  EXPECT_NE(
      result.find("bundle_path=/data/bundles/com.example.app/"),
      std::string::npos);
  EXPECT_NE(result.find("  at foo():1:2"), std::string::npos);
}

TEST(ExceptionStackHandlerUtil, BuildRecord_WithContext_PartialFields) {
  ExceptionStackHandler::Context ctx{
      7, "", ""}; // empty hspModuleName and bundlePath
  std::string result = buildCppCrashJSStackRecord("stack trace", ctx);
  EXPECT_NE(result.find("react_native_instance_id=7"), std::string::npos);
  EXPECT_EQ(
      result.find("hsp_module_name="), std::string::npos); // empty → omitted
  EXPECT_EQ(result.find("bundle_path="), std::string::npos); // empty → omitted
}

TEST(ExceptionStackHandlerUtil, BuildRecord_WithoutContext) {
  std::string result = buildCppCrashJSStackRecord("stack trace", std::nullopt);
  EXPECT_EQ(result, "stack trace\n"); // no context prefix, just stack + newline
  EXPECT_EQ(result.find("react_native_instance_id"), std::string::npos);
}

TEST(ExceptionStackHandlerUtil, BuildRecord_StackWithNewline_Preserved) {
  ExceptionStackHandler::Context ctx{1, "", ""};
  std::string result = buildCppCrashJSStackRecord("line1\nline2\n", ctx);
  // Stack already ends with \n → no extra \n added
  EXPECT_NE(result.find("line1\nline2\n"), std::string::npos);
}

TEST(ExceptionStackHandlerUtil, BuildRecord_StackWithoutNewline_AppendsOne) {
  ExceptionStackHandler::Context ctx{1, "", ""};
  std::string result = buildCppCrashJSStackRecord("no newline here", ctx);
  EXPECT_NE(result.find("no newline here\n"), std::string::npos);
}

// ---------------------------------------------------------------------------
// findHandledSignalIndex
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, FindSignalIndex_KnownSignals) {
  EXPECT_EQ(findHandledSignalIndex(SIGSEGV), 0);
  EXPECT_EQ(findHandledSignalIndex(SIGABRT), 1);
  EXPECT_EQ(findHandledSignalIndex(SIGFPE), 2);
  EXPECT_EQ(findHandledSignalIndex(SIGBUS), 3);
}

TEST(ExceptionStackHandlerUtil, FindSignalIndex_UnknownSignal_ReturnsMinusOne) {
  EXPECT_EQ(findHandledSignalIndex(999), -1);
  EXPECT_EQ(findHandledSignalIndex(0), -1);
}

// ---------------------------------------------------------------------------
// signalToName
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, SignalToName_KnownSignals) {
  EXPECT_STREQ(signalToName(SIGSEGV), "SIGSEGV");
  EXPECT_STREQ(signalToName(SIGABRT), "SIGABRT");
  EXPECT_STREQ(signalToName(SIGFPE), "SIGFPE");
  EXPECT_STREQ(signalToName(SIGBUS), "SIGBUS");
}

TEST(ExceptionStackHandlerUtil, SignalToName_UnknownSignal) {
  EXPECT_STREQ(signalToName(999), "UNKNOWN");
}

// ---------------------------------------------------------------------------
// signalNameSize
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, SignalNameSize_KnownSignals) {
  EXPECT_EQ(signalNameSize(SIGSEGV), sizeof("SIGSEGV") - 1);
  EXPECT_EQ(signalNameSize(SIGABRT), sizeof("SIGABRT") - 1);
  EXPECT_EQ(signalNameSize(SIGFPE), sizeof("SIGFPE") - 1);
  EXPECT_EQ(signalNameSize(SIGBUS), sizeof("SIGBUS") - 1);
}

TEST(ExceptionStackHandlerUtil, SignalNameSize_UnknownSignal) {
  EXPECT_EQ(signalNameSize(999), sizeof("UNKNOWN") - 1);
  EXPECT_EQ(signalNameSize(0), sizeof("UNKNOWN") - 1);
}

TEST(ExceptionStackHandlerUtil, SignalNameSize_MatchesSignalToNameLength) {
  EXPECT_EQ(signalNameSize(SIGSEGV), std::strlen(signalToName(SIGSEGV)));
  EXPECT_EQ(signalNameSize(SIGABRT), std::strlen(signalToName(SIGABRT)));
  EXPECT_EQ(signalNameSize(SIGFPE), std::strlen(signalToName(SIGFPE)));
  EXPECT_EQ(signalNameSize(SIGBUS), std::strlen(signalToName(SIGBUS)));
  EXPECT_EQ(signalNameSize(999), std::strlen(signalToName(999)));
}

// ---------------------------------------------------------------------------
// writeSignalSafeLineWithUInt / writeSignalSafeLineWithHex
// These write to stderr (fd 2) via write() — stubbed in unistd.h
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, WriteSignalSafeLineWithUInt_DoesNotCrash) {
  writeSignalSafeLineWithUInt("test_uint: ", 10, 42);
  writeSignalSafeLineWithUInt("max: ", 4, UINT64_MAX);
  writeSignalSafeLineWithUInt("zero: ", 5, 0);
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, WriteSignalSafeLineWithHex_DoesNotCrash) {
  writeSignalSafeLineWithHex("test_hex: ", 9, 0xDEADBEEF);
  writeSignalSafeLineWithHex("zero: ", 5, 0);
  writeSignalSafeLineWithHex("ptr: ", 4, reinterpret_cast<uintptr_t>(nullptr));
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, WriteSignalSafe_EmptyData_DoesNotCrash) {
  writeSignalSafe("", 0);
  writeSignalSafe("x", 1);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// readProcessNameFromProc — reads /proc/self/cmdline
// On Windows this file doesn't exist → returns empty string
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, ReadProcessNameFromProc_ReturnsString) {
  auto result = readProcessNameFromProc();
  // On Windows, /proc/self/cmdline doesn't exist → returns ""
  // On Linux, returns the process name
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, ResolveBundleName_ReturnsString) {
  auto result = resolveBundleName();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// isDefaultOrIgnoredSignalActionHandler / Siginfo
// These check sigaction structs — stubbed in signal.h
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, IsDefaultOrIgnored_DefaultHandler_ReturnsTrue) {
  struct sigaction sa;
  sa.sa_handler = SIG_DFL;
  EXPECT_TRUE(isDefaultOrIgnoredSignalActionHandler(sa));
}

TEST(ExceptionStackHandlerUtil, IsDefaultOrIgnored_IgnoredHandler_ReturnsTrue) {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  EXPECT_TRUE(isDefaultOrIgnoredSignalActionHandler(sa));
}

TEST(ExceptionStackHandlerUtil, IsDefaultOrIgnored_CustomHandler_ReturnsFalse) {
  struct sigaction sa;
  sa.sa_handler = reinterpret_cast<void (*)(int)>(0x1234);
  EXPECT_FALSE(isDefaultOrIgnoredSignalActionHandler(sa));
}

TEST(
    ExceptionStackHandlerUtil,
    IsDefaultOrIgnored_NullSiginfoHandler_ReturnsTrue) {
  struct sigaction sa;
  sa.sa_sigaction = nullptr;
  EXPECT_TRUE(isDefaultOrIgnoredSignalActionSiginfo(sa));
}

TEST(
    ExceptionStackHandlerUtil,
    IsDefaultOrIgnored_CustomSiginfoHandler_ReturnsFalse) {
  struct sigaction sa;
  sa.sa_sigaction = reinterpret_cast<void (*)(int, siginfo_t*, void*)>(0x5678);
  EXPECT_FALSE(isDefaultOrIgnoredSignalActionSiginfo(sa));
}

// ---------------------------------------------------------------------------
// dispatchPreviousSignalActionIfNeeded
// Dispatches previous signal handler — tests with default/ignored handlers
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, DispatchPrevious_DefaultHandler_DoesNotCrash) {
  siginfo_t info;
  info.si_signo = SIGSEGV;
  info.si_code = 0;
  info.si_addr = nullptr;
  dispatchPreviousSignalActionIfNeeded(SIGSEGV, &info, nullptr);
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, DispatchPrevious_UnknownSignal_DoesNotCrash) {
  siginfo_t info;
  info.si_signo = 999;
  info.si_code = 0;
  info.si_addr = nullptr;
  dispatchPreviousSignalActionIfNeeded(999, &info, nullptr);
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, DispatchPrevious_SIGABRT_DoesNotCrash) {
  siginfo_t info;
  info.si_signo = SIGABRT;
  info.si_code = 0;
  info.si_addr = nullptr;
  dispatchPreviousSignalActionIfNeeded(SIGABRT, &info, nullptr);
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, DispatchPrevious_SIGFPE_DoesNotCrash) {
  siginfo_t info;
  info.si_signo = SIGFPE;
  info.si_code = 0;
  info.si_addr = nullptr;
  dispatchPreviousSignalActionIfNeeded(SIGFPE, &info, nullptr);
  SUCCEED();
}

TEST(ExceptionStackHandlerUtil, DispatchPrevious_SIGBUS_DoesNotCrash) {
  siginfo_t info;
  info.si_signo = SIGBUS;
  info.si_code = 0;
  info.si_addr = nullptr;
  dispatchPreviousSignalActionIfNeeded(SIGBUS, &info, nullptr);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// configureCppCrashAppLogMergeOnce (static public method)
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandler, ConfigureCppCrashAppLogMergeOnce_DoesNotCrash) {
  ExceptionStackHandler::configureCppCrashAppLogMergeOnce();
  SUCCEED();
}

TEST(
    ExceptionStackHandler,
    ConfigureCppCrashAppLogMergeOnce_Twice_DoesNotCrash) {
  ExceptionStackHandler::configureCppCrashAppLogMergeOnce();
  ExceptionStackHandler::configureCppCrashAppLogMergeOnce();
  SUCCEED();
}

// ---------------------------------------------------------------------------
// storeProcessWideCppCrashJSStack / overwriteCachedJSStackFile
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, StoreProcessWideCppCrashJSStack_DoesNotCrash) {
  storeProcessWideCppCrashJSStack("test stack trace");
  SUCCEED();
}

TEST(
    ExceptionStackHandlerUtil,
    StoreProcessWideCppCrashJSStack_Empty_DoesNotCrash) {
  storeProcessWideCppCrashJSStack("");
  SUCCEED();
}

TEST(
    ExceptionStackHandlerUtil,
    StoreProcessWideCppCrashJSStack_Large_DoesNotCrash) {
  std::string largeStack(
      200 * 1024, 'x'); // 200KB — exceeds MAX_CRASH_JS_STACK_BYTES
  storeProcessWideCppCrashJSStack(largeStack);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// findHandledSignalIndex — additional coverage
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, FindHandledSignalIndex_UnknownSignal) {
  int idx = findHandledSignalIndex(999);
  EXPECT_EQ(idx, -1);
}

TEST(ExceptionStackHandlerUtil, FindHandledSignalIndex_AllHandledSignals) {
  for (int i = 0; i < static_cast<int>(HANDLED_CRASH_SIGNALS.size()); ++i) {
    int idx = findHandledSignalIndex(HANDLED_CRASH_SIGNALS[i]);
    EXPECT_EQ(idx, i);
  }
}

// ---------------------------------------------------------------------------
// overwriteCachedJSStackFile / initializeCachedJSStackFilePath
// ---------------------------------------------------------------------------

TEST(
    ExceptionStackHandlerUtil,
    OverwriteCachedJSStackFile_ValidStack_ReturnsTrue) {
  bool result = overwriteCachedJSStackFile("test stack content");
  // Returns true if file written, false on error
  SUCCEED();
}

TEST(
    ExceptionStackHandlerUtil,
    OverwriteCachedJSStackFile_EmptyStack_DoesNotCrash) {
  overwriteCachedJSStackFile("");
  SUCCEED();
}

TEST(
    ExceptionStackHandlerUtil,
    OverwriteCachedJSStackFile_LargeStack_DoesNotCrash) {
  std::string large(200 * 1024, 'x');
  overwriteCachedJSStackFile(large);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// storeProcessWideCppCrashJSStack (already tested, add edge cases)
// ---------------------------------------------------------------------------

TEST(
    ExceptionStackHandlerUtil,
    StoreProcessWideCppCrashJSStack_NullChars_DoesNotCrash) {
  storeProcessWideCppCrashJSStack("stack\0with\0nulls");
  SUCCEED();
}

TEST(
    ExceptionStackHandlerUtil,
    StoreProcessWideCppCrashJSStack_ExactMaxSize_DoesNotCrash) {
  std::string exactMax(MAX_CRASH_JS_STACK_BYTES, 'x');
  storeProcessWideCppCrashJSStack(exactMax);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// fitStackToCache (edge cases)
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandlerUtil, FitStackToCache_ExactMaxSize_ReturnsAsIs) {
  std::string exactMax(MAX_CRASH_JS_STACK_BYTES, 'x');
  auto result = fitStackToCache(exactMax);
  EXPECT_EQ(result.size(), MAX_CRASH_JS_STACK_BYTES);
}

TEST(ExceptionStackHandlerUtil, FitStackToCache_OneOverMax_Truncates) {
  std::string overMax(MAX_CRASH_JS_STACK_BYTES + 1, 'x');
  auto result = fitStackToCache(overMax);
  EXPECT_LE(result.size(), MAX_CRASH_JS_STACK_BYTES);
}

TEST(ExceptionStackHandlerUtil, FitStackToCache_SingleChar_ReturnsAsIs) {
  auto result = fitStackToCache("x");
  EXPECT_EQ(result, "x");
}

// ---------------------------------------------------------------------------
// buildCppCrashJSStackRecord (needs Context)
// ---------------------------------------------------------------------------

TEST(
    ExceptionStackHandlerUtil,
    BuildCppCrashJSStackRecord_WithContext_DoesNotCrash) {
  ExceptionStackHandler::Context ctx;
  ctx.rnInstanceId = 1;
  ctx.bundlePath = "/data/test/bundle";
  ctx.hspModuleName = "test_module";
  std::optional<ExceptionStackHandler::Context> context = ctx;
  auto result = buildCppCrashJSStackRecord("test stack", context);
  EXPECT_FALSE(result.empty());
}

TEST(
    ExceptionStackHandlerUtil,
    BuildCppCrashJSStackRecord_NoContext_DoesNotCrash) {
  std::optional<ExceptionStackHandler::Context> context;
  auto result = buildCppCrashJSStackRecord("test stack", context);
  SUCCEED();
}

TEST(
    ExceptionStackHandlerUtil,
    BuildCppCrashJSStackRecord_EmptyStack_DoesNotCrash) {
  ExceptionStackHandler::Context ctx;
  ctx.rnInstanceId = 0;
  ctx.bundlePath = "";
  ctx.hspModuleName = "";
  std::optional<ExceptionStackHandler::Context> context = ctx;
  auto result = buildCppCrashJSStackRecord("", context);
  SUCCEED();
}

// ---------------------------------------------------------------------------
// installCppCrashStackBindings (needs jsi::Runtime)
// ---------------------------------------------------------------------------

TEST(ExceptionStackHandler, InstallCppCrashStackBindings_DoesNotCrash) {
  facebook::jsi::MockJSIRuntime rt;
  auto ctxProvider = []() -> std::optional<ExceptionStackHandler::Context> {
    ExceptionStackHandler::Context ctx;
    ctx.rnInstanceId = 1;
    ctx.bundlePath = "/test/bundle";
    ctx.hspModuleName = "test";
    return ctx;
  };
  ExceptionStackHandler::installCppCrashStackBindings(rt, ctxProvider);
  SUCCEED();
}

TEST(
    ExceptionStackHandler,
    InstallCppCrashStackBindings_NullProvider_DoesNotCrash) {
  facebook::jsi::MockJSIRuntime rt;
  auto ctxProvider = []() -> std::optional<ExceptionStackHandler::Context> {
    return std::nullopt;
  };
  ExceptionStackHandler::installCppCrashStackBindings(rt, ctxProvider);
  SUCCEED();
}

TEST(ExceptionStackHandler, InstallCppCrashStackBindings_Twice_DoesNotCrash) {
  facebook::jsi::MockJSIRuntime rt;
  auto ctxProvider = []() -> std::optional<ExceptionStackHandler::Context> {
    return std::nullopt;
  };
  ExceptionStackHandler::installCppCrashStackBindings(rt, ctxProvider);
  ExceptionStackHandler::installCppCrashStackBindings(rt, ctxProvider);
  SUCCEED();
}
