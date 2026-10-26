/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Host stubs for two folly helpers referenced by folly/Executor.cpp +
// folly/ScopeGuard.cpp. folly/lang/Exception.cpp does not compile on MinGW
// (it pulls the MSVC-only <ehdata.h>), so provide no-op definitions in an
// isolated TU that does NOT include folly headers (avoids redeclaration
// conflicts). These are never invoked at runtime by the methodMap_ tests.

#include <exception>

namespace folly {

std::exception_ptr current_exception() noexcept {
  return std::exception_ptr();
}

// Returns a non-abi-tagged type so the mangled symbol matches folly's real
// declaration (which returns folly::fbstring, no libstdc++ cxx11 tag). Return
// type is not part of Itanium mangling except for abi tags. Never invoked at
// runtime by the methodMap_ tests.
const char* exceptionStr(const std::exception_ptr&) {
  return nullptr;
}

} // namespace folly
