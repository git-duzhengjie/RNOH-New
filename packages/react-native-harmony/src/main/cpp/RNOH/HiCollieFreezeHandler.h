/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <memory>

namespace rnoh {

class RNInstanceInternal;

class HiCollieFreezeHandler final {
 public:
  static void registerInstance(
      const std::shared_ptr<RNInstanceInternal>& instance);
  static void unregisterInstance(size_t rnInstanceId);
  static void markInstanceDestroyed(size_t rnInstanceId);

 private:
  static void ensureFreezeCallbackRegisteredOnce();
};

} // namespace rnoh
