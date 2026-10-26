#include "TurboModule.h"

namespace rnoh {
/**
 * Define the virtual destructor out-of-line to emit a strong vtable symbol.
 * Required for RTTI and dynamic_cast to work correctly across shared libs
 * when using LLVM (Clang). See: https://github.com/android/ndk/issues/533
 */
TurboModule::~TurboModule() = default;
} // namespace rnoh