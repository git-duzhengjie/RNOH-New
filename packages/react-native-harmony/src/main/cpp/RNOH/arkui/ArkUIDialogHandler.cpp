/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ArkUIDialogHandler.h"
#include "RNOH/ApiVersionCheck.h"

namespace rnoh {

static std::vector<ArkUIDialogHandler*>
    dialogs; // global dialogs, for OnDismiss event

ArkUIDialogHandler::ArkUIDialogHandler()
    : handler_(NativeDialogApi::getInstance()->create()) {
  dialogs.push_back(this);
  initDialogProperties();
}

ArkUIDialogHandler::~ArkUIDialogHandler() {
  auto it = std::find(dialogs.begin(), dialogs.end(), this);
  if (it != dialogs.end()) {
    dialogs.erase(it);
  }
  destroy();
}

void ArkUIDialogHandler::show() {
  if (isShow_) {
    DLOG(INFO) << "open dialog abort, it has already been opened";
    return;
  }
  applyPageBindingIfNeeded();
  NativeDialogApi::getInstance()->show(
      handler_, false); // false means show in main window
  onShow();
  isShow_ = true;
}

void ArkUIDialogHandler::setContent(ArkUINode& node) {
  if (auto nodeHandler = node.getArkUINodeHandle()) {
    NativeDialogApi::getInstance()->setContent(handler_, nodeHandler);
  } else {
    DLOG(WARNING) << "mount dialog failed, node handler is null";
  }
}

void ArkUIDialogHandler::close() {
  if (handler_ != nullptr) {
    NativeDialogApi::getInstance()->close(handler_);
  }
  isShow_ = false;
}

void ArkUIDialogHandler::setPageBindingTarget(int32_t uniqueId) {
  m_hasPageBindingTarget = uniqueId >= 0;
  m_pageBindingTarget = m_hasPageBindingTarget ? uniqueId : -1;
  if (isShow_ && IsAtLeastApi15()) {
    applyPageBindingIfNeeded();
  }
}

void ArkUIDialogHandler::clearPageBindingTarget() {
  bool hadBinding = m_hasPageBindingTarget;
  m_hasPageBindingTarget = false;
  m_pageBindingTarget = -1;
  if (hadBinding && isShow_ && IsAtLeastApi15()) {
    applyPageBindingIfNeeded();
  }
}

void ArkUIDialogHandler::destroy() {
  close();
  if (handler_ != nullptr) {
    NativeDialogApi::getInstance()->dispose(handler_);
  }
}

void ArkUIDialogHandler::onShow() {
  if (m_dialogDelegate != nullptr) {
    m_dialogDelegate->onShow();
  }
}

void ArkUIDialogHandler::onRequestClose() {
  if (m_dialogDelegate != nullptr) {
    m_dialogDelegate->onRequestClose();
  }
}

void ArkUIDialogHandler::initDialogProperties() {
  NativeDialogApi::getInstance()->setModalMode(handler_, true);
  NativeDialogApi::getInstance()->setAutoCancel(handler_, false);
  NativeDialogApi::getInstance()->setContentAlignment(
      handler_, static_cast<int32_t>(ARKUI_ALIGNMENT_TOP_START), 0, 0);
  NativeDialogApi::getInstance()->enableCustomStyle(handler_, true);
  NativeDialogApi::getInstance()->enableCustomAnimation(handler_, true);
  NativeDialogApi::getInstance()->setMask(handler_, 0, nullptr);
  if (IsAtLeastApi15()) {
    NativeDialogApi::getInstance2()->setKeyboardAvoidDistance(
        handler_, 0, ArkUI_LengthMetricUnit::ARKUI_LENGTH_METRIC_UNIT_DEFAULT);
  }
  // register onWillDismiss event
  NativeDialogApi::getInstance()->registerOnWillDismiss(
      handler_, [](int32_t /*reason*/) -> bool {
        if (!dialogs.empty()) {
          auto dialog = dialogs.back();
          dialog->onRequestClose();
          return false;
        }
        return true;
      });
}

void ArkUIDialogHandler::applyPageBindingIfNeeded() {
  if (!IsAtLeastApi15()) {
    return;
  }
  if (m_hasPageBindingTarget) {
    NativeDialogApi::getInstance2()->setLevelUniqueId(
        handler_, m_pageBindingTarget);
    NativeDialogApi::getInstance2()->setLevelMode(
        handler_, ARKUI_LEVEL_MODE_EMBEDDED);
  } else {
    NativeDialogApi::getInstance2()->setLevelMode(
        handler_, ARKUI_LEVEL_MODE_OVERLAY);
    NativeDialogApi::getInstance2()->setLevelUniqueId(handler_, -1);
  }
}

} // namespace rnoh
