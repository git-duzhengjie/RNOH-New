/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import http from '@ohos.net.http';
import util from '@ohos.util';

import {
  AnyThreadTurboModule,
  WorkerTurboModule,
} from '../../../RNOH/TurboModule';
import {NetworkEventsDispatcher} from './NetworkEventDispatcher';
import {InspectorNetworkReporter} from './InspectorNetworkReporter';
import ArrayList from '@ohos.util.ArrayList';
import {BlobMetadata} from '../Blob';
import {
  CancelRequestCallback,
  HttpErrorResponse,
  ReceivingProgress,
  SendingProgress,
} from '../../../HttpClient/types';

type ResponseType = 'base64' | 'blob' | 'text';

interface Query {
  method: string;
  url: string;
  data: Object;
  headers: Object;
  responseType: ResponseType;
  incrementalUpdates: boolean;
  timeout: number;
  withCredentials: boolean;
  unstable_devToolsRequestId?: string;
}

export type UriHandler = {
  supports: (query: Query) => boolean;
  fetch: (query: Query) => Promise<Object>;
};

export type JsFormPart = {
  fieldName: string;
  headers: object;
  name: string;
  uri: string;
  string?: string;
};

export type JsFormData = {
  formData: JsFormPart[];
};

export type RequestBodyHandler = {
  supports: (data: Object) => boolean;
  handleRequest: (
    data: {blob: BlobMetadata},
    contentType?: string,
  ) => {
    body: string | ArrayBuffer | Object;
    contentType?: string;
  };
};

export type ResponseBodyHandler = {
  supports: (responseType: ResponseType) => boolean;
  handleResponse: (response: string | Object | ArrayBuffer) => BlobMetadata;
};

/**
 * This is a TurboModule for handling Networking requests. It encodes the data it gets from JS
 * (a JSON object with different fields) into a Harmony specific format.
 * It also exposes handlers which can be used to customize how requests are handled.
 */

export class NetworkingTurboModule extends AnyThreadTurboModule {
  public static readonly NAME = 'Networking';
  private networkEventDispatcher: NetworkEventsDispatcher =
    new NetworkEventsDispatcher((eventName, params) => {
      this.ctx.rnInstance.emitDeviceEvent(eventName, params);
    });
  private base64Helper: util.Base64Helper = new util.Base64Helper();
  private utf8TextEncoder: util.TextEncoder = new util.TextEncoder('utf-8');
  private uriHandlers: ArrayList<UriHandler> = new ArrayList();
  private requestCancellersById: Map<number, CancelRequestCallback> = new Map();
  private nextId: number = 0;
  private requestBodyHandlers: ArrayList<RequestBodyHandler> = new ArrayList();
  private responseBodyHandlers: ArrayList<ResponseBodyHandler> =
    new ArrayList();

  private static REQUEST_METHOD_BY_NAME: Record<
    string,
    http.RequestMethod
  > | null = null;

  private static getRequestMethodMap(): Record<string, http.RequestMethod> {
    if (NetworkingTurboModule.REQUEST_METHOD_BY_NAME === null) {
      NetworkingTurboModule.REQUEST_METHOD_BY_NAME =
        NetworkingTurboModule.buildRequestMethodMap();
    }
    return NetworkingTurboModule.REQUEST_METHOD_BY_NAME;
  }

  private static buildRequestMethodMap(): Record<string, http.RequestMethod> {
    const methods: Record<string, http.RequestMethod> = {
      OPTIONS: http.RequestMethod.OPTIONS,
      GET: http.RequestMethod.GET,
      HEAD: http.RequestMethod.HEAD,
      POST: http.RequestMethod.POST,
      PUT: http.RequestMethod.PUT,
      DELETE: http.RequestMethod.DELETE,
      TRACE: http.RequestMethod.TRACE,
      CONNECT: http.RequestMethod.CONNECT,
    };

    try {
      // @ts-ignore - PATCH may not exist on older API levels. It requires API 26+.
      const patchMethod = http.RequestMethod['PATCH'];
      if (patchMethod !== undefined) {
        methods.PATCH = patchMethod;
      } else {
        console.warn(
          'PATCH method is not supported on this device (requires API 26+)',
        );
      }
    } catch (e) {
      console.warn(
        'PATCH method is not supported on this device (requires API 26+)',
      );
    }

    return methods;
  }

