/**
 * RNOH-New: RNHWorker - Background worker for JS execution
 *
 * This worker runs the React Native JS bundle in a separate thread,
 * communicating with the main thread via message passing.
 */

import { worker, ThreadWorkerGlobalScope, MessageEvents, ErrorEvent } from '@kit.ArkTS';
import { RNHLogger } from '../RNH/RNHLogger';

const TAG = 'RNHWorker';

const workerPort: ThreadWorkerGlobalScope = worker.workerPort;

/**
 * Message types between main thread and worker
 */
enum RNHWorkerMessageType {
  // Main -> Worker
  INIT = 'init',
  LOAD_BUNDLE = 'loadBundle',
  LOAD_BUNDLE_FROM_URL = 'loadBundleFromUrl',
  EXECUTE_JS = 'executeJS',
  CALL_FUNCTION = 'callFunction',
  INVOKE_CALLBACK = 'invokeCallback',
  DESTROY = 'destroy',

  // Worker -> Main
  READY = 'ready',
  BUNDLE_LOADED = 'bundleLoaded',
  JS_RESULT = 'jsResult',
  JS_ERROR = 'jsError',
  EVENT = 'event',
  LOG = 'log',
}

/**
 * Initialize the RNH worker
 */
function setupRNHWorker(): void {
  RNHLogger.info(TAG, 'Setting up RNH worker...');

  workerPort.onmessage = (e: MessageEvents): void => {
    const { type, payload } = JSON.parse(e.data as string) as {
      type: string;
      payload: Record<string, Object>;
    };

    switch (type) {
      case RNHWorkerMessageType.INIT:
        handleInit(payload);
        break;
      case RNHWorkerMessageType.LOAD_BUNDLE:
        handleLoadBundle(payload);
        break;
      case RNHWorkerMessageType.LOAD_BUNDLE_FROM_URL:
        handleLoadBundleFromUrl(payload);
        break;
      case RNHWorkerMessageType.EXECUTE_JS:
        handleExecuteJS(payload);
        break;
      case RNHWorkerMessageType.CALL_FUNCTION:
        handleCallFunction(payload);
        break;
      case RNHWorkerMessageType.INVOKE_CALLBACK:
        handleInvokeCallback(payload);
        break;
      case RNHWorkerMessageType.DESTROY:
        handleDestroy();
        break;
      default:
        RNHLogger.warn(TAG, `Unknown message type: ${type}`);
    }
  };

  workerPort.onerror = (e: ErrorEvent): void => {
    RNHLogger.error(TAG, `Worker error: ${e.message}`);
  };

  // Notify main thread that worker is ready
  postMessage(RNHWorkerMessageType.READY, {});
  RNHLogger.info(TAG, 'RNH worker is ready');
}

/**
 * Handle init message
 */
function handleInit(payload: Record<string, Object>): void {
  RNHLogger.info(TAG, 'Initializing RNH worker runtime...');
  // TODO: Initialize JS VM, load RN framework
}

/**
 * Handle load bundle message
 */
function handleLoadBundle(payload: Record<string, Object>): void {
  const path = payload['path'] as string;
  RNHLogger.info(TAG, `Loading bundle: ${path}`);
  // TODO: Load and execute JS bundle
  postMessage(RNHWorkerMessageType.BUNDLE_LOADED, { path });
}

/**
 * Handle load bundle from URL message
 */
function handleLoadBundleFromUrl(payload: Record<string, Object>): void {
  const url = payload['url'] as string;
  RNHLogger.info(TAG, `Loading bundle from URL: ${url}`);
  // TODO: Fetch and execute JS bundle
  postMessage(RNHWorkerMessageType.BUNDLE_LOADED, { url });
}

/**
 * Handle execute JS message
 */
function handleExecuteJS(payload: Record<string, Object>): void {
  const script = payload['script'] as string;
  RNHLogger.debug(TAG, `Executing JS: ${script.substring(0, 100)}...`);
  // TODO: Execute JS in the VM
}

/**
 * Handle call function message
 */
function handleCallFunction(payload: Record<string, Object>): void {
  const module = payload['module'] as string;
  const method = payload['method'] as string;
  const args = payload['args'] as Array<Object>;
  RNHLogger.debug(TAG, `Calling ${module}.${method}`);
  // TODO: Call JS function
}

/**
 * Handle invoke callback message
 */
function handleInvokeCallback(payload: Record<string, Object>): void {
  const callbackId = payload['callbackId'] as number;
  const args = payload['args'] as Array<Object>;
  RNHLogger.debug(TAG, `Invoking callback ${callbackId}`);
  // TODO: Invoke JS callback
}

/**
 * Handle destroy message
 */
function handleDestroy(): void {
  RNHLogger.info(TAG, 'Destroying RNH worker...');
  // TODO: Cleanup JS VM
  workerPort.close();
}

/**
 * Post a message to the main thread
 */
function postMessage(type: string, payload: Record<string, Object>): void {
  workerPort.postMessage(JSON.stringify({ type, payload }));
}

// Initialize the worker
setupRNHWorker();
