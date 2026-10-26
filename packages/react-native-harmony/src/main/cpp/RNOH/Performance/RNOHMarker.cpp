/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOHMarker.h"
#include <glog/logging.h>
#include <algorithm>
#include <mutex>
#include <vector>
#include "RNOH/Assert.h"

namespace rnoh {

std::vector<std::weak_ptr<RNOHMarker::RNOHMarkerListener>>
    RNOHMarker::listeners = {};
std::mutex RNOHMarker::listenersMutex = {};

void RNOHMarker::addListener(RNOHMarkerListener::Shared listener) {
  if (listener == nullptr) {
    return;
  }
  std::scoped_lock lock{listenersMutex};
  listeners.push_back(listener);
}

void RNOHMarker::removeListener(RNOHMarkerListener::Shared listener) {
  if (listener == nullptr) {
    return;
  }
  std::scoped_lock lock{listenersMutex};
  listeners.erase(
      std::remove_if(
          listeners.begin(),
          listeners.end(),
          [&](const auto& weak) {
            auto locked = weak.lock();
            return locked == nullptr || locked.get() == listener.get();
          }),
      listeners.end());
}

void RNOHMarker::setLogMarkerIfNeeded() {
  static std::once_flag flag{};
  std::call_once(flag, []() {
    ReactMarker::logTaggedMarkerImpl = RNOHMarker::logMarker;
    ReactMarker::logTaggedMarkerBridgelessImpl = RNOHMarker::logMarker;
  });
}

void RNOHMarker::logMarker(RNOHMarkerId markerId) {
  logMarker(markerId, "");
}

void RNOHMarker::logMarker(RNOHMarkerId markerId, facebook::react::Tag tag) {
  logMarker(markerId, std::to_string(tag).c_str());
}

void RNOHMarker::logMarker(RNOHMarkerId markerId, size_t rnInstanceId) {
  RNOHMarker::notifyListeners(
      markerId,
      rnInstanceId,
      nullptr,
      HighResTimeStamp::now().toDOMHighResTimeStamp());
}

void RNOHMarker::logMarker(const std::string& markerId, size_t rnInstanceId) {
  logMarker(harmonyMarkerIdForString(markerId), rnInstanceId);
}

void RNOHMarker::logMarker(RNOHMarkerId markerId, const char* tag) {
  notifyListeners(
      markerId, -1, tag, HighResTimeStamp::now().toDOMHighResTimeStamp());
}

void RNOHMarker::logMarker(const std::string& markerId, const char* tag) {
  logMarker(harmonyMarkerIdForString(markerId), tag);
}

void RNOHMarker::logMarker(
    ReactMarker::ReactMarkerId markerId,
    const char* tag) {
  logMarker(harmonyMarkerIdForReactMarkerId(markerId), tag);
}

void RNOHMarker::logMarker(
    RNOHMarker::RNOHMarkerId markerId,
    const char* tag,
    double timestamp) {
  RNOHMarker::notifyListeners(markerId, -1, tag, timestamp);
}

void RNOHMarker::logMarker(
    RNOHMarker::RNOHMarkerId markerId,
    size_t rnInstanceId,
    uint64_t value) {
  RNOHMarker::notifyListeners(markerId, rnInstanceId, nullptr, value);
}

void RNOHMarker::notifyListeners(
    RNOHMarkerId markerId,
    size_t rnInstanceId,
    const char* tag,
    double timestamp) {
  auto tagStr = tag == nullptr ? "" : std::string(tag);
  std::vector<RNOHMarkerListener::Shared> snapshot;
  {
    std::scoped_lock lock{listenersMutex};
    snapshot.reserve(listeners.size());
    for (auto it = listeners.begin(); it != listeners.end();) {
      if (auto shared = it->lock()) {
        snapshot.push_back(std::move(shared));
        ++it;
      } else {
        it = listeners.erase(it);
      }
    }
  }
  for (auto& listener : snapshot) {
    listener->onMarkerReceived(markerId, rnInstanceId, tagStr, timestamp, 0);
  }
}

void RNOHMarker::notifyListeners(
    RNOHMarkerId markerId,
    size_t rnInstanceId,
    const char* tag,
    uint64_t value) {
  auto tagStr = tag == nullptr ? "" : std::string(tag);
  std::vector<RNOHMarkerListener::Shared> snapshot;
  {
    std::scoped_lock lock{listenersMutex};
    snapshot.reserve(listeners.size());
    for (auto it = listeners.begin(); it != listeners.end();) {
      if (auto shared = it->lock()) {
        snapshot.push_back(std::move(shared));
        ++it;
      } else {
        it = listeners.erase(it);
      }
    }
  }
  for (auto& listener : snapshot) {
    listener->onMarkerReceived(markerId, rnInstanceId, tagStr, 0, value);
  }
}

void RNOHMarker::setAppStartTime(double appStartTime) {
  sAppStartTime = appStartTime;
}

double RNOHMarker::getAppStartTime() {
  return sAppStartTime;
}

RNOHMarker::RNOHMarkerId RNOHMarker::harmonyMarkerIdForReactMarkerId(
    const ReactMarker::ReactMarkerId markerId) {
  switch (markerId) {
    case ReactMarker::ReactMarkerId::RUN_JS_BUNDLE_START:
      return RNOHMarkerId::RUN_JS_BUNDLE_START;
    case ReactMarker::ReactMarkerId::RUN_JS_BUNDLE_STOP:
      return RNOHMarkerId::RUN_JS_BUNDLE_STOP;
    case ReactMarker::ReactMarkerId::CREATE_REACT_CONTEXT_STOP:
      return RNOHMarkerId::CREATE_REACT_CONTEXT_STOP;
    case ReactMarker::ReactMarkerId::JS_BUNDLE_STRING_CONVERT_START:
      return RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_START;
    case ReactMarker::ReactMarkerId::JS_BUNDLE_STRING_CONVERT_STOP:
      return RNOHMarkerId::JS_BUNDLE_STRING_CONVERT_STOP;
    case ReactMarker::ReactMarkerId::NATIVE_MODULE_SETUP_START:
      return RNOHMarkerId::NATIVE_MODULE_SETUP_START;
    case ReactMarker::ReactMarkerId::NATIVE_MODULE_SETUP_STOP:
      return RNOHMarkerId::NATIVE_MODULE_SETUP_STOP;
    case ReactMarker::ReactMarkerId::REGISTER_JS_SEGMENT_START:
      return RNOHMarkerId::REGISTER_JS_SEGMENT_START;
    case ReactMarker::ReactMarkerId::REGISTER_JS_SEGMENT_STOP:
      return RNOHMarkerId::REGISTER_JS_SEGMENT_STOP;
    case ReactMarker::ReactMarkerId::NATIVE_REQUIRE_START:
      return RNOHMarkerId::NATIVE_REQUIRE_START;
    case ReactMarker::ReactMarkerId::NATIVE_REQUIRE_STOP:
      return RNOHMarkerId::NATIVE_REQUIRE_STOP;
    case ReactMarker::ReactMarkerId::REACT_INSTANCE_INIT_START:
      return RNOHMarkerId::REACT_INSTANCE_INIT_START;
    case ReactMarker::ReactMarkerId::REACT_INSTANCE_INIT_STOP:
      return RNOHMarkerId::REACT_INSTANCE_INIT_STOP;
    case ReactMarker::ReactMarkerId::APP_STARTUP_START:
      return RNOHMarkerId::APP_STARTUP_START;
    case ReactMarker::ReactMarkerId::APP_STARTUP_STOP:
      return RNOHMarkerId::APP_STARTUP_STOP;
    case ReactMarker::ReactMarkerId::INIT_REACT_RUNTIME_START:
      return RNOHMarkerId::INIT_REACT_RUNTIME_START;
    case ReactMarker::ReactMarkerId::INIT_REACT_RUNTIME_STOP:
      return RNOHMarkerId::INIT_REACT_RUNTIME_STOP;
    default:
      DLOG(WARNING) << "Unhandled ReactMarkerId " << markerId;
      RNOH_ASSERT(false);
      return RNOHMarkerId::UNKNOWN_REACT_MARKER;
  }
}

std::string RNOHMarker::harmonyMarkerIdToString(const RNOHMarkerId markerId) {
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
      DLOG(WARNING) << "Unknown RNOHMarkerId " << static_cast<int>(markerId);
      return "UNKNOWN";
  }
}

RNOHMarker::RNOHMarkerId RNOHMarker::harmonyMarkerIdForString(
    const std::string& markerId) {
  if (markerId == "CREATE_REACT_CONTEXT_START") {
    return RNOHMarkerId::CREATE_REACT_CONTEXT_START;
  }
  if (markerId == "DOWNLOAD_START") {
    return RNOHMarkerId::DOWNLOAD_START;
  }
  if (markerId == "DOWNLOAD_END") {
    return RNOHMarkerId::DOWNLOAD_END;
  }
  if (markerId == "TURBO_MODULE_SETUP_START") {
    return RNOHMarkerId::TURBO_MODULE_SETUP_START;
  }
  if (markerId == "TURBO_MODULE_SETUP_STOP") {
    return RNOHMarkerId::TURBO_MODULE_SETUP_STOP;
  }
  if (markerId == "REACT_INSTANCE_INIT_START") {
    return RNOHMarkerId::REACT_INSTANCE_INIT_START;
  }
  if (markerId == "REACT_INSTANCE_INIT_STOP") {
    return RNOHMarkerId::REACT_INSTANCE_INIT_STOP;
  }
  throw std::invalid_argument("Unknown markerId: " + markerId);
}

} // namespace rnoh
