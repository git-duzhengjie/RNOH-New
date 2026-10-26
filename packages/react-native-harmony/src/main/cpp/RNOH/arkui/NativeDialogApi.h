/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <arkui/native_dialog.h>
#include <arkui/native_interface.h>

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * A singleton class that provides access to the native dialog API.
 * This class acts as a wrapper for dialog-related operations, providing a
 * unified interface for creating, showing, and managing native dialogs.
 */
class NativeDialogApi {
 public:
  /**
   * @brief Gets the singleton instance of the NativeDialogApi.
   *
   * This method returns a pointer to the single instance of the
   * NativeDialogApi class, which can be used to interact with the native
   * dialog API (represented by ArkUI_NativeDialogAPI_1).
   *
   * @return ArkUI_NativeDialogAPI_1* A pointer to the NativeDialog API
   * instance.
   */
  static ArkUI_NativeDialogAPI_1* getInstance();

  /**
   * @brief Gets the singleton instance of the NativeDialogApi.
   *
   * @return ArkUI_NativeDialogAPI_2* A pointer to the NativeDialog API
   * instance.
   */
  static ArkUI_NativeDialogAPI_2* getInstance2();

 private:
  NativeDialogApi() {}
};

} // namespace rnoh