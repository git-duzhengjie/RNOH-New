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
 * Bridge message input (without auto-generated fields)
 */
export interface RNHBridgeMessageInput {
  type: RNHBridgeMessageType;
  moduleName: string;
  methodName: string;
  args: Array<Object>;
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
  enqueue(message: RNHBridgeMessageInput): number {
    const callId = this.nextCallId++;
    this.queue.push({
      type: message.type,
      moduleName: message.moduleName,
      methodName: message.methodName,
      args: message.args,
      callId: callId,
      timestamp: Date.now(),
    });
    return callId;
  }

  /**
   * Dequeue all messages
   */
  dequeueAll(): Array<RNHBridgeMessage> {
    const messages = this.queue.concat([]);
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
