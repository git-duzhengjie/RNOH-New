/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * TEST-ONLY copy of `NodeApi` methods. Compiled INSTEAD of the production
 * `NodeApi.cpp` so that:
 *
 *   1. The production `NodeApi.cpp` is NOT modified (zero code intrusion).
 *   2. The host test build does not pull in `DynamicArkUILoader.h` (which
 *      references `ArkUI_TextChangeEvent`, a v15+ type absent from the v12
 *      SDK sysroot).
 *
 * API-level branching: the production `NodeApi::createNode` etc. branch on
 * `IsNewAPIVersion()` (API >= 20). In this test copy, `IsNewAPIVersion()`
 * is made INJECTABLE via a global function pointer `g_isNewApiVersionOverride`
 * so tests can exercise BOTH code paths:
 *
 *   - Old API path (default, API < 20): delegates to
 *     `NativeNodeApi::getInstance()->createNode()` — the mock vtable.
 *   - New API path (API >= 20): calls a test-supplied run-task function
 *     (`g_runTaskFunction`) that mimics
 * `DynamicArkUILoader::getRunTaskFunction()`.
 *
 * The `*WithUIContext` method bodies are copied verbatim from
 * `NodeApi.cpp`; only the `DynamicArkUILoader::` calls are replaced with
 * the injectable `g_runTaskFunction`.
 */

#include <deviceinfo.h>

#include <cstdint>
#include <functional>

#include "NativeNodeApi.h"
#include "NodeApi.h"
#include "glog/logging.h"

