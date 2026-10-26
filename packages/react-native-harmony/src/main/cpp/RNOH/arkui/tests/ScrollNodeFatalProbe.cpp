/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <cstring>
#include "RNOH/arkui/ScrollNode.h"
#if defined(_WIN32)
#include <windows.h>
#endif
#include "MockArkUIAPI.h"
#include "RNOH/arkui/NativeNodeApi.h"

using namespace rnoh;

int main(int argc, char** argv) {
#if defined(_WIN32)
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
  if (argc != 2) {
    return 2;
  }

  resetMockState();
  NativeNodeApi::INSTANCE = nullptr;
  ScrollNode node;
  ArkUINode child(mockCreateNode(ARKUI_NODE_STACK));

  if (std::strcmp(argv[1], "insert") == 0) {
    ArkUINode secondChild(mockCreateNode(ARKUI_NODE_STACK));
    node.insertChild(child);
    node.insertChild(secondChild);
    return 0;
  }
  if (std::strcmp(argv[1], "remove") == 0) {
    node.removeChild(child);
    return 0;
  }
  return 2;
}
