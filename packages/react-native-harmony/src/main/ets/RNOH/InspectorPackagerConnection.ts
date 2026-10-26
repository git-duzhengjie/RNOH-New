/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import webSocket from '@ohos.net.webSocket';
import { RNOHLogger } from './RNOHLogger';
import { BusinessError } from '@ohos.base';
import { NapiBridge } from './NapiBridge';
import {
  InspectorPackagerConnection,
  InspectorWebsocket,
  InspectorWebsocketDelegate,
  InspectorPackagerConnectionDelegate
} from './types';

export function getInspectorPackagerConnection(napiBridge: NapiBridge, url: string, app: string, logger: RNOHLogger): InspectorPackagerConnection {
  return napiBridge.getInspectorPackagerConnection(url, app, new ArkTSConnectionDelegate(logger));
}

// Implementations
class ArkTSWebsocket implements InspectorWebsocket {
  private socket: webSocket.WebSocket;
  private logger: RNOHLogger;
  private messageBuffer: string[] = [];
  private isOpen: boolean = false;

  constructor(private url: string, private websocketDelegate: InspectorWebsocketDelegate, logger: RNOHLogger) {
    this.logger = logger.clone('InspectorWebsocket');
    if (this.url.startsWith('http')) {
      this.url = this.url.replace(/^http/, 'ws');
    }
    this.connect();
  }

  public connect() {
    const socket = webSocket.createWebSocket();
    socket.on('open', (_err) => this.onOpen());
    socket.on('message', (_err, message) => this.onMessage(message));
    socket.on('error', (err) => this.onError(err));
    socket.on('close', (_err, closeResult) => this.onClose(closeResult));

    socket.connect(this.url, (err) => {
      if (err) {
        this.onError(err);
      }
    });
    this.socket = socket;
  }

  // WebSocket handlers:
  private onOpen() {
    this.isOpen = true;
    const bufferedMessages = this.messageBuffer;
    this.messageBuffer = [];
    for (const message of bufferedMessages) {
      this.send(message);
    }
  }

  private onMessage(message: string | ArrayBuffer) {
    if (typeof message !== 'string') {
      this.logger.warn('Unrecognized inspector message');
      return;
    }

    this.websocketDelegate.didReceiveMessage(message);
  }

  private onClose(_closeResult: webSocket.CloseResult) {
    this.websocketDelegate.didClose();
    this.socket = undefined;
  }

  private onError(err: BusinessError) {
    this.websocketDelegate.didFailWithError(null, err.message ?? '<unknown error>');
  }

  // InspectorWebsocket implementation:
  send(message: string): void {
    if (!this.isOpen) {
      this.messageBuffer.push(message);
      return;
    }
    this.socket.send(message, (err) => {
      if (err) {
        this.onError(err);
      }
    });
  }

  close(): void {
    if (this.socket) {
      this.socket.close();
    }
  }
}

class ArkTSConnectionDelegate implements InspectorPackagerConnectionDelegate {
  private logger: RNOHLogger;
  constructor( logger: RNOHLogger) {
    this.logger = logger.clone('InspectorPackagerConnectionDelegate');
  }

  connectWebSocket(url: string, websocketDelegate: InspectorWebsocketDelegate): InspectorWebsocket {
    return new ArkTSWebsocket(url, websocketDelegate, this.logger);
  }
}

