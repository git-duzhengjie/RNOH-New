/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unit tests for the page-binding-id parsing logic in
 * `ModalHostViewComponentInstance::updateBindingFromProps`.
 *
 * ZERO PRODUCTION CODE MODIFICATION: the parsing logic is copied
 * verbatim from `ModalHostViewComponentInstance.cpp` as
 * `parsePageBindingTargetIdCopy` instead of extracting it to a
 * production header. Per `gtest-guide.md` §1.5, this inline copy is
 * the last-resort isolation strategy (header stubbing + .cpp copy
 * both infeasible here because the production .cpp transitively
 * pulls folly::dynamic + RNOH/ComponentInstance). The copied function
 * must be re-synced whenever `ModalHostViewComponentInstance.cpp`
 * changes — its comment header marks the verbatim-copy relationship.
 *
 * The copied function is a pure-logic extraction of the string
 * parsing branch (nativeId → newTargetId), without the dialog-handler
 * side effects. Tests cover every branch:
 *
 *   1. Empty nativeId → -1 (early return)
 *   2. Non-prefixed nativeId → -1 (prefix mismatch)
 *   3. Prefix only, no id → -1 ("Empty page binding id")
 *   4. Prefix + non-numeric id → -1 ("Failed to parse")
 *   5. Prefix + negative id → -1 ("Negative page binding id")
 *   6. Prefix + id with trailing chars → -1 ("Unexpected trailing chars")
 *   7. Prefix + valid zero → 0
 *   8. Prefix + valid positive id → id
 *   9. Prefix + valid large id (INT32_MAX) → id
 *  10. Prefix + overflow id (beyond INT32_MAX) → -1 (stoi out_of_range)
 *  11. Prefix + leading whitespace → valid (stoi skips whitespace)
 *  12. Prefix + leading + sign → valid (stoi accepts "+42")
 *  13. Prefix + leading zero → valid (stoi parses "007" → 7)
 *  14. nativeId with prefix substring mid-string → -1 (rfind checks pos 0)
 *  15. Prefix + valid id with trailing newline → -1 (pos != size)
 *  16. Hex string "0xFF" → -1 (stoi base 10 stops at "x")
 */

#include <cstdint>
#include <stdexcept>
#include <string>

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace {

// VERBATIM COPY (copied verbatim from) of the prefix constant from
// `ModalHostViewComponentInstance.cpp`. Re-sync if the production
// constant changes.
constexpr const char* kPageBoundPrefixCopy = "__harmony::page-bound:";

/**
 * VERBATIM COPY (copied verbatim from) of the pure string-parsing
 * branch of `ModalHostViewComponentInstance::updateBindingFromProps`.
 * Returns the parsed page-binding target id (>= 0) or -1.
 *
 * The ONLY difference from the production code: the dialog-handler
 * side effects (`m_dialogHandler.setPageBindingTarget` /
 * `clearPageBindingTarget` / `getPageBindingTarget`) are omitted —
 * this function only parses.
 *
 * Re-sync from ModalHostViewComponentInstance.cpp if that file changes.
 */
int32_t parsePageBindingTargetIdCopy(const std::string& nativeId) {
  int32_t newTargetId = -1;

  if (!nativeId.empty()) {
    const std::string prefix = kPageBoundPrefixCopy;
    if (nativeId.rfind(prefix, 0) == 0) {
      const auto idString = nativeId.substr(prefix.size());
      if (!idString.empty()) {
        try {
          size_t pos = 0;
          int32_t parsedId = std::stoi(idString, &pos);
          if (pos == idString.size() && parsedId >= 0) {
            newTargetId = parsedId;
          } else if (pos != idString.size()) {
            LOG(WARNING)
                << "Unexpected trailing characters in native page binding id: "
                << idString << ", parsed: " << parsedId;
          } else {
            LOG(WARNING) << "Negative page binding id is not valid: "
                         << parsedId << ", from nativeID: " << nativeId;
          }
        } catch (const std::exception& e) {
          LOG(WARNING)
              << "Failed to parse native page binding id from nativeID: "
              << nativeId << ", reason: " << e.what();
        }
      } else {
        LOG(WARNING) << "Empty page binding id in nativeID: " << nativeId;
      }
    }
  }

  return newTargetId;
}

std::string withId(std::string_view id) {
  std::string s{kPageBoundPrefixCopy};
  s.append(id.data(), id.size());
  return s;
}

} // namespace

TEST(ModalPageBindingIdTest, EmptyNativeId_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(""), -1);
}

TEST(ModalPageBindingIdTest, PlainNativeId_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy("my-modal-1"), -1);
}

TEST(ModalPageBindingIdTest, EmptyStringNativeId_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(std::string{}), -1);
}

TEST(ModalPageBindingIdTest, PrefixOnlyNoId_ReturnsMinusOne) {
  EXPECT_EQ(
      parsePageBindingTargetIdCopy(std::string{kPageBoundPrefixCopy}), -1);
}

TEST(ModalPageBindingIdTest, NonNumericId_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("abc")), -1);
}

TEST(ModalPageBindingIdTest, NegativeId_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("-1")), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("-42")), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("-2147483648")), -1);
}

TEST(ModalPageBindingIdTest, IdWithTrailingChars_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("42abc")), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("1 2")), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("42 ")), -1);
}

TEST(ModalPageBindingIdTest, IdWithLeadingWhitespace_AcceptedByStoi_ReturnsId) {
  // std::stoi discards leading whitespace, then parses the number — pos
  // ends up equal to idString.size(), so the production code treats this
  // as valid. This is a quirk of std::stoi, not intentional.
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId(" 42")), 42);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("\t42")), 42);
}

TEST(ModalPageBindingIdTest, IdWithNewlineSuffix_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("42\n")), -1);
}

TEST(ModalPageBindingIdTest, ValidZero_ReturnsZero) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("0")), 0);
}

TEST(ModalPageBindingIdTest, ValidPositiveId_ReturnsId) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("1")), 1);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("42")), 42);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("123456")), 123456);
}

TEST(ModalPageBindingIdTest, ValidExplicitPositiveSign_ReturnsId) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("+42")), 42);
}

TEST(ModalPageBindingIdTest, LeadingZerosAreParsedAsDecimal_ReturnsId) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("007")), 7);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("00042")), 42);
}

TEST(ModalPageBindingIdTest, MaxInt32_ReturnsId) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("2147483647")), 2147483647);
}

TEST(ModalPageBindingIdTest, OverflowInt32_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("2147483648")), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("9999999999")), -1);
}

TEST(ModalPageBindingIdTest, PrefixSubstringNotAtStart_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy("modal__harmony::page-bound:42"), -1);
}

TEST(ModalPageBindingIdTest, PrefixWithTrailingCharsAfterId_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy("__harmony::page-bound:42extra"), -1);
}

TEST(ModalPageBindingIdTest, CaseSensitivePrefix_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy("__Harmony::page-bound:42"), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy("__harmony::Page-Bound:42"), -1);
  EXPECT_EQ(parsePageBindingTargetIdCopy("__HARMONY::PAGE-BOUND:42"), -1);
}

TEST(ModalPageBindingIdTest, HexString_ReturnsMinusOne) {
  EXPECT_EQ(parsePageBindingTargetIdCopy(withId("0xFF")), -1);
}

TEST(ModalPageBindingIdTest, PrefixConstantMatchesExpectedValue) {
  EXPECT_STREQ(kPageBoundPrefixCopy, "__harmony::page-bound:");
}
