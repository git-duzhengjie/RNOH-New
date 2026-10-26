/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

export interface RNOHMarkerEventPayload {
  markerId: string;
  tag: string;
  timestamp: number;
}

/**
 * @actor RNOH_LIBRARY
 * Implement this interface to register callback when RNOH markers are logged
 */
export interface RNOHMarkerListener {
  /**
   * This callback is triggered when an RNOH marker is logged
   *
   * @param markerId See RNOHMarkerId
   * @param tag The tag provided to logMarker
   * @param timestamp Only the difference between two timestamps is meaningful.
   * Unit is ms
   */
  logMarker(markerId: RNOHMarkerId, tag: string, timestamp: number): void;
}

/**
 * @actor RNOH_LIBRARY
 * Important events happening throughout an RN app's lifecycle
 * @note to framework developers. Keep this enum in sync with the one defined
 * in RNOHMarker.h
 */
export enum RNOHMarkerId {
  UNKNOWN_REACT_MARKER,
  APP_STARTUP_START,
  APP_STARTUP_STOP,
  INIT_REACT_RUNTIME_START,
  INIT_REACT_RUNTIME_STOP,
  NATIVE_REQUIRE_START,
  NATIVE_REQUIRE_STOP,
  RUN_JS_BUNDLE_START,
  RUN_JS_BUNDLE_STOP,
  CREATE_REACT_CONTEXT_START,
  CREATE_REACT_CONTEXT_STOP,
  JS_BUNDLE_STRING_CONVERT_START,
  JS_BUNDLE_STRING_CONVERT_STOP,
  NATIVE_MODULE_SETUP_START,
  NATIVE_MODULE_SETUP_STOP,
  REGISTER_JS_SEGMENT_START,
  REGISTER_JS_SEGMENT_STOP,
  REACT_INSTANCE_INIT_START,
  REACT_INSTANCE_INIT_STOP,
  CONTENT_APPEARED,
  DOWNLOAD_START,
  DOWNLOAD_END,
  REACT_BRIDGE_LOADING_START,
  REACT_BRIDGE_LOADING_END,
  ON_HOST_RESUME_START,
  ON_HOST_RESUME_END,
  ON_HOST_PAUSE_START,
  ON_HOST_PAUSE_END,
  CHANGE_THREAD_PRIORITY,
  INITIALIZE_MODULE_START,
  INITIALIZE_MODULE_END,
  PROCESS_PACKAGES_START,
  PROCESS_PACKAGES_END,
  CREATE_UI_MANAGER_MODULE_START,
  CREATE_UI_MANAGER_MODULE_END,
  CREATE_MODULE_START,
  CREATE_MODULE_END,
  PROCESS_CORE_REACT_PACKAGE_START,
  PROCESS_CORE_REACT_PACKAGE_END,
  FABRIC_COMMIT_START,
  FABRIC_COMMIT_END,
  FABRIC_FINISH_TRANSACTION_START,
  FABRIC_FINISH_TRANSACTION_END,
  FABRIC_DIFF_START,
  FABRIC_DIFF_END,
  FABRIC_LAYOUT_START,
  FABRIC_LAYOUT_END,
  FABRIC_BATCH_EXECUTION_START,
  FABRIC_BATCH_EXECUTION_END,
  FABRIC_UPDATE_UI_MAIN_THREAD_START,
  FABRIC_UPDATE_UI_MAIN_THREAD_END,
}

/**
 * @actor RNOH_LIBRARY
 * Registers listeners to receive RNOH marker events
 */
export class RNOHMarker {
  private static listeners: Set<RNOHMarkerListener> = new Set();

  /**
   * Registers a listener to receive RNOH marker events
   * @param listener
   */
  static addListener(listener: RNOHMarkerListener) {
    this.listeners.add(listener);
  }

  /**
   * Unregisters a listener to stop receiving RNOH marker events
   * @param listener
   */
  static removeListener(listener: RNOHMarkerListener) {
    this.listeners.delete(listener);
  }

  /**
   * @internal
   */
  static notifyListeners(markerId: string, tag: string, timestamp: number) {
    const resolvedMarkerId =
      RNOHMarkerId[markerId as keyof typeof RNOHMarkerId] ||
      RNOHMarkerId.UNKNOWN_REACT_MARKER;
    this.listeners.forEach(listener =>
      listener.logMarker(resolvedMarkerId, tag, timestamp),
    );
  }
}
