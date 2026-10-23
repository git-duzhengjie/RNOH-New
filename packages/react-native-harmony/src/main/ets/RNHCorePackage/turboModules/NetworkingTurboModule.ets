/**
 * RNOH-New: NetworkingTurboModule - Networking native module
 *
 * Implements React Native's Networking interface using @ohos.net.http.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import http from '@ohos.net.http';

const TAG = 'NetworkingTurboModule';

/**
 * Request method type
 */
type HttpMethod = 'GET' | 'POST' | 'PUT' | 'DELETE' | 'PATCH' | 'HEAD' | 'OPTIONS';

/**
 * Response type
 */
type ResponseType = 'text' | 'base64' | 'blob';

/**
 * Headers map
 */
interface HttpHeaders {
  [key: string]: string;
}

/**
 * Network response data wrapper
 */
interface NetworkResponse {
  requestId: number;
  status: number;
  headers: HttpHeaders;
  response: string | ArrayBuffer;
  error?: string;
}

/**
 * Request task wrapper
 */
interface RequestTask {
  httpRequest: http.HttpRequest;
  startTime: number;
}

/**
 * Networking TurboModule
 */
export class NetworkingTurboModule extends RNHTurboModule {
  private requestIdCounter: number = 1;
  private activeRequests: Map<number, RequestTask> = new Map();

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'Networking';
  }

  /**
   * Send an HTTP request
   */
  async sendRequest(method: string, url: string, headers: HttpHeaders,
                    data: string | ArrayBuffer | null, responseType: ResponseType,
                    timeout: number, withCredentials: boolean): Promise<NetworkResponse> {
    const requestId = this.requestIdCounter++;
    RNHLogger.info(TAG, `[${requestId}] ${method} ${url}`);

    const httpRequest = http.createHttp();
    const task: RequestTask = {
      httpRequest,
      startTime: Date.now(),
    };
    this.activeRequests.set(requestId, task);

    try {
      httpRequest.setRequest(method as HttpMethod);
      httpRequest.setUrl(url);
      httpRequest.setHeader('Content-Type', 'application/octet-stream');
      httpRequest.setConnectTimeout(timeout);
      httpRequest.setReadTimeout(timeout);

      for (const key of Object.keys(headers)) {
        httpRequest.setHeader(key, headers[key]);
      }

      if (!withCredentials) {
        // HarmonyOS HTTP client handles cookies automatically by default.
        // There is no public API to disable credentials on http.HttpRequest.
        RNHLogger.debug(TAG, `[${requestId}] withCredentials ignored on HarmonyOS`);
      }

      this.emitEvent('didSendNetworkData', JSON.stringify({
        requestId,
        progress: 0,
        total: typeof data === 'string' ? data.length : (data instanceof ArrayBuffer ? data.byteLength : 0),
      }));

      const response = await httpRequest.request(url, {
        method: method as http.RequestMethod,
        header: headers,
        extraData: data ?? undefined,
        expectDataType: this.mapResponseType(responseType),
        usingCache: true,
        priority: 1,
        connectTimeout: timeout,
        readTimeout: timeout,
      });

      const responseHeaders = this.normalizeHeaders(response.header);

      this.emitEvent('didReceiveNetworkResponse', JSON.stringify({
        requestId,
        status: response.responseCode,
        headers: responseHeaders,
      }));

      let responseBody: string | ArrayBuffer;
      if (responseType === 'base64') {
        responseBody = typeof response.result === 'string'
          ? response.result
          : this.arrayBufferToBase64(response.result as ArrayBuffer);
      } else if (responseType === 'blob') {
        responseBody = response.result as ArrayBuffer;
      } else {
        responseBody = typeof response.result === 'string'
          ? response.result
          : this.arrayBufferToText(response.result as ArrayBuffer);
      }

      this.emitEvent('didReceiveNetworkData', JSON.stringify({
        requestId,
        response: typeof responseBody === 'string' ? responseBody : this.arrayBufferToBase64(responseBody),
      }));

      this.emitEvent('didCompleteNetworkResponse', JSON.stringify({
        requestId,
        error: null,
      }));

      this.activeRequests.delete(requestId);
      httpRequest.destroy();

      return {
        requestId,
        status: response.responseCode,
        headers: responseHeaders,
        response: responseBody,
      };
    } catch (e) {
      RNHLogger.error(TAG, `[${requestId}] Request failed: ${e}`);
      this.emitEvent('didCompleteNetworkResponse', JSON.stringify({
        requestId,
        error: `${e}`,
      }));
      this.activeRequests.delete(requestId);
      httpRequest.destroy();
      throw e;
    }
  }

  /**
   * Abort an active request
   */
  abortRequest(requestId: number): void {
    const task = this.activeRequests.get(requestId);
    if (task) {
      RNHLogger.info(TAG, `[${requestId}] Aborting request`);
      try {
        task.httpRequest.destroy();
      } catch (e) {
        RNHLogger.error(TAG, `[${requestId}] Failed to abort request: ${e}`);
      }
      this.activeRequests.delete(requestId);
    } else {
      RNHLogger.warn(TAG, `[${requestId}] Request not found, cannot abort`);
    }
  }

  /**
   * Map response type to HarmonyOS expect data type
   */
  private mapResponseType(responseType: ResponseType): http.HttpDataType {
    switch (responseType) {
      case 'text':
        return http.HttpDataType.STRING;
      case 'base64':
        return http.HttpDataType.ARRAY_BUFFER;
      case 'blob':
        return http.HttpDataType.ARRAY_BUFFER;
      default:
        return http.HttpDataType.STRING;
    }
  }

  /**
   * Normalize response headers to a simple string map
   */
  private normalizeHeaders(headers: Object): HttpHeaders {
    const result: HttpHeaders = {};
    const raw = headers as Record<string, string | string[]>;
    for (const key of Object.keys(raw)) {
      const value = raw[key];
      result[key] = Array.isArray(value) ? value.join(', ') : value;
    }
    return result;
  }

  /**
   * Convert ArrayBuffer to base64 string
   */
  private arrayBufferToBase64(buffer: ArrayBuffer): string {
    const bytes = new Uint8Array(buffer);
    let binary = '';
    for (let i = 0; i < bytes.byteLength; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }

  /**
   * Convert ArrayBuffer to text string
   */
  private arrayBufferToText(buffer: ArrayBuffer): string {
    const decoder = new TextDecoder('utf-8');
    return decoder.decode(buffer);
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'NetworkingTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'NetworkingTurboModule invalidated');
    for (const [requestId, task] of this.activeRequests) {
      try {
        task.httpRequest.destroy();
      } catch (e) {
        RNHLogger.error(TAG, `[${requestId}] Error destroying request: ${e}`);
      }
    }
    this.activeRequests.clear();
    super.invalidate();
  }
}
