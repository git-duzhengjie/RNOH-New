/**
 * RNOH-New: RNHBridge - Bridge message types and utilities
 */

/**
 * Bridge message types
 */
export enum RNHBridgeMessageType {
  // JS -> Native
  CALL_NATIVE_MODULE = 'callNativeModule',
  CALL_NATIVE_METHOD = 'callNativeMethod',

  // Native -> JS
  CALL_JS_MODULE = 'callJSModule',
  CALL_JS_METHOD = 'callJSMethod',
  INVOKE_CALLBACK = 'invokeCallback',
  EMIT_EVENT = 'emitEvent',
}

/**
 * Bridge message structure
 */
export interface RNHBridgeMessage {
  type: RNHBridgeMessageType;
  moduleName: string;
  methodName: string;
  args: Array<Object>;
  callId: number;
  timestamp: number;
}

/**
 * Bridge message queue
 */
export class RNHBridgeMessageQueue {
  private queue: Array<RNHBridgeMessage> = [];
  private nextCallId: number = 0;

  /**
   * Enqueue a message
   */
  enqueue(message: Omit<RNHBridgeMessage, 'callId' | 'timestamp'>): number {
    const callId = this.nextCallId++;
    this.queue.push({
      ...message,
      callId,
      timestamp: Date.now(),
    });
    return callId;
  }

  /**
   * Dequeue all messages
   */
  dequeueAll(): Array<RNHBridgeMessage> {
    const messages = [...this.queue];
    this.queue = [];
    return messages;
  }

  /**
   * Get queue size
   */
  size(): number {
    return this.queue.length;
  }

  /**
   * Clear the queue
   */
  clear(): void {
    this.queue = [];
  }
}
