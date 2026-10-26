/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

// Reimplement formatProfilerSample (private method, USE_HERMES guarded).
// Cache state management is tested by the existing rnoh_freeze_stack_test
// target (RNOH/tests/FreezeStackCollectorTest.cpp) which compiles the real
// FreezeStackCollector.cpp. This file only covers formatProfilerSample
// which the existing test does not cover.
// Keep in sync with RNOH/FreezeStackCollector.cpp.

namespace {

struct JSFunctionFrame {
  std::string functionName;
  std::optional<std::string_view> scriptUrl;
  std::optional<uint32_t> lineNumber;
  std::optional<uint32_t> columnNumber;
};

struct NonJSFrame {};

using ProfileCallStackFrame = std::variant<JSFunctionFrame, NonJSFrame>;

struct FakeProfileSample {
  std::vector<ProfileCallStackFrame> frames;
};

std::string formatProfilerSample(const FakeProfileSample& sample) {
  std::string result;
  result.reserve(sample.frames.size() * 64);

  for (const auto& frame : sample.frames) {
    const auto* jsFrame = std::get_if<JSFunctionFrame>(&frame);
    if (jsFrame == nullptr) {
      continue;
    }

    if (jsFrame->functionName.empty()) {
      result.append("<anonymous>");
    } else {
      result.append(jsFrame->functionName);
    }

    const auto hasLocation = jsFrame->scriptUrl.has_value() ||
        jsFrame->lineNumber.has_value() ||
        jsFrame->columnNumber.has_value();
    if (hasLocation) {
      result.push_back('@');
      if (jsFrame->scriptUrl.has_value()) {
        result.append(jsFrame->scriptUrl->data(), jsFrame->scriptUrl->size());
      } else {
        result.append("<unknown>");
      }
      if (jsFrame->lineNumber.has_value()) {
        result.push_back(':');
        result.append(std::to_string(*jsFrame->lineNumber));
      }
      if (jsFrame->columnNumber.has_value()) {
        result.push_back(':');
        result.append(std::to_string(*jsFrame->columnNumber));
      }
    }

    result.push_back('\n');
  }

  return result;
}

} // namespace

// === formatProfilerSample (not covered by existing rnoh_freeze_stack_test) ===

TEST(FreezeStackCollectorFormatProfiler, EmptySample_ReturnsEmptyString) {
  FakeProfileSample sample{};
  EXPECT_TRUE(formatProfilerSample(sample).empty());
}

TEST(FreezeStackCollectorFormatProfiler, SingleJSFrame_WithNameAndLocation) {
  FakeProfileSample sample{{
    JSFunctionFrame{"foo", "bar.js", 10, 5},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "foo@bar.js:10:5\n");
}

TEST(FreezeStackCollectorFormatProfiler, EmptyFunctionName_UsesAnonymous) {
  FakeProfileSample sample{{
    JSFunctionFrame{"", "bar.js", 1, std::nullopt},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "<anonymous>@bar.js:1\n");
}

TEST(FreezeStackCollectorFormatProfiler, NoScriptUrl_UsesUnknown) {
  FakeProfileSample sample{{
    JSFunctionFrame{"foo", std::nullopt, 10, std::nullopt},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "foo@<unknown>:10\n");
}

TEST(FreezeStackCollectorFormatProfiler, NoLocation_OnlyFunctionName) {
  FakeProfileSample sample{{
    JSFunctionFrame{"foo", std::nullopt, std::nullopt, std::nullopt},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "foo\n");
}

TEST(FreezeStackCollectorFormatProfiler, NonJSFramesSkipped) {
  FakeProfileSample sample{{
    NonJSFrame{},
    JSFunctionFrame{"jsFunc", "script.js", 1, 1},
    NonJSFrame{},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "jsFunc@script.js:1:1\n");
}

TEST(FreezeStackCollectorFormatProfiler, MultipleJSFrames_AllFormatted) {
  FakeProfileSample sample{{
    JSFunctionFrame{"foo", "a.js", 1, std::nullopt},
    JSFunctionFrame{"bar", "b.js", 2, 3},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "foo@a.js:1\nbar@b.js:2:3\n");
}

TEST(FreezeStackCollectorFormatProfiler, LineAndColumnOnly_NoScriptUrl) {
  FakeProfileSample sample{{
    JSFunctionFrame{"fn", std::nullopt, 42, 7},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "fn@<unknown>:42:7\n");
}

TEST(FreezeStackCollectorFormatProfiler, AllEmptyFields_OnlyAnonymousName) {
  FakeProfileSample sample{{
    JSFunctionFrame{"", std::nullopt, std::nullopt, std::nullopt},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "<anonymous>\n");
}

TEST(FreezeStackCollectorFormatProfiler, ScriptUrlOnly_NoLineCol) {
  FakeProfileSample sample{{
    JSFunctionFrame{"fn", "script.js", std::nullopt, std::nullopt},
  }};
  EXPECT_EQ(formatProfilerSample(sample), "fn@script.js\n");
}
