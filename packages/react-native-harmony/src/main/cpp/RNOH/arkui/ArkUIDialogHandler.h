/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include "ArkUINode.h"
#include "NativeDialogApi.h"

namespace rnoh {

/**
 * @actor RNOH_LIBRARY
 * A delegate interface for handling dialog-related events, such as showing or
 * requesting to close the dialog.
 * This interface can be implemented by other classes to define custom
 * behavior when a dialog is shown or closed.
 * It should be noted that ArkUIHandler::setDialogDelegate needs to be called
 * . Those methods won't work without that call.
 */
class ArkUIDialogDelegate {
 public:
  virtual ~ArkUIDialogDelegate() = default;

  /**
   * @brief Called when the dialog is shown.
   * This method can be overridden to define custom behavior when the dialog
   * becomes visible.
   */
  virtual void onShow(){};

  /**
   * @brief Called when a request to close the dialog is made.
   * This method can be overridden to define custom behavior when the dialog
   * is about to close.
   */
  virtual void onRequestClose(){};
};

/**
 * @actor RNOH_LIBRARY
 * A class for managing dialog interactions.
 * This class is responsible for showing, closing, and managing the content
 * of the dialog.
 * It delegates show and close actions to the ArkUIDialogDelegate.
 */
class ArkUIDialogHandler {
 public:
  /**
   * @brief Initializes the dialog handler and sets up necessary properties.
   */
  ArkUIDialogHandler();

  /**
   * @brief Deleted copy constructor to prevent copying of ArkUIDialogHandler
   * objects.
   * ArkUIDialogHandler should not be copied.
   */
  ArkUIDialogHandler(ArkUIDialogHandler const& other) = delete;

  /**
   * @brief Deleted copy assignment operator to prevent copying of
   * ArkUIDialogHandler objects.
   * ArkUIDialogHandler objects should not be assigned to each other.
   */
  ArkUIDialogHandler& operator=(const ArkUIDialogHandler& other) = delete;

  /**
   * @brief Cleans up the dialog resources and ensures proper destruction of the
   * handler.
   */
  ~ArkUIDialogHandler();

  /**
   * @brief Sets the content of the dialog by assigning a ArkUINode to be
   * displayed.
   * This method allows you to define the content (like text, buttons, etc.)
   * within the dialog.
   *
   * @param node The ArkUINode to be displayed inside the dialog.
   */
  void setContent(ArkUINode& node);

  /**
   * @brief Shows the dialog to the user.
   * This method makes the dialog visible and triggers the onShow() callback.
   */
  void show();

  /**
   * @brief Closes the dialog.
   * Closes a custom dialog box. If the dialog box has been closed, this API
   * does not take effect
   */
  void close();

  /**
   * @brief Called when the dialog is shown.
   * This method invokes the onShow() method of the delegate, if one is set.
   */
  void onShow();

  /**
   * @brief Called when there is a request to close the dialog.
   * This method shouldn't be called from outside of that class.
   */
  void onRequestClose();

  /**
   * @brief Sets the page binding target for the dialog.
   * This associates the dialog with a specific page identified by uniqueId,
   * enabling proper page-dialog relationship management.
   *
   * @param uniqueId The identifier of the page to bind to this dialog.
   */
  void setPageBindingTarget(int32_t uniqueId);

  /**
   * @brief Clears the previously set page binding target.
   * Removes the association between this dialog and its bound page.
   */
  void clearPageBindingTarget();

  /**
   * @brief Gets the current page binding target ID.
   *
   * @return The current binding target ID, or -1 if no binding is set.
   */
  int32_t getPageBindingTarget() const {
    return m_pageBindingTarget;
  }

  /**
   * @brief Sets the delegate to handle dialog events such as showing or
   * closing.
   * This allows custom behavior when the dialog is shown or closed.
   *
   * @param dialogDelegate A pointer to the delegate object that will handle
   * dialog events.
   */
  void setDialogDelegate(ArkUIDialogDelegate* dialogDelegate) {
    m_dialogDelegate = dialogDelegate;
  }

  /**
   * @brief Checks if the dialog is currently shown.
   *
   * @return bool Returns `true` if the dialog is currently shown, otherwise
   * `false`.
   */
  bool isShow() {
    return isShow_;
  }

 private:
  void destroy();

  void initDialogProperties();

  void applyPageBindingIfNeeded();

  bool isShow_ = false;
  ArkUI_NativeDialogHandle handler_;
  ArkUIDialogDelegate* m_dialogDelegate{};
  bool m_hasPageBindingTarget = false;
  int32_t m_pageBindingTarget = -1;
};

} // namespace rnoh