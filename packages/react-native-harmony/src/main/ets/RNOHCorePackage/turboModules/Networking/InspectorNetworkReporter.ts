/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// @ts-ignore
import libRNOHApp from 'librnoh_app.so';
import util from '@ohos.util';
import type {RNOHLogger} from '../../../RNOH/RNOHLogger';

export class InspectorNetworkReporter {
  private static logger: RNOHLogger | null = null;
  private static debuggingEnabledCache: boolean | null = null;
  private static debuggingEnabledCacheExpiryMs = 0;
  private static readonly DEBUGGING_ENABLED_CACHE_TTL_MS = 5000;

  static setLogger(logger: RNOHLogger): void {
    if (this.logger) {
      return;
    }
    this.logger = logger.clone('InspectorNetworkReporter');
  }

  static isDebuggingEnabled(): boolean {
    const now = Date.now();
    if (
      this.debuggingEnabledCache !== null &&
      now < this.debuggingEnabledCacheExpiryMs
    ) {
      return this.debuggingEnabledCache;
    }
    try {
      const enabled =
        libRNOHApp?.inspectorReporterIsDebuggingEnabled() ?? false;
      this.debuggingEnabledCache = enabled;
      this.debuggingEnabledCacheExpiryMs =
        now + this.DEBUGGING_ENABLED_CACHE_TTL_MS;
      return enabled;
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.isDebuggingEnabled', e);
      this.debuggingEnabledCache = false;
      this.debuggingEnabledCacheExpiryMs =
        now + this.DEBUGGING_ENABLED_CACHE_TTL_MS;
      return false;
    }
  }

  static reportRequestStart(
    requestId: string,
    requestUrl: string,
    requestMethod: string,
    requestHeaders: Record<string, string>,
    requestBody: string,
    encodedDataLength: number,
  ): void {
    try {
      libRNOHApp?.inspectorReporterReportRequestStart(
        requestId,
        requestUrl,
        requestMethod,
        requestHeaders,
        requestBody,
        encodedDataLength,
      );
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.reportRequestStart', e);
    }
  }

  static reportConnectionTiming(
    requestId: string,
    headers: Record<string, string>,
  ): void {
    try {
      libRNOHApp?.inspectorReporterReportConnectionTiming(requestId, headers);
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.reportConnectionTiming', e);
    }
  }

  static reportResponseStart(
    requestId: string,
    requestUrl: string,
    responseStatus: number,
    responseHeaders: Record<string, string>,
    expectedDataLength: number,
  ): void {
    try {
      libRNOHApp?.inspectorReporterReportResponseStart(
        requestId,
        requestUrl,
        responseStatus,
        responseHeaders,
        expectedDataLength,
      );
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.reportResponseStart', e);
    }
  }

  static reportDataReceived(requestId: string, data: string): void {
    if (!this.isDebuggingEnabled()) {
      return;
    }
    try {
      const dataLength = computeUtf8ByteLength(data);
      libRNOHApp?.inspectorReporterReportDataReceived(requestId, dataLength);
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.reportDataReceived', e);
    }
  }

  static reportResponseEnd(requestId: string, encodedDataLength: number): void {
    try {
      libRNOHApp?.inspectorReporterReportResponseEnd(
        requestId,
        encodedDataLength,
      );
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.reportResponseEnd', e);
    }
  }

  static reportRequestFailed(requestId: string, cancelled: boolean): void {
    try {
      libRNOHApp?.inspectorReporterReportRequestFailed(requestId, cancelled);
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.reportRequestFailed', e);
    }
  }

  static maybeStoreResponseBody(
    requestId: string,
    body: string,
    base64Encoded: boolean,
  ): void {
    if (!this.isDebuggingEnabled()) {
      return;
    }
    try {
      libRNOHApp?.inspectorReporterMaybeStoreResponseBody(
        requestId,
        body,
        base64Encoded,
      );
    } catch (e) {
      this.logger?.warn('InspectorNetworkReporter.maybeStoreResponseBody', e);
    }
  }

  static maybeStoreResponseBodyIncremental(
    requestId: string,
    data: string,
  ): void {
    if (!this.isDebuggingEnabled()) {
      return;
    }
    try {
      libRNOHApp?.inspectorReporterMaybeStoreResponseBodyIncremental(
        requestId,
        data,
      );
    } catch (e) {
      this.logger?.warn(
        'InspectorNetworkReporter.maybeStoreResponseBodyIncremental',
        e,
      );
    }
  }
}

const utf8Encoder = new util.TextEncoder('utf-8');

function computeUtf8ByteLength(value: string): number {
  if (!value || value.length === 0) {
    return 0;
  }
  try {
    return utf8Encoder.encodeInto(value).length;
  } catch (_) {
    return 0;
  }
}
