/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import webSocket from '@ohos.net.webSocket';
import util from '@ohos.util';
import { RNOHLogger, AnyThreadTurboModule } from '../../RNOH/ts';
import { BusinessError } from '@ohos.base';
import { BlobMetadata as BlobMetadata } from './Blob';
import { AnyThreadTurboModuleContext } from '../../RNOH/RNOHContext';

const WEB_SOCKET_SUPPORTED_EVENT_NAMES =
  ['websocketOpen', 'websocketClosed', 'websocketFailed', 'websocketMessage'] as const;

/**
 * WebSocket heartbeat (ping/pong) configuration. Values are forwarded as-is to the
 * system `WebSocketRequestOptions`. This allows disabling or customizing the heartbeat
 * behavior when servers do not respond to pings.
 *
 * NOTE: `pingInterval` and `pongTimeout` require API 21+. On older systems they are ignored.
 */
export type WebSocketModuleOptions = {
  /**
   * Heartbeat interval. Every `pingInterval` a ping is sent. Set to `0` to disable the
   * heartbeat. Requires API 21+.
   */
  pingInterval?: number;
  /**
   * Timeout for waiting a pong after a ping is sent. If no response arrives within `pongTimeout`,
   * the connection is closed. Must be less than or equal to `pingInterval`.
   */
  pongTimeout?: number;
};

type MessageParams = {
  id: number,
  type: 'text' | 'binary' | 'blob',
  data?: string | BlobMetadata,
};

export type ContentHandler = {
  processMessage: (params: MessageParams) => MessageParams;
  processByteMessage: (bytes: ArrayBuffer, params: MessageParams) => MessageParams;
};

export class WebSocketTurboModule extends AnyThreadTurboModule {
  public static readonly NAME = 'WebSocketModule';

  private socketById: Map<number, webSocket.WebSocket> = new Map();
  private logger: RNOHLogger;
  private base64 = new util.Base64Helper();
  private contentHandlerBySocketId: Map<number, ContentHandler> = new Map();

  constructor(ctx: AnyThreadTurboModuleContext) {
    super(ctx);
    this.logger = ctx.logger.clone('WebSocketTurboModule');
  }

  getSupportedEvents() {
    return WEB_SOCKET_SUPPORTED_EVENT_NAMES;
  }

  setContentHandler(socketID: number, contentHandler?: ContentHandler) {
    if (contentHandler) {
      this.contentHandlerBySocketId.set(socketID, contentHandler);
    } else {
      this.contentHandlerBySocketId.delete(socketID);
    }
  }

  private onMessage(err: BusinessError<void>, data: string | ArrayBuffer, socketID: number) {
    const contentHandler = this.contentHandlerBySocketId.get(socketID);

    if (typeof data === 'string') {
      let params: MessageParams = { id: socketID, type: 'text', data: data };
      if (contentHandler) {
        params = contentHandler.processMessage(params);
      }

      this.ctx.rnInstance.emitDeviceEvent('websocketMessage', params);
    } else if (data instanceof ArrayBuffer) {
      const base64Data = this.base64.encodeToStringSync(new Uint8Array(data));

      let params: MessageParams = { id: socketID, type: 'binary', data: base64Data };

      if (contentHandler) {
        params = contentHandler.processByteMessage(data, params);
      }

      this.ctx.rnInstance.emitDeviceEvent('websocketMessage', params);
    }
  }

