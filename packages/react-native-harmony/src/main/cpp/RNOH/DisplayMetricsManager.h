/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <folly/dynamic.h>
#include <memory>
#include <mutex>

namespace rnoh {
struct PhysicalPixels {
  double top;
  double left;
  double width;
  double height;
  double decorHeight;
  double scale;
  double fontScale;
  double densityDpi;
  double xDpi;
  double yDpi;

  static PhysicalPixels fromDynamic(const folly::dynamic& payload) {
    return {
        .top = payload.count("top") && payload["top"].isDouble()
            ? payload["top"].asDouble()
            : 0.0f,
        .left = payload.count("left") && payload["left"].isDouble()
            ? payload["left"].asDouble()
            : 0.0f,
        .width =
            payload["width"].isDouble() ? payload["width"].asDouble() : 0.0f,
        .height =
            payload["height"].isDouble() ? payload["height"].asDouble() : 0.0f,
        .decorHeight = payload.count("decorHeight") && payload["decorHeight"].isDouble()
            ? payload["decorHeight"].asDouble()
            : 0.0f,
        .scale =
            payload["scale"].isDouble() ? payload["scale"].asDouble() : 1.0f,
        .fontScale = payload["fontScale"].isDouble()
            ? payload["fontScale"].asDouble()
            : 1.0f,
        .densityDpi = payload["densityDpi"].isDouble()
            ? payload["densityDpi"].asDouble()
            : 480.0f,
        .xDpi =
            payload["xDpi"].isDouble() ? payload["xDpi"].asDouble() : 440.0f,
        .yDpi =
            payload["yDpi"].isDouble() ? payload["yDpi"].asDouble() : 440.0f};
  }
};

struct DisplayMetrics {
  PhysicalPixels windowPhysicalPixels;
  PhysicalPixels screenPhysicalPixels;
};

class DisplayMetricsManager {
 public:
  using Shared = std::shared_ptr<DisplayMetricsManager>;

  virtual DisplayMetrics getDisplayMetrics() = 0;

  virtual float getScaleRatioDpiX() const = 0;

  virtual float getScaleRatioDpiY() const = 0;

 protected:
  mutable std::mutex m_mutex;
  float m_scaleRatioDpiX = 1.0f;
  float m_scaleRatioDpiY = 1.0f;
};
} // namespace rnoh
