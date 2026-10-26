/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "HiTraceRNOHMarkerListener.h"
#include <glog/logging.h>

namespace rnoh {

HiTraceRNOHMarkerListener& HiTraceRNOHMarkerListener::getInstance() {
  static HiTraceRNOHMarkerListener l;
  return l;
}

void HiTraceRNOHMarkerListener::onMarkerReceived(
    const RNOHMarkerId markerId,
    size_t rnInstanceId,
    const std::string& tag,
    double timestamp,
    uint64_t value) {
  switch (markerId) {
    case RNOHMarkerId::RUN_JS_BUNDLE_START:
      logMarkerStart("RUN_JS_BUNDLE", tag);
      break;
    case RNOHMarkerId::RUN_JS_BUNDLE_STOP:
      logMarkerFinish("RUN_JS_BUNDLE", tag);
      break;
    case RNOHMarkerId::CREATE_REACT_CONTEXT_START:
      logMarkerStart("CREATE_REACT_CONTEXT", "");
      break;
    case RNOHMarkerId::CREATE_REACT_CONTEXT_STOP:
      logMarkerFinish("CREATE_REACT_CONTEXT", "");
      break;
    case RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_START:
      logMarkerStart("loadApplicationScript_StringConvert", "");
      break;
    case RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_STOP:
      logMarkerFinish("loadApplicationScript_StringConvert", "");
      break;
    case RNOHMarkerId::NATIVE_MODULE_SETUP_START:
      logMarkerStart("NATIVE_MODULE_SETUP", tag);
      break;
    case RNOHMarkerId::NATIVE_MODULE_SETUP_STOP:
      logMarkerFinish("NATIVE_MODULE_SETUP", tag);
      break;
    case RNOHMarkerId::REGISTER_JS_SEGMENT_START:
      logMarkerStart("REGISTER_JS_SEGMENT", tag);
      break;
    case RNOHMarkerId::REGISTER_JS_SEGMENT_STOP:
      logMarkerFinish("REGISTER_JS_SEGMENT", tag);
      break;
    case RNOHMarkerId::CONTENT_APPEARED:
      logMarker("CONTENT_APPEARED", tag);
      break;
    case RNOHMarkerId::CHANGE_THREAD_PRIORITY:
      logMarker("CHANGE_THREAD_PRIORITY", tag);
      break;
    case RNOHMarkerId::FABRIC_COMMIT_START:
      logMarkerStart("FABRIC_COMMIT", "");
      break;
    case RNOHMarkerId::FABRIC_COMMIT_END:
      logMarkerFinish("FABRIC_COMMIT", "");
      break;
    case RNOHMarkerId::FABRIC_FINISH_TRANSACTION_START:
      logMarkerStart("FABRIC_FINISH_TRANSACTION", "");
      break;
    case RNOHMarkerId::FABRIC_FINISH_TRANSACTION_END:
      logMarkerFinish("FABRIC_FINISH_TRANSACTION", "");
      break;
    case RNOHMarkerId::FABRIC_DIFF_START:
      logMarkerStart("FABRIC_DIFF", "");
      break;
    case RNOHMarkerId::FABRIC_DIFF_END:
      logMarkerFinish("FABRIC_DIFF", "");
      break;
    case RNOHMarkerId::FABRIC_LAYOUT_START:
      logMarkerStart("FABRIC_LAYOUT", "");
      break;
    case RNOHMarkerId::FABRIC_LAYOUT_END:
      logMarkerFinish("FABRIC_LAYOUT", "");
      break;
    case RNOHMarkerId::FABRIC_BATCH_EXECUTION_START:
      logMarkerStart("FABRIC_BATCH_EXECUTION", "");
      break;
    case RNOHMarkerId::FABRIC_BATCH_EXECUTION_END:
      logMarkerFinish("FABRIC_BATCH_EXECUTION", "");
      break;
    case RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_START:
      logMarkerStart("FABRIC_UPDATE_UI_MAIN_THREAD", "");
      break;
    case RNOHMarkerId::FABRIC_UPDATE_UI_MAIN_THREAD_END:
      logMarkerFinish("FABRIC_UPDATE_UI_MAIN_THREAD", "");
      break;
    case RNOHMarkerId::REACT_BRIDGE_LOADING_START:
      logMarkerStart("REACT_BRIDGE_LOADING", tag);
      break;
    case RNOHMarkerId::REACT_BRIDGE_LOADING_END:
      logMarkerFinish("REACT_BRIDGE_LOADING", tag);
      break;
    case RNOHMarkerId::ON_HOST_RESUME_START:
      logMarkerStart("ON_HOST_RESUME", tag);
      break;
    case RNOHMarkerId::ON_HOST_RESUME_END:
      logMarkerFinish("ON_HOST_RESUME", tag);
      break;
    case RNOHMarkerId::ON_HOST_PAUSE_START:
      logMarkerStart("ON_HOST_PAUSE", tag);
      break;
    case RNOHMarkerId::ON_HOST_PAUSE_END:
      logMarkerFinish("ON_HOST_PAUSE", tag);
      break;
    case RNOHMarkerId::REACT_INSTANCE_INIT_START:
      logMarkerStart("REACT_INSTANCE_INIT", tag);
      break;
    case RNOHMarkerId::REACT_INSTANCE_INIT_STOP:
      logMarkerFinish("REACT_INSTANCE_INIT", tag);
      break;
    case RNOHMarkerId::INITIALIZE_MODULE_START:
      logMarkerStart("INITIALIZE_MODULE", tag);
      break;
    case RNOHMarkerId::INITIALIZE_MODULE_END:
      logMarkerFinish("INITIALIZE_MODULE", tag);
      break;
    case RNOHMarkerId::PROCESS_PACKAGES_START:
      logMarkerStart("PROCESS_PACKAGES", tag);
      break;
    case RNOHMarkerId::PROCESS_PACKAGES_END:
      logMarkerFinish("PROCESS_PACKAGES", tag);
      break;
    case RNOHMarkerId::CREATE_UI_MANAGER_MODULE_START:
      logMarkerStart("CREATE_UI_MANAGER_MODULE", tag);
      break;
    case RNOHMarkerId::CREATE_UI_MANAGER_MODULE_END:
      logMarkerFinish("CREATE_UI_MANAGER_MODULE", tag);
      break;
    case RNOHMarkerId::CREATE_MODULE_START:
      logMarkerStart("CREATE_MODULE", tag);
      break;
    case RNOHMarkerId::CREATE_MODULE_END:
      logMarkerFinish("CREATE_MODULE", tag);
      break;
    case RNOHMarkerId::PROCESS_CORE_REACT_PACKAGE_START:
      logMarkerStart("PROCESS_CORE_REACT_PACKAGE", tag);
      break;
    case RNOHMarkerId::PROCESS_CORE_REACT_PACKAGE_END:
      logMarkerFinish("PROCESS_CORE_REACT_PACKAGE", tag);
      break;
    case RNOHMarkerId::DOWNLOAD_START:
      logMarkerStart("DOWNLOAD", tag);
      break;
    case RNOHMarkerId::DOWNLOAD_END:
      logMarkerFinish("DOWNLOAD", tag);
      break;
    case RNOHMarkerId::NATIVE_REQUIRE_START:
    case RNOHMarkerId::NATIVE_REQUIRE_STOP:
      break;
    case RNOHMarkerId::APP_STARTUP_START:
      logMarkerStart("APP_STARTUP", "");
      break;
    case RNOHMarkerId::APP_STARTUP_STOP:
      logMarkerFinish("APP_STARTUP", "");
      break;
    case RNOHMarkerId::INIT_REACT_RUNTIME_START:
      logMarkerStart("INIT_REACT_RUNTIME", "");
      break;
    case RNOHMarkerId::INIT_REACT_RUNTIME_STOP:
      logMarkerFinish("INIT_REACT_RUNTIME", "");
      break;
    default:
      DLOG(WARNING) << "Unexpected marker";
  }
}

void HiTraceRNOHMarkerListener::logMarker(
    const std::string& marker,
    const std::string& tag) {
  logMarkerStart(marker, tag);
  logMarkerFinish(marker, tag);
}

void HiTraceRNOHMarkerListener::logMarkerStart(
    const std::string& marker,
    const std::string& tag) {
#ifdef WITH_HITRACE_REACT_MARKER
  auto message = makeMessage(marker, tag);
  OH_HiTrace_StartAsyncTrace(message.c_str(), getMessageId(message.c_str()));
#endif
}

void HiTraceRNOHMarkerListener::logMarkerFinish(
    const std::string& marker,
    const std::string& tag) {
#ifdef WITH_HITRACE_REACT_MARKER
  auto message = makeMessage(marker, tag);
  OH_HiTrace_FinishAsyncTrace(message.c_str(), getMessageId(message.c_str()));
#endif
}

std::string HiTraceRNOHMarkerListener::makeMessage(
    const std::string& marker,
    const std::string& tag) {
  std::string message = "ReactMarker::" + marker;
  if (!tag.empty()) {
    message += "::Tag::";
    message += tag;
  }
  return message;
};

int32_t HiTraceRNOHMarkerListener::getMessageId(const std::string& message) {
  std::hash<std::string> hasher;
  size_t hash = hasher(message);
  // Ensure the hash fits into int32_t. We don't care that much about collisions
  int32_t small_hash = hash ^ (hash >> 32);
  return small_hash;
};

} // namespace rnoh