/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

export type Point = {
  x: number,
  y: number,
};

export type Position = {
  x: number,
  y: number,
};

export type BoundingBox = {
  left: number,
  right: number,
  top: number,
  bottom: number,
};

export interface Size {
  width: number;
  height: number;
}

export type Edges<T> = {
  top: T,
  left: T,
  right: T,
  bottom: T;
};

export type Corners<T> = {
  topLeft: T,
  topRight: T,
  bottomLeft: T,
  bottomRight: T,
};

// The minimum supported SDK API version for getMaxFontSize is 13
export const OH_API_LEVEL_13 = 13;
// The minimum supported SDK API version for getWindowDensityInfo is 15
export const OH_API_LEVEL_15 = 15;

export type PointerEvents = 'auto' | 'none' | 'box-none' | 'box-only';

export type DisplayMetrics = {
  windowPhysicalPixels: PhysicalPixels,
  screenPhysicalPixels: PhysicalPixels,
};

export type PhysicalPixels = {
  top?: number,
  left?: number,
  width: number,
  height: number,
  decorHeight?: number,
  scale: number,
  fontScale: number,
  densityDpi: number,
  xDpi: number,
  yDpi: number;
};

// interfaces defined in "jsinspector-modern/WebSocketInterfaces.h"
export interface InspectorWebsocketDelegate {
  didFailWithError(posixCode: number | null, error: string): void;

  didReceiveMessage(message: string): void;

  didClose(): void;
}

export interface InspectorWebsocket {
  send(message: string): void;
  close(): void;
}

// interfaces defined in "jsinspector-modern/InspectorPackagerConnection.h"
export interface InspectorPackagerConnection {
  isConnected(): boolean,
  connect(): void,
  closeQuietly(): void,
  sendEventToAllConnections(event: string): void,
}

export interface InspectorPackagerConnectionDelegate {
  connectWebSocket(url: string, websocketDelegate: InspectorWebsocketDelegate): InspectorWebsocket;
  // void scheduleCallback implemented in CPP
}

export type JSEngineName = 'hermes' | 'jsvm';