  addUriHandler(handler: UriHandler) {
    this.uriHandlers.add(handler);
  }

  removeUriHandler(handler: UriHandler) {
    this.uriHandlers.remove(handler);
  }

  addRequestBodyHandler(handler: RequestBodyHandler) {
    this.requestBodyHandlers.add(handler);
  }

  removeRequestBodyHandler(handler: RequestBodyHandler) {
    this.requestBodyHandlers.remove(handler);
  }

  addResponseBodyHandler(handler: ResponseBodyHandler) {
    this.responseBodyHandlers.add(handler);
  }

  removeResponseBodyHandler(handler: ResponseBodyHandler) {
    this.responseBodyHandlers.remove(handler);
  }

  decodeBufferToText(buf: ArrayBuffer): string {
    const textDecoder = util.TextDecoder.create();
    const byteArray = new Uint8Array(buf);
    return textDecoder.decodeWithStream(byteArray, {stream: false});
  }

  async encodeResponse(
    response: string | Object | ArrayBuffer,
    responseType: ResponseType,
  ): Promise<string | Object> {
    if (responseType === 'text') {
      if (typeof response === 'string') {
        return response;
      } else if (response instanceof ArrayBuffer) {
        return this.decodeBufferToText(response);
      } else {
        // NOTE: Object responses have been long deprecated in Ark, we don't expect them here
        throw new Error('INTERNAL: unexpected Object http response');
      }
    } else if (responseType === 'base64') {
      let byteArray: Uint8Array;
      if (typeof response === 'string') {
        const textEncoder = new util.TextEncoder();
        byteArray = textEncoder.encodeInto(response);
      } else if (response instanceof ArrayBuffer) {
        byteArray = new Uint8Array(response);
      } else {
        throw new Error('INTERNAL: unexpected Object http response');
      }
      return this.base64Helper.encodeToString(byteArray);
    }

    throw new Error('Unsupported query response type');
  }

  private encodeBody(data: Object): string | ArrayBuffer | Object | undefined {
    if ('trackingName' in data) {
      delete data.trackingName;
    }
    if ('string' in data) {
      return data.string as string;
    }
    if ('base64' in data) {
      const base64 = data.base64 as string;
      const textEncoder = new util.TextEncoder();
      const byteArray = textEncoder.encodeInto(base64);
      return byteArray.buffer;
    }
    if (Object.keys(data).length === 0) {
      return undefined;
    }
    return data;
  }

  private extractDevToolsRequestId(query: Query): string | undefined {
    const queryAsRecord = query as unknown as Record<string, unknown>;
    if ('unstable_devToolsRequestId' in queryAsRecord) {
      const value = queryAsRecord.unstable_devToolsRequestId;
      if (typeof value === 'string' && value.length > 0) {
        return value;
      }
    }
    if (query.data && typeof query.data === 'object') {
      const dataAsRecord = query.data as Record<string, unknown>;
      if ('devToolsRequestId' in dataAsRecord) {
        const value = dataAsRecord.devToolsRequestId;
        if (typeof value === 'string' && value.length > 0) {
          return value;
        }
      }
    }
    return undefined;
  }

  private stripDevToolsRequestIdFromData<T>(data: T): T {
    if (!data || typeof data !== 'object') {
      return data;
    }
    const record = data as unknown as Record<string, unknown>;
    if (!('devToolsRequestId' in record)) {
      return data;
    }
    const {devToolsRequestId: _ignored, ...rest} = record;
    return rest as unknown as T;
  }

  private utf8ByteLength(value: string | undefined | null): number {
    if (!value) {
      return 0;
    }
    try {
      return this.utf8TextEncoder.encodeInto(value).length;
    } catch (_) {
      return 0;
    }
  }

