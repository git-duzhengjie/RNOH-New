/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ActionSheetManagerTurboModule.h"

namespace rnoh {
using namespace facebook;

ActionSheetManagerTurboModule::ActionSheetManagerTurboModule(
    const ArkTSTurboModule::Context ctx,
    const std::string name)
    : ArkTSTurboModule(ctx, name) {
  methodMap_ = {
      ARK_METHOD_METADATA(getConstants, 0),
      ARK_METHOD_METADATA(showActionSheetWithOptions, 2),
      ARK_METHOD_METADATA(showShareActionSheetWithOptions, 3),
      ARK_METHOD_METADATA(dismissActionSheet, 0)};
}
} // namespace rnoh
