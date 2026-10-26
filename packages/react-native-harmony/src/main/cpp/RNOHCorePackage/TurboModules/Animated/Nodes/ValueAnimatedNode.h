/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "AnimatedNode.h"

#include <folly/dynamic.h>
#include "RNOH/Assert.h"

namespace rnoh {

class ValueAnimatedNode : public AnimatedNode {
 public:
  using AnimatedNodeValueListener = std::function<void(double)>;
  using VirtualPixelsPerSecond = double;

  static constexpr int32_t FRAME_RATE_30 = 30;
  static constexpr int32_t FRAME_RATE_60 = 60;
  static constexpr int32_t FRAME_RATE_90 = 90;
  static constexpr int32_t FRAME_RATE_120 = 120;

  static constexpr double SPEED_UP_30_TO_60 = 5.0;    // mm/s
  static constexpr double SPEED_UP_60_TO_90 = 22.0;   // mm/s
  static constexpr double SPEED_UP_90_TO_120 = 77.0;  // mm/s

  static constexpr double SPEED_DOWN_120_TO_90 = 54.0;  // mm/s (77 × 0.7)
  static constexpr double SPEED_DOWN_90_TO_60 = 15.0;   // mm/s (22 × 0.7)
  static constexpr double SPEED_DOWN_60_TO_30 = 3.5;    // mm/s (5 × 0.7)

  ValueAnimatedNode() {}

  ValueAnimatedNode(folly::dynamic const& config) {
    RNOH_ASSERT(config.count("value") > 0);
    m_value = config["value"];
    RNOH_ASSERT(config["offset"].isDouble());
    m_offset = config["offset"].getDouble();
  }

  folly::dynamic getOutput() override {
    if (m_value.isDouble()) {
      folly::dynamic result = this->getOutputAsDouble();
      return result;
    }
    return m_value;
  }

  double getOutputAsDouble() {
    return this->getValueAsDouble() + m_offset;
  }

  void setValue(folly::dynamic value) {
    m_value = value;
  }

  void setValue(double value) {
    m_value = value;
  }

  void setOffset(double offset) {
    m_offset = offset;
  }

  void setVelocity(VirtualPixelsPerSecond velocity) {
    m_velocity = velocity;
  }

  int32_t getFrameRate() const {
    return m_frameRate;
  }

  void recalculateMinAcceptableFrameRate(float scaleRatioDpi) {
    /**
     * Note:
     * - `m_velocity`: Velocity in virtual pixels units per second (vp/s).
     * - `25.4`: Converts inches to millimeters (1 inch = 25.4 mm).
     * - `scaleRatioDpi`: `1px = (25.4 / Dpi) mm`, where `Dpi` is the display's
     * dots per inch. `1vp = scale * px`, so `scaleRatioDpi` is `scale / Dpi`.
     *
     * This formula converts velocity from vp/s to mm/s, giving a real-world
     * measurement of animation speed.
     *
     * Uses hysteresis to prevent rapid frame rate switching:
     * - Upshift uses higher thresholds, downshift uses lower thresholds (×0.7)
     * - Upshift allows multi-step jumps (e.g. 30→120 in one frame) so a fast
     *   starting animation reaches the target rate immediately, avoiding the
     *   perceptible stutter of climbing one level per frame.
     * - Downshift stays single-step to keep the slow-release behavior that
     *   prevents oscillation near thresholds.
     */
    double animationSpeed = m_velocity * 25.4 * scaleRatioDpi;

    if (m_frameRate == FRAME_RATE_30) {
      if (animationSpeed > SPEED_UP_90_TO_120) {
        m_frameRate = FRAME_RATE_120;
      } else if (animationSpeed > SPEED_UP_60_TO_90) {
        m_frameRate = FRAME_RATE_90;
      } else if (animationSpeed > SPEED_UP_30_TO_60) {
        m_frameRate = FRAME_RATE_60;
      }
    } else if (m_frameRate == FRAME_RATE_60) {
      if (animationSpeed > SPEED_UP_90_TO_120) {
        m_frameRate = FRAME_RATE_120;
      } else if (animationSpeed > SPEED_UP_60_TO_90) {
        m_frameRate = FRAME_RATE_90;
      } else if (animationSpeed < SPEED_DOWN_60_TO_30) {
        m_frameRate = FRAME_RATE_30;
      }
    } else if (m_frameRate == FRAME_RATE_90) {
      if (animationSpeed > SPEED_UP_90_TO_120) {
        m_frameRate = FRAME_RATE_120;
      } else if (animationSpeed < SPEED_DOWN_90_TO_60) {
        m_frameRate = FRAME_RATE_60;
      }
    } else if (m_frameRate == FRAME_RATE_120) {
      if (animationSpeed < SPEED_DOWN_120_TO_90) {
        m_frameRate = FRAME_RATE_90;
      }
    } else {
      m_frameRate = FRAME_RATE_60;
    }
  }

  void flattenOffset() {
    m_value = this->getOutputAsDouble();
    m_offset = 0;
  }

  void extractOffset() {
    m_offset += this->getValueAsDouble();
    m_value = 0;
  }

  void onValueUpdate() {
    if (m_valueListener.has_value()) {
      /**
       * NOTE: ValueListenerCallback on JS side expects only numbers.
       */
      m_valueListener.value()(getOutputAsDouble());
    }
  }

  void setValueListener(AnimatedNodeValueListener&& listener) {
    if (m_valueListener.has_value()) {
      throw std::runtime_error("AnimatedNode already has a value listener");
    }
    m_valueListener = std::move(listener);
  }

  void removeValueListener() {
    m_valueListener = std::nullopt;
  }

  double getValueAsDouble() {
    if (m_value.isNull()) {
      // Special case for diffClamp
      return 0;
    }
    if (m_value.isNumber()) {
      return m_value.asDouble();
    }
    LOG(ERROR) << "Unexpected value type: " << m_value.typeName();
    RNOH_ASSERT(false);
    return 0;
  }

 protected:
  folly::dynamic m_value;
  double m_offset = 0.0;
  VirtualPixelsPerSecond m_velocity = 0.0;
  int32_t m_frameRate = 60;
  std::optional<AnimatedNodeValueListener> m_valueListener;
};

} // namespace rnoh