  private parseContentLength(headers: Object | null | undefined): number {
    const normalizedHeaders = this.normalizeHeaders(headers);
    for (const key of Object.keys(normalizedHeaders)) {
      if (key.toLowerCase() === 'content-length') {
        const raw = normalizedHeaders[key];
        const num = Number(raw);
        return Number.isFinite(num) && num > 0 ? num : 0;
      }
    }
    return 0;
  }

  private normalizeHeaders(
    headers: Object | null | undefined,
  ): Record<string, string> {
    if (!headers || typeof headers !== 'object') {
      return {};
    }
    const result: Record<string, string> = {};
    const headersAsRecord = headers as Record<string, unknown>;
    for (const key of Object.keys(headersAsRecord)) {
      const normalizedKey = key.toLowerCase();
      const raw = headersAsRecord[key];
      if (typeof raw === 'string') {
        result[normalizedKey] = raw;
        continue;
      }
      if (typeof raw === 'number' || typeof raw === 'boolean') {
        result[normalizedKey] = String(raw);
        continue;
      }
      if (Array.isArray(raw)) {
        const normalizedValues = raw
          .map(value => {
            if (typeof value === 'string') {
              return value;
            }
            if (typeof value === 'number' || typeof value === 'boolean') {
              return String(value);
            }
            return '';
          })
          .filter(value => value.length > 0);
        if (normalizedValues.length > 0) {
          result[normalizedKey] = normalizedValues.join(', ');
        }
      }
    }
    return result;
  }

  private async toBase64FromArrayBuffer(buf: ArrayBuffer): Promise<string> {
    return await this.base64Helper.encodeToString(new Uint8Array(buf));
  }

  private encodeFormData(jsFormData: JsFormData): Array<http.MultiFormData> {
    const formData = jsFormData.formData.map(
      (jsFormPart: JsFormPart): http.MultiFormData => {
        const contentType = jsFormPart.headers['content-type'] ?? 'text/plain';
        return {
          name: jsFormPart.fieldName,
          contentType: contentType,
          data: jsFormPart.string,
          filePath: jsFormPart.uri,
          remoteFileName: jsFormPart.name,
        };
      },
    );
    return formData;
  }

  async clearCookies(callback: (didDeleteAnyCookies: boolean) => void) {
    const didDeleteAnyCookies = await this.ctx.httpClient.clearCookies();
    callback(didDeleteAnyCookies);
  }

  private isEncodedURI(str: string): boolean {
    try {
      const decodedStr = decodeURI(str);
      return decodedStr !== str;
    } catch (e) {
      return false;
    }
  }

  private getEncodedURI(str: string): string {
    if (this.isEncodedURI(str)) {
      return str;
    }
    return str.replace(/[\u4e00-\u9fa5]/g, char => encodeURIComponent(char));
  }

