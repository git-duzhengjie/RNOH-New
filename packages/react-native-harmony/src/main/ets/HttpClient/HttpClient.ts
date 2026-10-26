/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import http from '@ohos.net.http';
import url from '@ohos.url';
import {
  CancelRequestCallback,
  HttpErrorResponse,
  HttpResponse,
  RequestInterceptor,
  RequestOptions,
  ResponseInterceptor
} from './types';
import { webview } from '@kit.ArkWeb';
import type { RNOHLogger } from '../RNOH/RNOHLogger';
import { InspectorNetworkReporter } from '../RNOHCorePackage/turboModules/Networking/InspectorNetworkReporter';

export interface HttpClient {
  addResponseInterceptor(interceptor: ResponseInterceptor);

  addRequestInterceptor(interceptor: RequestInterceptor)

  sendRequest(url: string, requestOptions: RequestOptions): {
    cancel: CancelRequestCallback,
    promise: Promise<HttpResponse>;
  },

  clearCookies(): Promise<boolean>;
}

export type CAPathProvider = (url: string) => string;

function mergeObjects<T>(baseObject: Partial<T>, overridingObject: Partial<T>): T {
  const mergedObject: Partial<Record<keyof T, any>> = {};

  const allKeys = new Set([
    ...Object.keys(baseObject),
    ...Object.keys(overridingObject)
  ]);

  allKeys.forEach((value: string) => {
    const key = value as keyof T;
    mergedObject[key] = overridingObject[key] !== null && overridingObject[key] !== undefined
      ? overridingObject[key]
      : baseObject[key];
  });

  return mergedObject as T;
}


export class DefaultHttpClient implements HttpClient {
  protected responseInterceptors: ResponseInterceptor[];
  protected requestInterceptors: RequestInterceptor[];
  protected baseRequestOptions: RequestOptions;
  private logger: RNOHLogger;

  constructor({ baseRequestOptions, responseInterceptors, requestInterceptors, logger }: {
    baseRequestOptions?: RequestOptions;
    responseInterceptors?: ResponseInterceptor[],
    requestInterceptors?: RequestInterceptor[],
    logger?: RNOHLogger,
  } = {}) {
    this.baseRequestOptions = baseRequestOptions ?? {};
    this.responseInterceptors = responseInterceptors ?? [];
    this.requestInterceptors = requestInterceptors ?? [];
    this.logger = logger?.clone('DefaultHttpClient') ?? undefined;
  }


  private async fetchCookies(url: string,): Promise<string> {
    const cookies = await webview.WebCookieManager.fetchCookie(url);
    return cookies;
  }

  private setCookiesFromHeaders(url: string, headers: Object) {
    const cookieHeaders = headers['set-cookie'] as Array<string>;
    if (!cookieHeaders) {
      return;
    }
    //each cookie has to be set separately because of a bug which happens when setting multiple cookies at once
    for (const cookie of cookieHeaders) {
      try {
        webview.WebCookieManager.configCookieSync(url, cookie);
      } catch (err) {
        this.logger?.error('ConfigCookieSync Error, Cookie: ' + JSON.stringify(cookie));
      }
    }
    webview.WebCookieManager.saveCookieAsync();
  }

  async clearCookies(): Promise<boolean> {
    const didDeleteAnyCookies = webview.WebCookieManager.existCookie();
    await webview.WebCookieManager.clearAllCookies();
    return didDeleteAnyCookies;
  }

  addResponseInterceptor(interceptor: ResponseInterceptor) {
    this.responseInterceptors.push(interceptor);
  }

  addRequestInterceptor(interceptor: RequestInterceptor) {
    this.requestInterceptors.push(interceptor);
  }


