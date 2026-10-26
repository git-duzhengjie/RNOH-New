/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "RNOHMarker.h"

namespace rnoh {

class NativePerformanceRNOHMarkerListener : public RNOHMarker::RNOHMarkerListener {
 public:
  using RNOHMarkerId = RNOHMarker::RNOHMarkerId;

  NativePerformanceRNOHMarkerListener() = default;

  static NativePerformanceRNOHMarkerListener& getInstance() {
    static NativePerformanceRNOHMarkerListener listener;
    return listener;
  }

  void onMarkerReceived(
      RNOHMarkerId markerId,
      size_t rnInstanceId,
      const std::string& tag,
      double timestamp,
      uint64_t value) override;

 private:
  static void logStartupMarkers(RNOHMarkerId markerId, double timestamp);
};

} // namespace rnoh
