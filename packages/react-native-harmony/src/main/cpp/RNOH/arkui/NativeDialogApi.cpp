/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "NativeDialogApi.h"
#include <glog/logging.h>

namespace rnoh {

ArkUI_NativeDialogAPI_1* NativeDialogApi::getInstance() {
  static ArkUI_NativeDialogAPI_1* INSTANCE = nullptr;
  if (INSTANCE == nullptr) {
    OH_ArkUI_GetModuleInterface(
        ARKUI_NATIVE_DIALOG, ArkUI_NativeDialogAPI_1, INSTANCE);
    if (INSTANCE == nullptr) {
      LOG(FATAL) << "Failed to get ArkUI_NativeDialogAPI_1 instance.";
    }
  }
  return INSTANCE;
}

ArkUI_NativeDialogAPI_2* NativeDialogApi::getInstance2() {
  static ArkUI_NativeDialogAPI_2* INSTANCE = nullptr;
  if (INSTANCE == nullptr) {
    OH_ArkUI_GetModuleInterface(
        ARKUI_NATIVE_DIALOG, ArkUI_NativeDialogAPI_2, INSTANCE);
    if (INSTANCE == nullptr) {
      LOG(FATAL) << "Failed to get ArkUI_NativeDialogAPI_2 instance.";
    }
  }
  return INSTANCE;
}

} // namespace rnoh