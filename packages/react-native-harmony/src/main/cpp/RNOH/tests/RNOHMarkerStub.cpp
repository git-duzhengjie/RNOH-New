/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include "RNOH/Performance/RNOHMarker.h"
#include "cxxreact/ReactMarker.h"
namespace rnoh {
void RNOHMarker::logMarker(RNOHMarkerId, size_t, uint64_t) {}
void RNOHMarker::logMarker(const std::string&, size_t) {}
void RNOHMarker::logMarker(RNOHMarkerId, size_t) {}
void RNOHMarker::logMarker(RNOHMarkerId) {}
void RNOHMarker::logMarker(RNOHMarkerId, const char*) {}
void RNOHMarker::logMarker(const RNOHMarkerId, facebook::react::Tag) {}
void RNOHMarker::logMarker(const std::string&, const char*) {}
void RNOHMarker::logMarker(facebook::react::ReactMarker::ReactMarkerId, const char*) {}
void RNOHMarker::logMarker(RNOHMarkerId, const char*, double) {}
void RNOHMarker::addListener(RNOHMarkerListener::Shared) {}
void RNOHMarker::removeListener(RNOHMarkerListener::Shared) {}
void RNOHMarker::setLogMarkerIfNeeded() {}
}