namespace rnoh {

namespace {

constexpr int32_t OH_SDK_API_LEVEL_20 = 20;

struct SetAttributeTaskData {
  ArkUI_NodeHandle node;
  ArkUI_NodeAttributeType attribute;
  ArkUI_AttributeItem item;
};

struct ResetAttributeTaskData {
  ArkUI_NodeHandle node;
  ArkUI_NodeAttributeType attribute;
};

struct CreateNodeData {
  ArkUI_NodeType type;
  ArkUI_NodeHandle nodeHandle;
};

struct MeasuredSizeData {
  ArkUI_NodeHandle node;
  int32_t width;
  int32_t height;
};

struct NodeChildData {
  ArkUI_NodeHandle parent;
  ArkUI_NodeHandle child;
  int32_t position;
};

// --- Injectable hooks for the new-API (API >= 20) path ---
//
// In production these come from `DynamicArkUILoader::getRunTaskFunction()`
// and `DynamicArkUILoader::isAvailable()`. Since the loader header pulls in
// v15+ types not in the v12 SDK, tests supply these via function pointers.
//
// Default: nullptr → `IsNewAPIVersion()` returns false → old-API path.
// Tests set these to exercise the new-API path.

std::function<int32_t(ArkUI_ContextHandle, void*, void (*)(void*))>*
    g_runTaskFunction = nullptr;
bool g_forceNewApiVersion = false;

bool IsNewAPIVersion() {
  if (g_forceNewApiVersion && g_runTaskFunction != nullptr) {
    return true;
  }
  return false;
}

} // namespace

// Test-only API to control the API-level branch.
void setNewApiVersionForTesting(bool enabled) {
  g_forceNewApiVersion = enabled;
}

void setRunTaskFunctionForTesting(
    std::function<int32_t(ArkUI_ContextHandle, void*, void (*)(void*))> fn) {
  // `static` gives the storage a stable address; the assignment runs on
  // EVERY call (unlike `static auto x = init` which runs once).
  static std::function<int32_t(ArkUI_ContextHandle, void*, void (*)(void*))>
      stored;
  stored = std::move(fn);
  g_runTaskFunction = &stored;
  g_forceNewApiVersion = true;
}

void clearRunTaskFunctionForTesting() {
  g_runTaskFunction = nullptr;
  g_forceNewApiVersion = false;
}

// --- NodeApi method implementations ---

NodeApi::NodeApi(ArkUI_ContextHandle uiContext) : m_uiContext(uiContext) {}

ArkUI_NodeHandle NodeApi::createNode(ArkUI_NodeType nodeType) {
  if (IsNewAPIVersion() && m_uiContext != nullptr) {
    return createNodeWithUIContext(nodeType);
  }
  return NativeNodeApi::getInstance()->createNode(nodeType);
}

void NodeApi::setAttribute(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute,
    const ArkUI_AttributeItem* item) {
  if (IsNewAPIVersion() && m_uiContext != nullptr) {
    setAttributeWithUIContext(node, attribute, item);
    return;
  }
  maybeThrow(NativeNodeApi::getInstance()->setAttribute(node, attribute, item));
}

void NodeApi::resetAttribute(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute) {
  if (IsNewAPIVersion() && m_uiContext != nullptr) {
    resetAttributeWithUIContext(node, attribute);
    return;
  }
  maybeThrow(NativeNodeApi::getInstance()->resetAttribute(node, attribute));
}

const ArkUI_AttributeItem* NodeApi::getAttribute(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute) {
  return NativeNodeApi::getInstance()->getAttribute(node, attribute);
}

void NodeApi::setMeasuredSize(
    ArkUI_NodeHandle node,
    int32_t width,
    int32_t height) {
  if (IsNewAPIVersion() && m_uiContext != nullptr) {
    setMeasuredSizeWithUIContext(node, width, height);
    return;
  }
  maybeThrow(
      NativeNodeApi::getInstance()->setMeasuredSize(node, width, height));
}

void NodeApi::insertChildAt(
    ArkUI_NodeHandle parent,
    ArkUI_NodeHandle child,
    int32_t position) {
  if (IsNewAPIVersion() && m_uiContext != nullptr) {
    insertChildAtWithUIContext(parent, child, position);
    return;
  }
  maybeThrow(
      NativeNodeApi::getInstance()->insertChildAt(parent, child, position));
}

void NodeApi::addChild(ArkUI_NodeHandle parent, ArkUI_NodeHandle child) {
  if (IsNewAPIVersion() && m_uiContext != nullptr) {
    addChildWithUIContext(parent, child);
    return;
  }
  maybeThrow(NativeNodeApi::getInstance()->addChild(parent, child));
}

// --- New-API (UIContext) path implementations (copied from NodeApi.cpp) ---
// The only change: `DynamicArkUILoader::getRunTaskFunction()` is replaced
// with the injectable `(*g_runTaskFunction)`.

ArkUI_NodeHandle NodeApi::createNodeWithUIContext(ArkUI_NodeType nodeType) {
  CreateNodeData data{nodeType, nullptr};
  int32_t result = (*g_runTaskFunction)(m_uiContext, &data, [](void* userData) {
    CreateNodeData* data = static_cast<CreateNodeData*>(userData);
    if (data == nullptr) {
      LOG(ERROR)
          << "createNodeWithUIContext callback called with null userData";
      return;
    }
    data->nodeHandle = NativeNodeApi::getInstance()->createNode(data->type);
  });
  if (result == ARKUI_ERROR_CODE_NO_ERROR && data.nodeHandle != nullptr) {
    return data.nodeHandle;
  } else {
    if (data.nodeHandle != nullptr) {
      NativeNodeApi::getInstance()->disposeNode(data.nodeHandle);
    }
    return NativeNodeApi::getInstance()->createNode(nodeType);
  }
}

void NodeApi::setAttributeWithUIContext(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute,
    const ArkUI_AttributeItem* item) {
  auto* taskData = new SetAttributeTaskData{node, attribute, *item};
  static void (*callback)(void*) = [](void* userData) {
    SetAttributeTaskData* data = static_cast<SetAttributeTaskData*>(userData);
    if (data == nullptr) {
      LOG(ERROR)
          << "setAttributeWithUIContext callback called with null userData";
      return;
    }
    NativeNodeApi::getInstance()->setAttribute(
        data->node, data->attribute, &(data->item));
    delete data;
  };
  int32_t result = (*g_runTaskFunction)(m_uiContext, taskData, callback);
  if (result != ARKUI_ERROR_CODE_NO_ERROR) {
    delete taskData;
    LOG(ERROR) << "setAttributeWithUIContext failed to run task in scope";
  }
  maybeThrow(result);
}

void NodeApi::resetAttributeWithUIContext(
    ArkUI_NodeHandle node,
    ArkUI_NodeAttributeType attribute) {
  auto* taskData = new ResetAttributeTaskData{node, attribute};
  static void (*callback)(void*) = [](void* userData) {
    ResetAttributeTaskData* data =
        static_cast<ResetAttributeTaskData*>(userData);
    if (data == nullptr) {
      LOG(ERROR)
          << "resetAttributeWithUIContext callback called with null userData";
      return;
    }
    NativeNodeApi::getInstance()->resetAttribute(data->node, data->attribute);
    delete data;
  };
  int32_t result = (*g_runTaskFunction)(m_uiContext, taskData, callback);
  if (result != ARKUI_ERROR_CODE_NO_ERROR) {
    delete taskData;
    LOG(ERROR) << "resetAttributeWithUIContext failed to run task in scope";
  }
  maybeThrow(result);
}

void NodeApi::setMeasuredSizeWithUIContext(
    ArkUI_NodeHandle node,
    int32_t width,
    int32_t height) {
  auto* taskData = new MeasuredSizeData{node, width, height};
  static void (*callback)(void*) = [](void* userData) {
    MeasuredSizeData* data = static_cast<MeasuredSizeData*>(userData);
    if (data == nullptr) {
      LOG(ERROR)
          << "setMeasuredSizeWithUIContext callback called with null userData";
      return;
    }
    NativeNodeApi::getInstance()->setMeasuredSize(
        data->node, data->width, data->height);
    delete data;
  };
  int32_t result = (*g_runTaskFunction)(m_uiContext, taskData, callback);
  if (result != ARKUI_ERROR_CODE_NO_ERROR) {
    delete taskData;
    LOG(ERROR) << "setMeasuredSizeWithUIContext failed to run task in scope";
  }
  maybeThrow(result);
}

void NodeApi::insertChildAtWithUIContext(
    ArkUI_NodeHandle parent,
    ArkUI_NodeHandle child,
    int32_t position) {
  auto* taskData = new NodeChildData{parent, child, position};
  static void (*callback)(void*) = [](void* userData) {
    NodeChildData* data = static_cast<NodeChildData*>(userData);
    if (data == nullptr) {
      LOG(ERROR)
          << "insertChildAtWithUIContext callback called with null userData";
      return;
    }
    NativeNodeApi::getInstance()->insertChildAt(
        data->parent, data->child, data->position);
    delete data;
  };
  int32_t result = (*g_runTaskFunction)(m_uiContext, taskData, callback);
  if (result != ARKUI_ERROR_CODE_NO_ERROR) {
    delete taskData;
    LOG(ERROR) << "insertChildAtWithUIContext failed to run task in scope";
  }
  maybeThrow(result);
}

void NodeApi::addChildWithUIContext(
    ArkUI_NodeHandle parent,
    ArkUI_NodeHandle child) {
  auto* taskData = new NodeChildData{parent, child};
  static void (*callback)(void*) = [](void* userData) {
    NodeChildData* data = static_cast<NodeChildData*>(userData);
    if (data == nullptr) {
      LOG(ERROR) << "addChildWithUIContext callback called with null userData";
      return;
    }
    NativeNodeApi::getInstance()->addChild(data->parent, data->child);
    delete data;
  };
  int32_t result = (*g_runTaskFunction)(m_uiContext, taskData, callback);
  if (result != ARKUI_ERROR_CODE_NO_ERROR) {
    delete taskData;
    LOG(ERROR) << "addChildWithUIContext failed to run task in scope";
  }
  maybeThrow(result);
}

} // namespace rnoh
