/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import { EventEmitter } from './EventEmitter';
import { RNOHLogger } from './RNOHLogger';
import { RNOHError } from './RNOHError';
import { DevServerHelper } from './DevServerHelper';

interface DevMenuItem {
  title: string;
}

interface PauseInDebuggerDialogPayload {
  message: string;
}

/**
 * @actor RNOH_APP
 *
 * @brief DevToolsController manages developer tool functionalities for RNOH,
 * including error dialogs, element inspector, dev menu interactions,
 * and opening tools like Flipper or debugger.
 */
export class DevToolsController {
  /**
   * @brief Event emitter for dev tool-related events.
   * Events include reload, error dialogs, loading views,
   * and developer menu actions.
   */
  public eventEmitter = new EventEmitter<{
    'RELOAD': [{ reason: string | undefined }],
    'DISMISS_RNOH_ERROR_DIALOG',
    'SHOW_DEV_LOADING_VIEW': [
      rnInstanceId: number,
      message: string,
      color?: number,
      backgroundColor?: number
    ],
    'HIDE_DEV_LOADING_VIEW': [rnInstanceId: number],
    'NEW_ERROR': [RNOHError],
    'TOGGLE_ELEMENT_INSPECTOR',
    'DEV_MENU_SHOWN',
    'DID_PRESS_MENU_ITEM': [item: DevMenuItem],
    'OPEN_DEBUGGER': [onError: () => void],
    'TOGGLE_PERF_MONITOR',
    'STOP_PERF_MONITOR': [rnInstanceId: number];
  }>();

  /**
   * @brief Logger instance used to record internal DevTools-related logs.
   */
  private logger: RNOHLogger;

  /**
   * @brief The last recorded RNOH error.
   */
  protected lastError: RNOHError | null = null;

  /**
   * @brief Constructs a DevToolsController.
   * @param logger - Logger instance for logging internal events.
   */
  constructor(logger: RNOHLogger) {
    this.logger = logger.clone('DevMenu');
  }

  /**
   * @brief Triggers reload of the current RNInstance.
   * @param reason - Optional reason for the reload.
   */
  reload(reason: string | undefined = undefined): void {
    this.logger.info('Reload triggered' + (reason ? ': ' + reason : ''));
    this.eventEmitter.emit('RELOAD', { reason });
  }

  /**
   * @brief Toggles the element inspector overlay.
   */
  toggleElementInspector(): void {
    this.eventEmitter.emit('TOGGLE_ELEMENT_INSPECTOR');
  }

  /**
   * @brief Emits an event indicating that the dev menu was shown.
   */
  emitDevMenuShown(): void {
    this.eventEmitter.emit('DEV_MENU_SHOWN');
  }

  /**
   * @brief Emits an event when a dev menu item is selected.
   * @param title — title Title of the selected menu item.
   */
  emitDidPressMenuItem(title: string): void {
    this.eventEmitter.emit('DID_PRESS_MENU_ITEM', { title });
  }

  /**
   * @brief Emits an event to dismiss the RNOH error dialog.
   */
  dismissRNOHErrorDialog(): void {
    this.eventEmitter.emit('DISMISS_RNOH_ERROR_DIALOG');
  }

  togglePerfMonitor(): void {
    this.eventEmitter.emit('TOGGLE_PERF_MONITOR');
  }

  /**
   * @brief Opens React Native DevTools.
   * Logs an error if it fails to open.
   */
  openDebugger(): void {
    this.eventEmitter.emit(
      'OPEN_DEBUGGER',
      () => this.logger.error('Failed to open DevTools. Please check that the dev server is running and reload the app.')
    );
  }

  /**
   * @brief Returns the last error recorded by the system.
   * @return The last RNOHError or null if none.
   */
  getLastError(): RNOHError | null {
    return this.lastError;
  }

  /**
   * @brief Sets the last error encountered.
   * Intended for internal use only.
   * @param error - The error to record.
   * @internal
   */
  setLastError(error: RNOHError): void {
    this.lastError = error;
  }

}

export class InternalDevToolsController extends DevToolsController {
  public inspectorEventEmitter = new EventEmitter<{
    'OPEN_DEBUGGER_PAUSED_DIALOG': [payload: PauseInDebuggerDialogPayload],
    'CLOSE_DEBUGGER_PAUSED_DIALOG';
  }>();

  public processInspectorMessage(type: string, payload: any): void {
    if (type === 'RNOH::ON_RELOAD') {
      this.reload();
    }
    else if (type === 'RNOH::OPEN_DEBUGGER_PAUSED_DIALOG') {
      this.inspectorEventEmitter.emit('OPEN_DEBUGGER_PAUSED_DIALOG', payload);
    }
    else if (type === 'RNOH::CLOSE_DEBUGGER_PAUSED_DIALOG') {
      this.inspectorEventEmitter.emit('CLOSE_DEBUGGER_PAUSED_DIALOG');
    }
  }
}