  connect(url: string, protocols: string[] | undefined | null, options: { headers?: object }, socketID: number) {
    this.logger.clone('connect').debug(url);
    const ws = webSocket.createWebSocket();

    // `ws.connect` fails for `http:`-scheme urls,
    // so we do this instead
    if (url.startsWith('http')) {
      url = url.replace('http', 'ws');
    }

    ws.on('open', (data) => {
      this.ctx.rnInstance.emitDeviceEvent('websocketOpen', {
        id: socketID,
        protocol: '',
      });
    });

    ws.on('error', (err) => this.maybeHandleError(socketID, err));
    ws.on('message', (err, data) => this.onMessage(err, data, socketID));
    ws.on('close', (err, data) => {
      this.ctx.rnInstance.emitDeviceEvent('websocketClosed', {
        id: socketID,
        ...data,
      });

    });
    const wsOptions: webSocket.WebSocketRequestOptions = {};
    if (options.headers) {
      wsOptions.header = options.headers;
    }
    const { pingInterval, pongTimeout } = this.resolveConnectHeartbeat(
      options.headers,
      this.ctx.rnInstance.getWebSocketOptions?.(),
    );
    if (protocols && protocols.length > 0) {
      wsOptions.protocol = protocols.join(',');
    }
    if (pingInterval !== undefined) {
      wsOptions.pingInterval = pingInterval;
    }
    if (pongTimeout !== undefined) {
      wsOptions.pongTimeout = pongTimeout;
    }
    ws.connect(url, wsOptions,
      (err) => this.maybeHandleError(socketID, err));
    this.socketById.set(socketID, ws);
  }

  private resolveConnectHeartbeat(
    headers: object | undefined,
    global?: WebSocketModuleOptions,
  ): { pingInterval?: number; pongTimeout?: number } {
    let pingInterval: number | undefined = global?.pingInterval;
    let pongTimeout: number | undefined = global?.pongTimeout;

    if (headers) {
      const source = headers as Record<string, unknown>;
      if ('pingInterval' in source || 'pongTimeout' in source) {
        pingInterval = this.toNumberSafe(source.pingInterval);
        pongTimeout = this.toNumberSafe(source.pongTimeout);
      }
    }
    return { pingInterval, pongTimeout };
  }

  private toNumberSafe(value: unknown): number | undefined {
    if (value === null || value === undefined || value === '') {
      return undefined;
    }
    const num = Number(value);
    return isNaN(num) ? undefined : num;
  }

  send(message: string, socketId: number) {
    const ws = this.getSocketById(socketId, 'send');
    if (!ws) {
      return;
    }
    ws.send(message, (err) => this.maybeHandleError(socketId, err));
  }

  sendBinary(base64Message: string, socketId: number) {
    const ws = this.getSocketById(socketId, 'sendBinary');
    if (!ws) {
      return;
    }
    const message = this.base64.decodeSync(base64Message);
    ws.send(message.buffer, (err) => this.maybeHandleError(socketId, err));
  }

  sendBinaryArray(message: Uint8Array, socketId: number) {
    const ws = this.getSocketById(socketId, 'sendBinaryArray');
    if (!ws) {
      return;
    }
    ws.send(message.buffer, (err) => this.maybeHandleError(socketId, err));
  }

  ping(socketId: number) {
    const ws = this.getSocketById(socketId, 'ping');
    if (!ws) {
      return;
    }
    this.ctx.logger.warn('WebSocketTurboModule::ping not implemented');
  }

  close(code: number, reason: string, socketId: number) {
    const ws = this.getSocketById(socketId, 'close');
    if (!ws) {
      return;
    }

    ws.close({ code, reason }, (err) => this.maybeHandleError(socketId, err));
    this.socketById.delete(socketId);
    this.contentHandlerBySocketId.delete(socketId);
  }

  private maybeHandleError(socketId: number, err: unknown | undefined): boolean {
    const logger = this.logger.clone('maybeHandleError');
    if (err) {
      if (typeof err === 'object' && 'code' in err && err?.code === 201) {
        logger.error('Permission denied. Check if \'ohos.permission.INTERNET\' has been granted');
      }
      let stringifiedErr = '';
      if (err instanceof Error) {
        stringifiedErr = err.message;
      } else {
        stringifiedErr = JSON.stringify(err);
      }
      logger.error(JSON.stringify(err));
      this.ctx.rnInstance.emitDeviceEvent('websocketFailed', { id: socketId, message: stringifiedErr });
      this.socketById.delete(socketId);
      this.contentHandlerBySocketId.delete(socketId);
      return true;
    }
    return false;
  }

  private getSocketById(socketId: number, methodName: string) {
    const ws = this.socketById.get(socketId);
    if (!ws) {
      this.maybeHandleError(socketId,
        new Error(`Tried to get websocket with ID "${socketId}", but such socket hasn't been created or it's closed. (WebSocketTurboModule::${methodName})`));
      return null;
    }
    return ws;
  }
}
