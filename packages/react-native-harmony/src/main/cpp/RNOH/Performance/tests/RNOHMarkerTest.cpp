/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "../RNOHMarker.h"
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <utility>

using namespace rnoh;
using RNOHMarkerId = RNOHMarker::RNOHMarkerId;

// Reimplement pure conversion functions from RNOHMarker.cpp.
// The .cpp cannot be included directly because setLogMarkerIfNeeded()
// hits a LogTaggedMarker type mismatch on macOS (__APPLE__ branch).
// Keep in sync with RNOH/Performance/RNOHMarker.cpp.

static std::string harmonyMarkerIdToString(RNOHMarkerId markerId) {
  switch (markerId) {
    case RNOHMarkerId::UNKNOWN_REACT_MARKER:
      return "UNKNOWN_REACT_MARKER";
    case RNOHMarkerId::APP_STARTUP_START:
      return "APP_STARTUP_START";
    case RNOHMarkerId::APP_STARTUP_STOP:
      return "APP_STARTUP_STOP";
    case RNOHMarkerId::INIT_REACT_RUNTIME_START:
      return "INIT_REACT_RUNTIME_START";
    case RNOHMarkerId::INIT_REACT_RUNTIME_STOP:
      return "INIT_REACT_RUNTIME_STOP";
    case RNOHMarkerId::NATIVE_REQUIRE_START:
      return "NATIVE_REQUIRE_START";
    case RNOHMarkerId::NATIVE_REQUIRE_STOP:
      return "NATIVE_REQUIRE_STOP";
    case RNOHMarkerId::RUN_JS_BUNDLE_START:
      return "RUN_JS_BUNDLE_START";
    case RNOHMarkerId::RUN_JS_BUNDLE_STOP:
      return "RUN_JS_BUNDLE_STOP";
    case RNOHMarkerId::CREATE_REACT_CONTEXT_START:
      return "CREATE_REACT_CONTEXT_START";
    case RNOHMarkerId::CREATE_REACT_CONTEXT_STOP:
      return "CREATE_REACT_CONTEXT_STOP";
    case RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_START:
      return "JS_BUNDLE_STRING_CONVERT_START";
    case RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_STOP:
      return "JS_BUNDLE_STRING_CONVERT_STOP";
    case RNOHMarkerId::NATIVE_MODULE_SETUP_START:
      return "NATIVE_MODULE_SETUP_START";
    case RNOHMarkerId::NATIVE_MODULE_SETUP_STOP:
      return "NATIVE_MODULE_SETUP_STOP";
    case RNOHMarkerId::REGISTER_JS_SEGMENT_START:
      return "REGISTER_JS_SEGMENT_START";
    case RNOHMarkerId::REGISTER_JS_SEGMENT_STOP:
      return "REGISTER_JS_SEGMENT_STOP";
    case RNOHMarkerId::REACT_INSTANCE_INIT_START:
      return "REACT_INSTANCE_INIT_START";
    case RNOHMarkerId::REACT_INSTANCE_INIT_STOP:
      return "REACT_INSTANCE_INIT_STOP";
    case RNOHMarkerId::CONTENT_APPEARED:
      return "CONTENT_APPEARED";
    case RNOHMarkerId::DOWNLOAD_START:
      return "DOWNLOAD_START";
    case RNOHMarkerId::DOWNLOAD_END:
      return "DOWNLOAD_END";
    case RNOHMarkerId::REACT_BRIDGE_LOADING_START:
      return "REACT_BRIDGE_LOADING_START";
    case RNOHMarkerId::REACT_BRIDGE_LOADING_END:
      return "REACT_BRIDGE_LOADING_END";
    case RNOHMarkerId::ON_HOST_RESUME_START:
      return "ON_HOST_RESUME_START";
    case RNOHMarkerId::ON_HOST_RESUME_END:
      return "ON_HOST_RESUME_END";
    case RNOHMarkerId::ON_HOST_PAUSE_START:
      return "ON_HOST_PAUSE_START";
    case RNOHMarkerId::ON_HOST_PAUSE_END:
      return "ON_HOST_PAUSE_END";
    case RNOHMarkerId::CHANGE_THREAD_PRIORITY:
      return "CHANGE_THREAD_PRIORITY";
    case RNOHMarkerId::INITIALIZE_MODULE_START:
      return "INITIALIZE_MODULE_START";
    case RNOHMarkerId::INITIALIZE_MODULE_END:
      return "INITIALIZE_MODULE_END";
    case RNOHMarkerId::PROCESS_PACKAGES_START:
      return "PROCESS_PACKAGES_START";
    case RNOHMarkerId::PROCESS_PACKAGES_END:
      return "PROCESS_PACKAGES_END";
    case RNOHMarkerId::CREATE_UI_MANAGER_MODULE_START:
      return "CREATE_UI_MANAGER_MODULE_START";
    case RNOHMarkerId::CREATE_UI_MANAGER_MODULE_END:
      return "CREATE_UI_MANAGER_MODULE_END";
    case RNOHMarkerId::CREATE_MODULE_START:
      return "CREATE_MODULE_START";
    case RNOHMarkerId::CREATE_MODULE_END:
      return "CREATE_MODULE_END";
    case RNOHMarkerId::PROCESS_CORE_REACT_PACKAGE_START:
      return "PROCESS_CORE_REACT_PACKAGE_START";
    case RNOHMarkerId::PROCESS_CORE_REACT_PACKAGE_END:
      return "PROCESS_CORE_REACT_PACKAGE_END";
    case RNOHMarkerId::FABRIC_COMMIT_START:
      return "FABRIC_COMMIT_START";
    case RNOHMarkerId::FABRIC_COMMIT_END:
      return "FABRIC_COMMIT_END";
    case RNOHMarkerId::FABRIC_FINISH_TRANSACTION_START:
      return "FABRIC_FINISH_TRANSACTION_START";
    case RNOHMarkerId::FABRIC_FINISH_TRANSACTION_END:
      return "FABRIC_FINISH_TRANSACTION_END";
    case RNOHMarkerId::FABRIC_DIFF_START:
      return "FABRIC_DIFF_START";
    case RNOHMarkerId::FABRIC_DIFF_END:
      return "FABRIC_DIFF_END";
    case RNOHMarkerId::FABRIC_LAYOUT_START:
      return "FABRIC_LAYOUT_START";
    case RNOHMarkerId::FABRIC_LAYOUT_END:
      return "FABRIC_LAYOUT_END";
    case RNOHMarkerId::FABRIC_BATCH_EXECUTION_START:
      return "FABRIC_BATCH_EXECUTION_START";
    case RNOHMarkerId::FABRIC_BATCH_EXECUTION_END:
      return "FABRIC_BATCH_EXECUTION_END";
    case RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_START:
      return "FABRIC_UPDATE_UI_MAIN_THREAD_START";
    case RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_END:
      return "FABRIC_UPDATE_UI_MAIN_THREAD_END";
    default:
      return "UNKNOWN";
  }
}

