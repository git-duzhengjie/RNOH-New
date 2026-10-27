#pragma once

/**
 * RNOH: HarmonyOS libc++ 兼容补丁
 *
 * RN 0.88 的 ReactCommon/jsinspector-modern 多处使用 std::ranges::empty()，
 * 而 HarmonyOS SDK 自带的 libc++ 尚未实现该函数
 * （std::ranges::range / begin / end 等均可用）。
 *
 * 这里补一个等价实现：
 *   ranges::empty(r)  <=>  ranges::begin(r) == ranges::end(r)
 *
 * 通过 CMake 的 -include 选项注入，避免修改 third-party 中的 vendor 源码。
 */

#include <iterator>
#include <ranges>

#ifndef RNOH_RANGES_EMPTY_COMPAT
#define RNOH_RANGES_EMPTY_COMPAT

namespace std {
namespace ranges {

template <class _Range>
  requires std::ranges::range<_Range>
constexpr bool empty(_Range &&__range) noexcept(
    noexcept(std::ranges::begin(__range) == std::ranges::end(__range))) {
  return std::ranges::begin(__range) == std::ranges::end(__range);
}

} // namespace ranges
} // namespace std

#endif // RNOH_RANGES_EMPTY_COMPAT
