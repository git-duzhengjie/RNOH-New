/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Stub aliasing `google::LogMessage::LogMessage(char const*, int,
 * google::LogSeverity)` to the `int`-typed constructor that the
 * Homebrew-installed glog 0.6.0 lib actually exports.
 *
 * Background: glog 0.6.0's logging.h declares
 *   LogMessage(const char* file, int line, LogSeverity severity);
 * which (per the Itanium C++ ABI) should mangle as
 *   _ZN6google10LogMessageC1EPKciNS_11LogSeverityE
 * (and a parallel C2 base constructor). The Homebrew glog 0.6.0 bottle
 * was built with a toolchain that mangled `LogSeverity` as its underlying
 * type (`int`), producing
 *   _ZN6google10LogMessageC1EPKcii
 * instead. RNOH code compiled against the matching 0.6.0 headers emits
 * calls to the enum-typed symbol, which the bottle does not export,
 * leading to a link error.
 *
 * The fix here uses inline assembly (`.set` directive) to alias the
 * enum-typed symbol to the int-typed symbol. Since `LogSeverity` is an
 * unscoped enum, the calling convention is identical for `int` and
 * `LogSeverity`, so the alias is sound.
 */

asm(".text\n"
    ".globl __ZN6google10LogMessageC1EPKciNS_11LogSeverityE\n"
    ".set   __ZN6google10LogMessageC1EPKciNS_11LogSeverityE, "
    "__ZN6google10LogMessageC1EPKcii\n"
    ".globl __ZN6google10LogMessageC2EPKciNS_11LogSeverityE\n"
    ".set   __ZN6google10LogMessageC2EPKciNS_11LogSeverityE, "
    "__ZN6google10LogMessageC2EPKcii\n");
