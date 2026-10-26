/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOHMarker.h"

namespace rnoh {
class HiTraceRNOHMarkerListener : public RNOHMarker::RNOHMarkerListener {
 public:
  using RNOHMarkerId = RNOHMarker::RNOHMarkerId;

  HiTraceRNOHMarkerListener() = default;

  static HiTraceRNOHMarkerListener& getInstance();
  void onMarkerReceived(
      RNOHMarkerId markerId,
      size_t rnInstanceId,
      const std::string& tag,
      double timestamp,
      uint64_t value) override;

 private:
  void logMarker(const std::string& marker, const std::string& tag);
  void logMarkerStart(const std::string& marker, const std::string& tag);
  void logMarkerFinish(const std::string& marker, const std::string& tag);
  int32_t getMessageId(const std::string& message);
  std::string makeMessage(const std::string& marker, const std::string& tag);
};

} // namespace rnoh