  async sendRequest(
    query: Query,
    onRequestRegistered: (requestId: number) => void,
  ) {
    const httpClient = this.ctx.httpClient;
    const caPath = this.ctx.caPathProvider?.(query.url);
    const requestId = this.createId();
    onRequestRegistered(requestId);

    const devToolsRequestId = this.extractDevToolsRequestId(query);
    const queryDataForRequest = this.stripDevToolsRequestIdFromData(query.data);
    InspectorNetworkReporter.setLogger(this.ctx.logger);
    const requestHeadersForReporter = this.normalizeHeaders(query.headers);

    for (const handler of this.uriHandlers) {
      if (handler.supports(query)) {
        if (devToolsRequestId) {
          InspectorNetworkReporter.reportRequestStart(
            devToolsRequestId,
            query.url,
            query.method,
            requestHeadersForReporter,
            '',
            0,
          );
          InspectorNetworkReporter.reportConnectionTiming(
            devToolsRequestId,
            requestHeadersForReporter,
          );
        }
        try {
          const response = await handler.fetch(query);
          this.networkEventDispatcher.dispatchDidReceiveNetworkResponse(
            requestId,
            200,
            {},
            query.url,
          );
          this.networkEventDispatcher.dispatchDidReceiveNetworkData(
            requestId,
            response,
          );
          this.networkEventDispatcher.dispatchDidCompleteNetworkResponse(
            requestId,
          );
          if (devToolsRequestId) {
            InspectorNetworkReporter.reportResponseStart(
              devToolsRequestId,
              query.url,
              200,
              {},
              0,
            );
            if (typeof response === 'string') {
              InspectorNetworkReporter.maybeStoreResponseBody(
                devToolsRequestId,
                response,
                false,
              );
              InspectorNetworkReporter.reportResponseEnd(
                devToolsRequestId,
                this.utf8ByteLength(response),
              );
            } else {
              InspectorNetworkReporter.reportResponseEnd(devToolsRequestId, 0);
            }
          }
        } catch (error) {
          this.networkEventDispatcher.dispatchDidReceiveNetworkResponse(
            requestId,
            400,
            {},
            query.url,
          );
          this.networkEventDispatcher.dispatchDidCompleteNetworkResponseWithError(
            requestId,
            error.toString(),
            false,
          );
          if (devToolsRequestId) {
            InspectorNetworkReporter.reportResponseStart(
              devToolsRequestId,
              query.url,
              400,
              {},
              0,
            );
            InspectorNetworkReporter.reportRequestFailed(
              devToolsRequestId,
              false,
            );
          }
        }

        return;
      }
    }

    const method = NetworkingTurboModule.getRequestMethodMap()[query.method];
    if (method === undefined) {
      if (devToolsRequestId) {
        InspectorNetworkReporter.reportRequestStart(
          devToolsRequestId,
          query.url,
          query.method,
          requestHeadersForReporter,
          '',
          0,
        );
        InspectorNetworkReporter.reportConnectionTiming(
          devToolsRequestId,
          requestHeadersForReporter,
        );
        InspectorNetworkReporter.reportResponseStart(
          devToolsRequestId,
          query.url,
          501,
          {},
          0,
        );
        // reportRequestFailed implies the request is terminated; no need for reportResponseEnd
        InspectorNetworkReporter.reportRequestFailed(devToolsRequestId, false);
      }
      this.networkEventDispatcher.dispatchDidReceiveNetworkResponse(
        requestId,
        501,
        {},
        query.url,
      );
      this.networkEventDispatcher.dispatchDidCompleteNetworkResponseWithError(
        requestId,
        `Unsupported HTTP method: "${query.method}"`,
        false,
      );
      return;
    }

    let requestBodyHandler: RequestBodyHandler | null = null;
    if (queryDataForRequest) {
      for (const handler of this.requestBodyHandlers) {
        if (handler.supports(queryDataForRequest)) {
          requestBodyHandler = handler;
          break;
        }
      }
    }

    let extraData: unknown = null;
    const headers: Record<string, string> = Object.assign(
      {},
      query.headers as Record<string, string>,
    );
    let multiFormDataList: Array<http.MultiFormData> = [];

    // Capture a request body preview before encodeBody() mutates query.data.
    // Only string bodies are forwarded to the inspector; binary / form-data
    // bodies are reported as empty preview with byte length set conservatively.
    let inspectorRequestBody = '';
    let inspectorRequestEncodedSize = 0;
    if (
      devToolsRequestId &&
      queryDataForRequest &&
      typeof queryDataForRequest === 'object' &&
      'string' in queryDataForRequest &&
      typeof (queryDataForRequest as Record<string, unknown>).string ===
        'string'
    ) {
      const stringBody = (queryDataForRequest as Record<string, string>).string;
      inspectorRequestBody = stringBody;
      inspectorRequestEncodedSize = this.utf8ByteLength(stringBody);
    }

    if (requestBodyHandler != null) {
      const requestData = requestBodyHandler.handleRequest(
        queryDataForRequest as {blob: BlobMetadata},
        query.responseType,
      );
      extraData = requestData.body;
      if (requestData.contentType !== undefined) {
        headers['Content-Type'] = requestData.contentType;
      }
    } else if (
      queryDataForRequest &&
      typeof queryDataForRequest === 'object' &&
      'formData' in queryDataForRequest
    ) {
      headers['Content-Type'] = 'multipart/form-data';
      multiFormDataList = this.encodeFormData(
        queryDataForRequest as JsFormData,
      );
    } else {
      extraData = this.encodeBody(queryDataForRequest);
    }

    if (devToolsRequestId) {
      const requestHeadersAfterMutation = this.normalizeHeaders(headers);
      InspectorNetworkReporter.reportRequestStart(
        devToolsRequestId,
        query.url,
        query.method,
        requestHeadersAfterMutation,
        inspectorRequestBody,
        inspectorRequestEncodedSize,
      );
      InspectorNetworkReporter.reportConnectionTiming(
        devToolsRequestId,
        requestHeadersAfterMutation,
      );
    }

    let onReceiveProgress:
      | ((receiveProgress: ReceivingProgress) => void)
      | null = null;

    if (query.incrementalUpdates) {
      if (query.responseType === 'text') {
        const textDecoder = util.TextDecoder.create('utf-8');
        onReceiveProgress = (receiveProgress: ReceivingProgress) => {
          const uintArray = new Uint8Array(receiveProgress.bitsReceived);
          const decodedStr = textDecoder.decodeWithStream(uintArray, {
            stream: true,
          }); //we want to carry over bytes from incomplete characters
          this.networkEventDispatcher.dispatchDidReceiveNetworkIncrementalData(
            requestId,
            decodedStr,
            receiveProgress.lengthReceived,
            receiveProgress.totalLength,
          );
          if (devToolsRequestId) {
            InspectorNetworkReporter.reportDataReceived(
              devToolsRequestId,
              decodedStr,
            );
            InspectorNetworkReporter.maybeStoreResponseBodyIncremental(
              devToolsRequestId,
              decodedStr,
            );
          }
        };
      } else {
        onReceiveProgress = (partialProgress: ReceivingProgress) => {
          this.networkEventDispatcher.dispatchDidReceiveNetworkDataProgress(
            requestId,
            partialProgress.lengthReceived,
            partialProgress.totalLength,
          );
        };
      }
    }

    let onSendProgress = (sendProgress: SendingProgress) => {
      this.networkEventDispatcher.dispatchDidSendNetworkData(
        requestId,
        sendProgress.lengthSent,
        sendProgress.totalLength,
      );
    };

    const {cancel, promise} = httpClient.sendRequest(
      this.getEncodedURI(query.url),
      {
        method: method,
        header: headers,
        extraData: extraData,
        timeout: query.timeout,
        readTimeout: query.timeout,
        connectTimeout: query.timeout,
        multiFormDataList: multiFormDataList,
        onReceiveProgress: onReceiveProgress,
        onSendProgress: onSendProgress,
        handleCookies: query.withCredentials,
        ...(caPath ? {caPath} : {}),
      },
    );
    this.requestCancellersById.set(requestId, cancel);

    promise
      .then(async httpResponse => {
        const responseUrlForReporter =
          typeof (httpResponse as {responseUrl?: unknown}).responseUrl ===
            'string' &&
          (httpResponse as {responseUrl: string}).responseUrl.length > 0
            ? (httpResponse as {responseUrl: string}).responseUrl
            : query.url;
        const responseHeaders: Object = httpResponse.headers ?? {};
        this.networkEventDispatcher.dispatchDidReceiveNetworkResponse(
          requestId,
          httpResponse.statusCode,
          responseHeaders,
          responseUrlForReporter,
        );
        if (devToolsRequestId) {
          const responseHeadersForReporter =
            this.normalizeHeaders(responseHeaders);
          InspectorNetworkReporter.reportResponseStart(
            devToolsRequestId,
            responseUrlForReporter,
            httpResponse.statusCode,
            responseHeadersForReporter,
            this.parseContentLength(responseHeaders),
          );
        }
        if (this.requestCancellersById.has(requestId)) {
          this.requestCancellersById.delete(requestId);
        }
        for (const handler of this.responseBodyHandlers) {
          if (handler.supports(query.responseType)) {
            this.networkEventDispatcher.dispatchDidReceiveNetworkData(
              requestId,
              handler.handleResponse(httpResponse.body),
            );
            this.networkEventDispatcher.dispatchDidCompleteNetworkResponse(
              requestId,
            );
            if (devToolsRequestId) {
              const skipInspectorFullBodyStore =
                query.incrementalUpdates && query.responseType === 'text';
              if (!skipInspectorFullBodyStore) {
                if (httpResponse.body instanceof ArrayBuffer) {
                  InspectorNetworkReporter.maybeStoreResponseBody(
                    devToolsRequestId,
                    await this.toBase64FromArrayBuffer(httpResponse.body),
                    true,
                  );
                } else if (typeof httpResponse.body === 'string') {
                  InspectorNetworkReporter.maybeStoreResponseBody(
                    devToolsRequestId,
                    httpResponse.body,
                    false,
                  );
                }
              } else if (
                typeof httpResponse.body === 'string' &&
                httpResponse.body.length === 0
              ) {
                InspectorNetworkReporter.maybeStoreResponseBody(
                  devToolsRequestId,
                  '',
                  false,
                );
              }
              InspectorNetworkReporter.reportResponseEnd(
                devToolsRequestId,
                this.parseContentLength(responseHeaders),
              );
            }
            return;
          }
        }
        const encodedResponse = await this.encodeResponse(
          httpResponse.body,
          query.responseType,
        );
        this.networkEventDispatcher.dispatchDidReceiveNetworkData(
          requestId,
          encodedResponse,
        );
        this.networkEventDispatcher.dispatchDidCompleteNetworkResponse(
          requestId,
        );
        if (devToolsRequestId) {
          // Keep a transport body available for DevTools in all paths.
          const skipInspectorFullBodyStore =
            query.incrementalUpdates && query.responseType === 'text';
          if (!skipInspectorFullBodyStore) {
            if (typeof encodedResponse === 'string') {
              InspectorNetworkReporter.maybeStoreResponseBody(
                devToolsRequestId,
                encodedResponse,
                query.responseType === 'base64',
              );
            } else if (httpResponse.body instanceof ArrayBuffer) {
              InspectorNetworkReporter.maybeStoreResponseBody(
                devToolsRequestId,
                await this.toBase64FromArrayBuffer(httpResponse.body),
                true,
              );
            }
          } else if (
            typeof encodedResponse === 'string' &&
            encodedResponse.length === 0
          ) {
            InspectorNetworkReporter.maybeStoreResponseBody(
              devToolsRequestId,
              '',
              query.responseType === 'base64',
            );
          }
          const reportedSize =
            typeof encodedResponse === 'string'
              ? this.utf8ByteLength(encodedResponse)
              : httpResponse.body instanceof ArrayBuffer
                ? httpResponse.body.byteLength
                : 0;
          InspectorNetworkReporter.reportResponseEnd(
            devToolsRequestId,
            reportedSize || this.parseContentLength(responseHeaders),
          );
        }
      })
      .catch((errorResponse: HttpErrorResponse) => {
        this.networkEventDispatcher.dispatchDidReceiveNetworkResponse(
          requestId,
          errorResponse.statusCode || 0,
          {},
          query.url,
        );
        this.networkEventDispatcher.dispatchDidCompleteNetworkResponseWithError(
          requestId,
          errorResponse.error.toString(),
          errorResponse.timeout ?? false,
        );
        if (devToolsRequestId) {
          InspectorNetworkReporter.reportRequestFailed(
            devToolsRequestId,
            errorResponse.timeout ?? false,
          );
        }
        if (this.requestCancellersById.has(requestId)) {
          this.requestCancellersById.delete(requestId);
        }
      });
  }

  abortRequest(requestId: number) {
    const cancel = this.requestCancellersById.get(requestId);
    if (cancel) {
      cancel();
      this.requestCancellersById.delete(requestId);
    }
  }

  private createId(): number {
    return this.nextId++;
  }

  public __onDestroy__() {
    for (const [requestId, cancel] of this.requestCancellersById.entries()) {
      try {
        cancel();
      } catch (e) {
        // ignore cancellation errors during destroy
      }
      this.requestCancellersById.delete(requestId);
    }
    super.__onDestroy__();
  }
}
