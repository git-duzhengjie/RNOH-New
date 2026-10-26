/**
 * RNOH-New: NetworkingTurboModule - Networking native module
 *
 * Implements React Native's Networking interface using @ohos.net.http.
 */

import { RNHTurboModule, RNHTurboModuleContext } from '../../RNH/RNHTurboModule';
import { RNHLogger } from '../../RNH/RNHLogger';
import { toError } from '../../RNH/RNHErrorUtils';
import http from '@ohos.net.http';
import util from '@ohos.util';

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
 * Network response data wrapper
 */
interface NetworkResponse {
  requestId: number;
  status: number;
  headers: Map<string, string>;
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
 * Request progress event
 */
interface RequestProgressEvent {
  requestId: number;
  progress: number;
  total: number;
}

/**
 * Response received event
 */
interface ResponseReceivedEvent {
  requestId: number;
  status: number;
  headers: Map<string, string>;
}

/**
 * Data received event
 */
interface DataReceivedEvent {
  requestId: number;
  response: string;
}

/**
 * Response completed event
 */
interface ResponseCompletedEvent {
  requestId: number;
  error: string | null;
}

/**
 * Networking TurboModule
 */
export class NetworkingTurboModule extends RNHTurboModule {
  private requestIdCounter: number = 1;
  private activeRequests: Map<number, RequestTask> = new Map();
  private base64Helper: util.Base64Helper = new util.Base64Helper();
  private textDecoder: util.TextDecoder = new util.TextDecoder('utf-8');

  constructor(context: RNHTurboModuleContext) {
    super(context);
  }

  override getName(): string {
    return 'Networking';
  }

  /**
   * Send an HTTP request
   */
  async sendRequest(method: string, url: string, headers: Map<string, string>,
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
      if (!withCredentials) {
        // HarmonyOS HTTP client handles cookies automatically by default.
        // There is no public API to disable credentials on http.HttpRequest.
        RNHLogger.debug(TAG, `[${requestId}] withCredentials ignored on HarmonyOS`);
      }

      const dataLength = typeof data === 'string' ? data.length : (data instanceof ArrayBuffer ? data.byteLength : 0);
      const sendEvent: RequestProgressEvent = {
        requestId,
        progress: 0,
        total: dataLength,
      };
      this.emitEvent('didSendNetworkData', JSON.stringify(sendEvent));

      // Convert Map<string,string> headers to Record-like object for http.request
      const headerObj: Record<string, string> = {};
      for (const pair of headers) {
        const key = pair[0];
        const value = pair[1];
        headerObj[key] = value;
      }
      headerObj['Content-Type'] = headerObj['Content-Type'] ?? 'application/octet-stream';

      const response = await httpRequest.request(url, {
        method: method as http.RequestMethod,
        header: headerObj,
        extraData: data ?? undefined,
        expectDataType: this.mapResponseType(responseType),
        usingCache: true,
        priority: 1,
        connectTimeout: timeout,
        readTimeout: timeout,
      });

      const responseHeaders = this.normalizeHeaders(response.header);

      const receivedEvent: ResponseReceivedEvent = {
        requestId,
        status: response.responseCode,
        headers: responseHeaders,
      };
      this.emitEvent('didReceiveNetworkResponse', JSON.stringify(receivedEvent));

      let responseBody: string | ArrayBuffer;
      if (responseType === 'base64') {
        if (typeof response.result === 'string') {
          responseBody = response.result;
        } else {
          responseBody = this.arrayBufferToBase64(response.result as ArrayBuffer);
        }
      } else if (responseType === 'blob') {
        responseBody = response.result as ArrayBuffer;
      } else {
        if (typeof response.result === 'string') {
          responseBody = response.result;
        } else {
          responseBody = this.arrayBufferToText(response.result as ArrayBuffer);
        }
      }

      const dataEvent: DataReceivedEvent = {
        requestId,
        response: typeof responseBody === 'string' ? responseBody : this.arrayBufferToBase64(responseBody),
      };
      this.emitEvent('didReceiveNetworkData', JSON.stringify(dataEvent));

      const completedEvent: ResponseCompletedEvent = {
        requestId,
        error: null,
      };
      this.emitEvent('didCompleteNetworkResponse', JSON.stringify(completedEvent));

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
      const completedEvent: ResponseCompletedEvent = {
        requestId,
        error: String(e),
      };
      this.emitEvent('didCompleteNetworkResponse', JSON.stringify(completedEvent));
      this.activeRequests.delete(requestId);
      httpRequest.destroy();
      throw toError(e);
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
   * Normalize response headers to a string map
   */
  private normalizeHeaders(headers: Object): Map<string, string> {
    const result: Map<string, string> = new Map();
    const raw = headers as Record<string, string | string[]>;
    for (const key of Object.keys(raw)) {
      const value = raw[key];
      const normalized = Array.isArray(value) ? value.join(', ') : value;
      result.set(key, normalized);
    }
    return result;
  }

  /**
   * Convert ArrayBuffer to base64 string
   */
  private arrayBufferToBase64(buffer: ArrayBuffer): string {
    return this.base64Helper.encodeToStringSync(new Uint8Array(buffer));
  }

  /**
   * Convert ArrayBuffer to text string
   */
  private arrayBufferToText(buffer: ArrayBuffer): string {
    return this.textDecoder.decodeToString(new Uint8Array(buffer));
  }

  override initialize(): void {
    super.initialize();
    RNHLogger.info(TAG, 'NetworkingTurboModule initialized');
  }

  override invalidate(): void {
    RNHLogger.info(TAG, 'NetworkingTurboModule invalidated');
    for (const entry of this.activeRequests) {
      const task = entry[1];
      const requestId = entry[0];
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
