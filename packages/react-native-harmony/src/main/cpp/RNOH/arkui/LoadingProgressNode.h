/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "ArkUINode.h"

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * A UI node class that represents a loading progress indicator. This class
 * allows to create and configure a loading progress element within a user
 * interface, allowing customization of properties like color and animation
 * state.
 */
class LoadingProgressNode : public ArkUINode {
 public:
  /**
   * @brief Initializes a new instance of the loading progress indicator node
   * with the specified context.
   *
   * @param context The Context of ArkUINode.
   */
  explicit LoadingProgressNode(
      const ArkUINode::Context::Shared& context = nullptr);

  /**
   * @brief Sets the foreground color of the loading progress bar.
   *
   * @param color The color to set for the loading progress bar.
   * @return LoadingProgressNode& A reference to the current instance of the
   * LoadingProgressNode, allowing for method chaining.
   */
  LoadingProgressNode& setColor(facebook::react::SharedColor const& color);

  LoadingProgressNode& resetColor();

  /**
   * @brief Enables or disables animation of the loading progress indicator.
   *
   * @param enable A boolean indicating whether the animation should be enabled
   * (true) or disabled (false).
   * @return LoadingProgressNode& A reference to the current instance of the
   * LoadingProgressNode, allowing for method chaining.
   */
  LoadingProgressNode& setAnimating(const bool& enable);
};

} // namespace rnoh
