/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <queue>
#include "ComponentInstanceFactory.h"
#include "ComponentInstanceRegistry.h"
#include "react/renderer/core/ReactPrimitives.h"
#include "react/renderer/core/ShadowNode.h"

namespace rnoh {

/**
 * @internal
 * @threadSafe
 */
class ComponentInstancePreallocationRequestQueue {
 public:
  using Request = struct {
    facebook::react::Tag tag;
    facebook::react::ComponentHandle componentHandle;
    facebook::react::ComponentName componentName;
    facebook::react::Props::Shared props;
  };
  class Delegate {
    friend ComponentInstancePreallocationRequestQueue;

   public:
    using Weak = std::weak_ptr<Delegate>;

   protected:
    virtual void onPushPreallocationRequest() = 0;
    virtual void processPreallocationRequest(Request const& shadowView) = 0;
  };

  using Shared = std::shared_ptr<ComponentInstancePreallocationRequestQueue>;
  using Weak = std::weak_ptr<ComponentInstancePreallocationRequestQueue>;

 private:
  std::queue<Request> m_queue;
  std::mutex m_mtx;
  Delegate::Weak m_weakDelegate;

 public:
  void setDelegate(
      ComponentInstancePreallocationRequestQueue::Delegate::Weak weakDelegate);

  void push(Request request);

  std::optional<Request> pop();

  void clear();

  bool isEmpty();
};
} // namespace rnoh