  sendRequest(url: string, requestOptions: RequestOptions): {
    cancel: CancelRequestCallback,
    promise: Promise<HttpResponse>;
  } {
    const httpRequest = http.createHttp();
    let timeoutId: number | null = null;

    let headersReceiveHandler: ((data: Object) => void) | undefined;
    let dataReceiveHandler: ((chunk: ArrayBuffer) => void) | undefined;
    let dataSendProgressHandler: ((progress: any) => void) | undefined;
    let dataEndHandler: (() => void) | undefined;

    const cleanup = () => {
      cancelled = true;
      try {
        if (headersReceiveHandler) {
          httpRequest.off('headersReceive', headersReceiveHandler);
          headersReceiveHandler = undefined;
        }
        if (dataReceiveHandler) {
          httpRequest.off('dataReceive', dataReceiveHandler);
          dataReceiveHandler = undefined;
        }
        if (dataSendProgressHandler) {
          httpRequest.off('dataSendProgress', dataSendProgressHandler);
          dataSendProgressHandler = undefined;
        }
        if (dataEndHandler) {
          httpRequest.off('dataEnd', dataEndHandler);
          dataEndHandler = undefined;
        }
      } catch (err) {
        this.logger?.warn('Failed to remove event listeners: ' + err);
      }
      httpRequest.destroy();
      if (timeoutId !== null) {
        clearTimeout(timeoutId);
        timeoutId = null;
      }
    };
    let rejectRef: ((reason: HttpErrorResponse) => void) | null = null;
    let timedOut = false;
    let cancelled = false;
    const promise: Promise<HttpResponse> = (async (): Promise<HttpResponse> => {
      let finalRequestOptions: RequestOptions = mergeObjects(this.baseRequestOptions, requestOptions);
      if (finalRequestOptions.timeout) {
        timeoutId = setTimeout(() => {
          timedOut = true;
          this.logger?.warn(`Request timeout after ${finalRequestOptions.timeout}ms for '${url}'`);
          rejectRef?.({ timeout: true, statusCode: 0, error: Error('Request timeout.') });
          cleanup();
        }, finalRequestOptions.timeout);
      }
      for (const interceptor of this.requestInterceptors) {
        if (interceptor.shouldIntercept(url, finalRequestOptions)) {
          ({ url, requestOptions } = interceptor.intercept(url, finalRequestOptions));
          finalRequestOptions = requestOptions;
        }
      }
      if (requestOptions.handleCookies) {
        try {
          const cookies = await this.fetchCookies(url);
          const cookiesForHeader = cookies.replace(/,/g, ';');
          if (requestOptions.header['cookie']) {
            requestOptions.header['cookie'] += '; ' + cookiesForHeader;
          } else {
            requestOptions.header['cookie'] = cookiesForHeader;
          }
        } catch (err) {
          cleanup();
          throw {
            timeout: false,
            statusCode: 0,
            error: err instanceof Error ? err : new Error(String(err)),
          } as HttpErrorResponse;
        }
      }

      return await new Promise<HttpResponse>((resolve, reject) => {
        rejectRef = reject;

        let headers: Object | undefined;
        let resultBody: ArrayBuffer;
        let responseCode: number;
        let effectiveUrl: string = url;

        let currentLength = 0;
        let totalLength = null;
        const dataChunks: ArrayBuffer[] = [];
        let headersReceiveCount = 0;

        const maybeResolve = () => {
          if (resultBody !== undefined && headers !== undefined && responseCode !== undefined) {
            let response: HttpResponse = {
              statusCode: responseCode,
              headers: headers,
              body: resultBody,
              responseUrl: effectiveUrl,
            };
            for (const interceptor of this.responseInterceptors) {
              if (interceptor.shouldIntercept(url, response)) {
                response = interceptor.intercept(url, response);
              }
            }
            this.logger?.debug(`HTTP ${responseCode} for '${url}' (headersReceiveCount=${headersReceiveCount})`);
            resolve(response);
            cleanup();
          }
        };

        if (timedOut) {
          reject({ timeout: true, statusCode: 0, error: Error('Request timeout.') });
          return;
        }
        if (cancelled) {
          reject({ timeout: false, statusCode: 0, error: Error('Request aborted.') });
          return;
        }

        headersReceiveHandler = (data) => {
        headers = data;
        totalLength = headers['content-length'] ?? -1;
        headersReceiveCount++;
        this.logger?.debug(`headersReceive #${headersReceiveCount} content-length=${totalLength}`);
        this.setCookiesFromHeaders(effectiveUrl, headers);

        let location: unknown = undefined;
        try {
          // eslint-disable-next-line dot-notation
          location = headers?.['location'] ?? headers?.['Location'];
          if (typeof location === 'string' && location.length > 0) {
            effectiveUrl = resolveRedirectUrl(location, effectiveUrl);
          }
        } catch (e) {
          if (InspectorNetworkReporter.isDebuggingEnabled()) {
            this.logger?.warn(
              `Failed to resolve redirect URL. location=${String(location)} from=${effectiveUrl}`,
              e
            );
          }
        }

        maybeResolve();
      };

      httpRequest.on('headersReceive', headersReceiveHandler);

      dataReceiveHandler = (chunk) => {
        dataChunks.push(chunk);
        currentLength += chunk.byteLength;
        if (requestOptions.onReceiveProgress) {
          requestOptions.onReceiveProgress({
            totalLength: totalLength,
            lengthReceived: currentLength,
            bitsReceived: chunk,
          });
        }
      };

      httpRequest.on('dataReceive', dataReceiveHandler);

      dataSendProgressHandler = (progress) => {
        if (requestOptions.onSendProgress) {
          requestOptions.onSendProgress(
            {
              totalLength: progress.totalSize,
              lengthSent: progress.sendSize
            }
          );
        }
      };

      httpRequest.on('dataSendProgress', dataSendProgressHandler);

      dataEndHandler = () => {
        const totalLength = dataChunks.map(chunk => chunk.byteLength).reduce((acc, length) => acc + length, 0);
        const data = new Uint8Array(totalLength);
        let offset = 0;
        for (const chunk of dataChunks) {
          const chunkArray = new Uint8Array(chunk);
          data.set(chunkArray, offset);
          offset += chunk.byteLength;
        }
        resultBody = data.buffer;
        this.logger?.debug(`dataEnd. bytesReceived=${currentLength}`);
        maybeResolve();
      };

      httpRequest.on('dataEnd', dataEndHandler);

      if (this.logger?.isDebugEnabled()) {
        const d = finalRequestOptions.extraData as any;
        const bodySize = d == null ? 0
          : typeof d === 'string' ? d.length
          : (typeof d === 'object' && typeof d.byteLength === 'number') ? d.byteLength
          : -1;
        this.logger.debug(`Request initiated. url="${url}" method=${finalRequestOptions.method ?? 'GET'} bodySize=${bodySize} timeout=${finalRequestOptions.timeout ?? 0}`);
      }
      try {
        httpRequest.requestInStream(
          url,
          finalRequestOptions,
          (err, data) => {
            responseCode = data;
            if (err) {
              if (err.code === 201) {
                this.logger?.clone('requestInStream').error('Permission denied. Check if \'ohos.permission.INTERNET\' has been granted');
              }
              cleanup();
              reject({
                error: err,
                statusCode: responseCode ?? 0
              } as HttpErrorResponse);
            } else {
              this.logger?.debug(`requestInStream completed. HTTP statusCode=${responseCode} headersReceiveCount=${headersReceiveCount}`);
              maybeResolve();
            }
          }
        );
      } catch (err) {
        cleanup();
        reject({
          error: err,
          statusCode: 0
        } as HttpErrorResponse);
      }
      });

    })();

    const cancel = cleanup;

    return { cancel, promise };
  }
}

function resolveRedirectUrl(location: string, baseUrl: string): string {
  try {
    return url.URL.parseURL(location, baseUrl).href;
  } catch {
    return location;
  }
}
