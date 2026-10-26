/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <arkui/native_interface.h>
#include <arkui/native_node.h>
#include <mutex>

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * A singleton class that provides access to the native API for handling Node
 * operations.
 * This class acts as an interface to the native system for performing
 * actions related to UI nodes
 */
class NativeNodeApi {
 public:
  /**
   * @brief Gets the singleton instance of the NativeNodeApi.
   * This method returns a pointer to the single instance of the NativeNodeApi
   * class.
   * The instance of this class is used to interact with the native node API
   * (represented by ArkUI_NativeNodeAPI_1).
   *
   * @return ArkUI_NativeNodeAPI_1* A pointer to the NativeNode API instance.
   */
  static ArkUI_NativeNodeAPI_1* getInstance();
  static void resetInstance();

  static ArkUI_NativeNodeAPI_1* INSTANCE;
  static std::mutex instanceMutex;

 private:
  NativeNodeApi() {}

  static ArkUI_NativeNodeAPI_1* api;
};

} // namespace rnoh