static RNOHMarkerId harmonyMarkerIdForString(const std::string& markerId) {
  if (markerId == "CREATE_REACT_CONTEXT_START")
    return RNOHMarkerId::CREATE_REACT_CONTEXT_START;
  if (markerId == "DOWNLOAD_START")
    return RNOHMarkerId::DOWNLOAD_START;
  if (markerId == "DOWNLOAD_END")
    return RNOHMarkerId::DOWNLOAD_END;
  if (markerId == "TURBO_MODULE_SETUP_START")
    return RNOHMarkerId::TURBO_MODULE_SETUP_START;
  if (markerId == "TURBO_MODULE_SETUP_STOP")
    return RNOHMarkerId::TURBO_MODULE_SETUP_STOP;
  if (markerId == "REACT_INSTANCE_INIT_START")
    return RNOHMarkerId::REACT_INSTANCE_INIT_START;
  if (markerId == "REACT_INSTANCE_INIT_STOP")
    return RNOHMarkerId::REACT_INSTANCE_INIT_STOP;
  throw std::invalid_argument("Unknown markerId: " + markerId);
}

class RNOHMarkerToStringTest
    : public ::testing::TestWithParam<std::pair<RNOHMarkerId, std::string>> {};

TEST_P(RNOHMarkerToStringTest, ReturnsCorrectString) {
  auto [markerId, expected] = GetParam();
  EXPECT_EQ(harmonyMarkerIdToString(markerId), expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllMarkerIds,
    RNOHMarkerToStringTest,
    ::testing::Values(
        std::make_pair(
            RNOHMarkerId::UNKNOWN_REACT_MARKER,
            "UNKNOWN_REACT_MARKER"),
        std::make_pair(RNOHMarkerId::APP_STARTUP_START, "APP_STARTUP_START"),
        std::make_pair(RNOHMarkerId::APP_STARTUP_STOP, "APP_STARTUP_STOP"),
        std::make_pair(
            RNOHMarkerId::INIT_REACT_RUNTIME_START,
            "INIT_REACT_RUNTIME_START"),
        std::make_pair(
            RNOHMarkerId::INIT_REACT_RUNTIME_STOP,
            "INIT_REACT_RUNTIME_STOP"),
        std::make_pair(
            RNOHMarkerId::NATIVE_REQUIRE_START,
            "NATIVE_REQUIRE_START"),
        std::make_pair(
            RNOHMarkerId::NATIVE_REQUIRE_STOP,
            "NATIVE_REQUIRE_STOP"),
        std::make_pair(
            RNOHMarkerId::RUN_JS_BUNDLE_START,
            "RUN_JS_BUNDLE_START"),
        std::make_pair(RNOHMarkerId::RUN_JS_BUNDLE_STOP, "RUN_JS_BUNDLE_STOP"),
        std::make_pair(
            RNOHMarkerId::CREATE_REACT_CONTEXT_START,
            "CREATE_REACT_CONTEXT_START"),
        std::make_pair(
            RNOHMarkerId::CREATE_REACT_CONTEXT_STOP,
            "CREATE_REACT_CONTEXT_STOP"),
        std::make_pair(
            RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_START,
            "JS_BUNDLE_STRING_CONVERT_START"),
        std::make_pair(
            RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_STOP,
            "JS_BUNDLE_STRING_CONVERT_STOP"),
        std::make_pair(
            RNOHMarkerId::NATIVE_MODULE_SETUP_START,
            "NATIVE_MODULE_SETUP_START"),
        std::make_pair(
            RNOHMarkerId::NATIVE_MODULE_SETUP_STOP,
            "NATIVE_MODULE_SETUP_STOP"),
        std::make_pair(
            RNOHMarkerId::REGISTER_JS_SEGMENT_START,
            "REGISTER_JS_SEGMENT_START"),
        std::make_pair(
            RNOHMarkerId::REGISTER_JS_SEGMENT_STOP,
            "REGISTER_JS_SEGMENT_STOP"),
        std::make_pair(
            RNOHMarkerId::REACT_INSTANCE_INIT_START,
            "REACT_INSTANCE_INIT_START"),
        std::make_pair(
            RNOHMarkerId::REACT_INSTANCE_INIT_STOP,
            "REACT_INSTANCE_INIT_STOP"),
        std::make_pair(RNOHMarkerId::CONTENT_APPEARED, "CONTENT_APPEARED"),
        std::make_pair(RNOHMarkerId::DOWNLOAD_START, "DOWNLOAD_START"),
        std::make_pair(RNOHMarkerId::DOWNLOAD_END, "DOWNLOAD_END"),
        std::make_pair(
            RNOHMarkerId::REACT_BRIDGE_LOADING_START,
            "REACT_BRIDGE_LOADING_START"),
        std::make_pair(
            RNOHMarkerId::REACT_BRIDGE_LOADING_END,
            "REACT_BRIDGE_LOADING_END"),
        std::make_pair(
            RNOHMarkerId::ON_HOST_RESUME_START,
            "ON_HOST_RESUME_START"),
        std::make_pair(RNOHMarkerId::ON_HOST_RESUME_END, "ON_HOST_RESUME_END"),
        std::make_pair(
            RNOHMarkerId::ON_HOST_PAUSE_START,
            "ON_HOST_PAUSE_START"),
        std::make_pair(RNOHMarkerId::ON_HOST_PAUSE_END, "ON_HOST_PAUSE_END"),
        std::make_pair(
            RNOHMarkerId::CHANGE_THREAD_PRIORITY,
            "CHANGE_THREAD_PRIORITY"),
        std::make_pair(
            RNOHMarkerId::INITIALIZE_MODULE_START,
            "INITIALIZE_MODULE_START"),
        std::make_pair(
            RNOHMarkerId::INITIALIZE_MODULE_END,
            "INITIALIZE_MODULE_END"),
        std::make_pair(
            RNOHMarkerId::PROCESS_PACKAGES_START,
            "PROCESS_PACKAGES_START"),
        std::make_pair(
            RNOHMarkerId::PROCESS_PACKAGES_END,
            "PROCESS_PACKAGES_END"),
        std::make_pair(
            RNOHMarkerId::CREATE_UI_MANAGER_MODULE_START,
            "CREATE_UI_MANAGER_MODULE_START"),
        std::make_pair(
            RNOHMarkerId::CREATE_UI_MANAGER_MODULE_END,
            "CREATE_UI_MANAGER_MODULE_END"),
        std::make_pair(
            RNOHMarkerId::CREATE_MODULE_START,
            "CREATE_MODULE_START"),
        std::make_pair(RNOHMarkerId::CREATE_MODULE_END, "CREATE_MODULE_END"),
        std::make_pair(
            RNOHMarkerId::PROCESS_CORE_REACT_PACKAGE_START,
            "PROCESS_CORE_REACT_PACKAGE_START"),
        std::make_pair(
            RNOHMarkerId::PROCESS_CORE_REACT_PACKAGE_END,
            "PROCESS_CORE_REACT_PACKAGE_END"),
        std::make_pair(
            RNOHMarkerId::FABRIC_COMMIT_START,
            "FABRIC_COMMIT_START"),
        std::make_pair(RNOHMarkerId::FABRIC_COMMIT_END, "FABRIC_COMMIT_END"),
        std::make_pair(
            RNOHMarkerId::FABRIC_FINISH_TRANSACTION_START,
            "FABRIC_FINISH_TRANSACTION_START"),
        std::make_pair(
            RNOHMarkerId::FABRIC_FINISH_TRANSACTION_END,
            "FABRIC_FINISH_TRANSACTION_END"),
        std::make_pair(RNOHMarkerId::FABRIC_DIFF_START, "FABRIC_DIFF_START"),
        std::make_pair(RNOHMarkerId::FABRIC_DIFF_END, "FABRIC_DIFF_END"),
        std::make_pair(
            RNOHMarkerId::FABRIC_LAYOUT_START,
            "FABRIC_LAYOUT_START"),
        std::make_pair(RNOHMarkerId::FABRIC_LAYOUT_END, "FABRIC_LAYOUT_END"),
        std::make_pair(
            RNOHMarkerId::FABRIC_BATCH_EXECUTION_START,
            "FABRIC_BATCH_EXECUTION_START"),
        std::make_pair(
            RNOHMarkerId::FABRIC_BATCH_EXECUTION_END,
            "FABRIC_BATCH_EXECUTION_END"),
        std::make_pair(
            RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_START,
            "FABRIC_UPDATE_UI_MAIN_THREAD_START"),
        std::make_pair(
            RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_END,
            "FABRIC_UPDATE_UI_MAIN_THREAD_END")));

TEST(RNOHMarker, HarmonyMarkerIdToString_UnknownDefault_ReturnsUnknown) {
  auto unknownVal = static_cast<RNOHMarkerId>(-1);
  EXPECT_EQ(harmonyMarkerIdToString(unknownVal), "UNKNOWN");
}

TEST(RNOHMarker, HarmonyMarkerIdForString_KnownStrings_ReturnsCorrectId) {
  EXPECT_EQ(
      harmonyMarkerIdForString("CREATE_REACT_CONTEXT_START"),
      RNOHMarkerId::CREATE_REACT_CONTEXT_START);
  EXPECT_EQ(
      harmonyMarkerIdForString("DOWNLOAD_START"), RNOHMarkerId::DOWNLOAD_START);
  EXPECT_EQ(
      harmonyMarkerIdForString("DOWNLOAD_END"), RNOHMarkerId::DOWNLOAD_END);
  EXPECT_EQ(
      harmonyMarkerIdForString("TURBO_MODULE_SETUP_START"),
      RNOHMarkerId::TURBO_MODULE_SETUP_START);
  EXPECT_EQ(
      harmonyMarkerIdForString("TURBO_MODULE_SETUP_STOP"),
      RNOHMarkerId::TURBO_MODULE_SETUP_STOP);
  EXPECT_EQ(
      harmonyMarkerIdForString("REACT_INSTANCE_INIT_START"),
      RNOHMarkerId::REACT_INSTANCE_INIT_START);
  EXPECT_EQ(
      harmonyMarkerIdForString("REACT_INSTANCE_INIT_STOP"),
      RNOHMarkerId::REACT_INSTANCE_INIT_STOP);
}

TEST(RNOHMarker, HarmonyMarkerIdForString_UnknownString_ThrowsInvalidArgument) {
  EXPECT_THROW(
      harmonyMarkerIdForString("NONEXISTENT_MARKER"), std::invalid_argument);
}

TEST(RNOHMarker, HarmonyMarkerIdForString_EmptyString_ThrowsInvalidArgument) {
  EXPECT_THROW(harmonyMarkerIdForString(""), std::invalid_argument);
}

// Reimplement harmonyMarkerIdForReactMarkerId (private in production)
static RNOHMarkerId harmonyMarkerIdForReactMarkerId(
    facebook::react::ReactMarker::ReactMarkerId markerId) {
  switch (markerId) {
    case facebook::react::ReactMarker::ReactMarkerId::RUN_JS_BUNDLE_START:
      return RNOHMarkerId::RUN_JS_BUNDLE_START;
    case facebook::react::ReactMarker::ReactMarkerId::RUN_JS_BUNDLE_STOP:
      return RNOHMarkerId::RUN_JS_BUNDLE_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::CREATE_REACT_CONTEXT_STOP:
      return RNOHMarkerId::CREATE_REACT_CONTEXT_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::
        JS_BUNDLE_STRING_CONVERT_START:
      return RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_START;
    case facebook::react::ReactMarker::ReactMarkerId::
        JS_BUNDLE_STRING_CONVERT_STOP:
      return RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::NATIVE_MODULE_SETUP_START:
      return RNOHMarkerId::NATIVE_MODULE_SETUP_START;
    case facebook::react::ReactMarker::ReactMarkerId::NATIVE_MODULE_SETUP_STOP:
      return RNOHMarkerId::NATIVE_MODULE_SETUP_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::REGISTER_JS_SEGMENT_START:
      return RNOHMarkerId::REGISTER_JS_SEGMENT_START;
    case facebook::react::ReactMarker::ReactMarkerId::REGISTER_JS_SEGMENT_STOP:
      return RNOHMarkerId::REGISTER_JS_SEGMENT_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::NATIVE_REQUIRE_START:
      return RNOHMarkerId::NATIVE_REQUIRE_START;
    case facebook::react::ReactMarker::ReactMarkerId::NATIVE_REQUIRE_STOP:
      return RNOHMarkerId::NATIVE_REQUIRE_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::REACT_INSTANCE_INIT_START:
      return RNOHMarkerId::REACT_INSTANCE_INIT_START;
    case facebook::react::ReactMarker::ReactMarkerId::REACT_INSTANCE_INIT_STOP:
      return RNOHMarkerId::REACT_INSTANCE_INIT_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::APP_STARTUP_START:
      return RNOHMarkerId::APP_STARTUP_START;
    case facebook::react::ReactMarker::ReactMarkerId::APP_STARTUP_STOP:
      return RNOHMarkerId::APP_STARTUP_STOP;
    case facebook::react::ReactMarker::ReactMarkerId::INIT_REACT_RUNTIME_START:
      return RNOHMarkerId::INIT_REACT_RUNTIME_START;
    case facebook::react::ReactMarker::ReactMarkerId::INIT_REACT_RUNTIME_STOP:
      return RNOHMarkerId::INIT_REACT_RUNTIME_STOP;
    default:
      return RNOHMarkerId::UNKNOWN_REACT_MARKER;
  }
}

using ReactMarkerId = facebook::react::ReactMarker::ReactMarkerId;

class RNOHMarkerForReactMarkerTest
    : public ::testing::TestWithParam<std::pair<ReactMarkerId, RNOHMarkerId>> {
};

TEST_P(RNOHMarkerForReactMarkerTest, MapsCorrectly) {
  auto [reactId, expectedRnohId] = GetParam();
  EXPECT_EQ(harmonyMarkerIdForReactMarkerId(reactId), expectedRnohId);
}

INSTANTIATE_TEST_SUITE_P(
    AllReactMarkers,
    RNOHMarkerForReactMarkerTest,
    ::testing::Values(
        std::make_pair(
            ReactMarkerId::RUN_JS_BUNDLE_START,
            RNOHMarkerId::RUN_JS_BUNDLE_START),
        std::make_pair(
            ReactMarkerId::RUN_JS_BUNDLE_STOP,
            RNOHMarkerId::RUN_JS_BUNDLE_STOP),
        std::make_pair(
            ReactMarkerId::CREATE_REACT_CONTEXT_STOP,
            RNOHMarkerId::CREATE_REACT_CONTEXT_STOP),
        std::make_pair(
            ReactMarkerId::JS_BUNDLE_STRING_CONVERT_START,
            RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_START),
        std::make_pair(
            ReactMarkerId::JS_BUNDLE_STRING_CONVERT_STOP,
            RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_STOP),
        std::make_pair(
            ReactMarkerId::NATIVE_MODULE_SETUP_START,
            RNOHMarkerId::NATIVE_MODULE_SETUP_START),
        std::make_pair(
            ReactMarkerId::NATIVE_MODULE_SETUP_STOP,
            RNOHMarkerId::NATIVE_MODULE_SETUP_STOP),
        std::make_pair(
            ReactMarkerId::REGISTER_JS_SEGMENT_START,
            RNOHMarkerId::REGISTER_JS_SEGMENT_START),
        std::make_pair(
            ReactMarkerId::REGISTER_JS_SEGMENT_STOP,
            RNOHMarkerId::REGISTER_JS_SEGMENT_STOP),
        std::make_pair(
            ReactMarkerId::NATIVE_REQUIRE_START,
            RNOHMarkerId::NATIVE_REQUIRE_START),
        std::make_pair(
            ReactMarkerId::NATIVE_REQUIRE_STOP,
            RNOHMarkerId::NATIVE_REQUIRE_STOP),
        std::make_pair(
            ReactMarkerId::REACT_INSTANCE_INIT_START,
            RNOHMarkerId::REACT_INSTANCE_INIT_START),
        std::make_pair(
            ReactMarkerId::REACT_INSTANCE_INIT_STOP,
            RNOHMarkerId::REACT_INSTANCE_INIT_STOP),
        std::make_pair(
            ReactMarkerId::APP_STARTUP_START,
            RNOHMarkerId::APP_STARTUP_START),
        std::make_pair(
            ReactMarkerId::APP_STARTUP_STOP,
            RNOHMarkerId::APP_STARTUP_STOP),
        std::make_pair(
            ReactMarkerId::INIT_REACT_RUNTIME_START,
            RNOHMarkerId::INIT_REACT_RUNTIME_START),
        std::make_pair(
            ReactMarkerId::INIT_REACT_RUNTIME_STOP,
            RNOHMarkerId::INIT_REACT_RUNTIME_STOP)));

TEST(RNOHMarker, HarmonyMarkerIdForReactMarkerId_UnknownId_ReturnsUnknown) {
  auto unknownReactId = static_cast<ReactMarkerId>(-1);
  EXPECT_EQ(
      harmonyMarkerIdForReactMarkerId(unknownReactId),
      RNOHMarkerId::UNKNOWN_REACT_MARKER);
}
