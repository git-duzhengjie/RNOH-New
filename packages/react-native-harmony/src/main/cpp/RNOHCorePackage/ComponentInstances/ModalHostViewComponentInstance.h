/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <react/renderer/components/modal/ModalHostViewShadowNode.h>
#include "RNOH/ArkTSBridge.h"
#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/CppComponentInstance.h"
#include "RNOH/arkui/ArkUIDialogHandler.h"
#include "RNOH/arkui/CustomNode.h"
#include "RNOH/arkui/StackNode.h"
#include "RNOH/arkui/UIInputEventHandler.h"

namespace rnoh {
class ModalHostViewComponentInstance
    : public CppComponentInstance<facebook::react::ModalHostViewShadowNode>,
      public ArkUIDialogDelegate,
      public ArkTSMessageHub::Observer {
 private:
  using ScreenOrientation = facebook::react::ModalHostViewEventEmitter::
      OnOrientationChangeOrientation;
  CustomNode m_virtualNode{};
  StackNode m_rootStackNode;
  CustomNode m_rootCustomNode{};
  ArkUIDialogHandler m_dialogHandler;
  std::shared_ptr<UIInputEventHandler> m_touchHandler;
  std::shared_ptr<ArkTSMessageHandler> m_displaySizeChangeHandler;
  std::optional<ScreenOrientation> m_screenOrientation;
  bool m_modalVisibilityReported{false};

 private:
  void updateBindingFromProps(SharedConcreteProps const& props);

  void updateDisplayRect(
      DisplayMetrics const& displayMetrics,
      SharedConcreteState const& state);

  void updateSlideTransition(DisplayMetrics const& displayMetrics);
  void closeDialog();

 public:
  ModalHostViewComponentInstance(Context context);
  ~ModalHostViewComponentInstance() override;

  std::weak_ptr<UIInputEventHandler> getUIInputEventHandler() override {
    return m_touchHandler;
  }

  void onPropsChanged(SharedConcreteProps const& props) override;
  void onStateChanged(SharedConcreteState const& state) override;
  void setLayout(facebook::react::LayoutMetrics layoutMetrics) override;

  void onChildInserted(
      ComponentInstance::Shared const& childComponentInstance,
      std::size_t index) override;
  void onChildRemoved(
      ComponentInstance::Shared const& childComponentInstance) override;

  bool canHandleTouch() const override;
  bool canChildrenHandleTouch() const override;

  void onFinalizeUpdates() override;

  void showDialog();

  void onShow() override;

  void onRequestClose() override;

  CustomNode& getLocalRootArkUINode() override;

  // ArkTSMessageHub::Observer
  void onMessageReceived(ArkTSMessage const& message) override;

 protected:
  void onArkUINodeTouchIntercept(
      const ArkUI_UIInputEvent* event) override;

  friend class ModalHostTouchHandler;
};
} // namespace rnoh
