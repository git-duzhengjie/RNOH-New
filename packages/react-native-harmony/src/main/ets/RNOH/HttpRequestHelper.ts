/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

import http from '@ohos.net.http';
import {deviceInfo} from '@kit.BasicServicesKit';
import fs from '@ohos.file.fs';

/**
 * Determines whether the current device supports the remoteValidation field at the http layer, API >= 18.
 *
 * On API < 18 devices this field does not exist and will be ignored by the system; callers use this to decide whether to pass it.
 */
const API_LEVEL_18 = 18;
const API_LEVEL_26 = 26;

/**
 * Decides whether remoteValidation should be passed through to the http layer, based on value type and device API version:
 *   - undefined: do not pass (system default validation)
 *   - API < 18: field does not exist, do not pass
 *   - API 18-25: only 'system' | 'skip' (string) can be passed; callback is not passed
 *   - API >= 26: 'system' | 'skip' | ValidationCallback can all be passed
 *
 * Callbacks are silently dropped on API < 26 (falls back to system default validation) to avoid http layer errors.
 */
export function getRemoteValidation(
  remoteValidation: http.RemoteValidation | undefined,
  apiVersion: number = deviceInfo.sdkApiVersion,
): http.RemoteValidation | undefined {
  if (apiVersion >= API_LEVEL_26 && typeof remoteValidation === 'function') {
    return remoteValidation;
  }
  if (
    apiVersion >= API_LEVEL_18 &&
    (remoteValidation === 'system' || remoteValidation === 'skip')
  ) {
    return remoteValidation;
  }
  return undefined;
}

export type FetchOptions = {
  headers?: Record<string, any>;
  usingCache?: boolean;
  /**
   * http layer RemoteValidation value. Effective by API version:
   *   - API < 18: field does not exist, setting is ineffective (system default validation)
   *   - API 18-25: only 'system' | 'skip' are effective
   *   - API >= 26: 'system' | 'skip' | ValidationCallback are all effective
   */
  remoteValidation?: http.RemoteValidation;
};

export type FetchResult = {
  headers: Object;
  result: ArrayBuffer;
  responseCode?: number;
};

export type StreamFetchResult = {
  headers: Object;
  responseCode?: number;
};

/**
 * Request abort controller, used by external callers to cancel in-progress HTTP requests.
 */
export class FetchAbortController {
  private _aborted: boolean = false;
  private _onAbort: (() => void) | null = null;

  get aborted(): boolean {
    return this._aborted;
  }

  abort(): void {
    // Idempotent: if already aborted, return immediately to avoid re-triggering the onAbort callback (matches AbortController one-shot semantics).
    if (this._aborted) {
      return;
    }
    this._aborted = true;
    this._onAbort?.();
  }

  set onAbort(callback: (() => void) | null) {
    this._onAbort = callback;
    if (this._aborted && callback) {
      callback();
    }
  }

  get onAbort(): (() => void) | null {
    return this._onAbort;
  }
}

/**
 * Error thrown when a request is aborted, carrying a code field for caller identification.
 */
export class AbortRequestError extends Error {
  public code: string;

  constructor(message: string, code: string) {
    super(message);
    this.name = 'AbortRequestError';
    this.code = code;
  }
}

export async function fetchDataFromUrl(
  url: string,
  options: FetchOptions = {usingCache: true},
  onProgress?: (progress: number) => void,
): Promise<FetchResult> {
  return new Promise((resolve, reject) => {
    const httpRequest = http.createHttp();
    // If already aborted, clean up and return early to avoid unnecessary work
    const dataChunks: ArrayBuffer[] = [];
    let result: ArrayBuffer | undefined;
    let headers: Object | undefined;
    let responseCode: number | undefined;

    let dataReceiveProgressHandler:
      | ((progress: {receiveSize: number; totalSize: number}) => void)
      | undefined;
    let headersReceiveHandler: ((data: Object) => void) | undefined;
    let dataReceiveHandler: ((chunk: ArrayBuffer) => void) | undefined;
    let dataEndHandler: (() => void) | undefined;

    let cleanedUp = false;
    function cleanUp() {
      if (cleanedUp) {
        return;
      }
      cleanedUp = true;
      try {
        if (dataReceiveProgressHandler) {
          httpRequest.off('dataReceiveProgress', dataReceiveProgressHandler);
          dataReceiveProgressHandler = undefined;
        }
        if (headersReceiveHandler) {
          httpRequest.off('headersReceive', headersReceiveHandler);
          headersReceiveHandler = undefined;
        }
        if (dataReceiveHandler) {
          httpRequest.off('dataReceive', dataReceiveHandler);
          dataReceiveHandler = undefined;
        }
        if (dataEndHandler) {
          httpRequest.off('dataEnd', dataEndHandler);
          dataEndHandler = undefined;
        }
      } catch (err) {
        // eslint-disable-next-line no-console
        console.warn('Failed to remove httpRequest listeners: ' + err);
      }
      try {
        httpRequest.destroy();
      } catch (err) {
        console.warn('Failed to destroy httpRequest: ' + err);
      }
    }

    function maybeResolve() {
      if (
        result !== undefined &&
        headers !== undefined &&
        responseCode !== undefined
      ) {
        resolve({headers, result, responseCode});
        cleanUp();
      }
    }

    dataReceiveProgressHandler = ({receiveSize, totalSize}) => {
      onProgress?.(receiveSize / totalSize);
    };

    httpRequest.on('dataReceiveProgress', dataReceiveProgressHandler);

    headersReceiveHandler = data => {
      headers = data;
      maybeResolve();
    };

    httpRequest.on('headersReceive', headersReceiveHandler);

    dataReceiveHandler = chunk => {
      dataChunks.push(chunk);
    };

    httpRequest.on('dataReceive', dataReceiveHandler);

    dataEndHandler = () => {
      const totalLength = dataChunks
        .map(chunk => chunk.byteLength)
        .reduce((acc, length) => acc + length, 0);
      const data = new Uint8Array(totalLength);
      let offset = 0;
      for (const chunk of dataChunks) {
        const chunkArray = new Uint8Array(chunk);
        data.set(chunkArray, offset);
        offset += chunk.byteLength;
      }
      result = data.buffer;
      maybeResolve();
    };

    httpRequest.on('dataEnd', dataEndHandler);

    try {
      // Start with Record so the remoteValidation field can be attached even on low compileSdk
      // where HttpRequestOptions does not declare it; the structure matches HttpRequestOptions when passed to requestInStream.

      httpRequest.requestInStream(
        url,
        {
          header: options.headers,
          usingCache: options.usingCache,
          remoteValidation: getRemoteValidation(options.remoteValidation),
        },
        (err, data) => {
          responseCode = data;
          if (err) {
            cleanUp();
            reject(
              new Error(`Couldn't fetch data from ${url}, ${err.message}`),
            );
          } else {
            maybeResolve();
          }
        },
      );
    } catch (err) {
      cleanUp();
      reject(new Error(`Couldn't fetch data from ${url}, ${err.message}`));
    }
  });
}

// Maximum read timeout for streaming downloads: 7 days (604,800,000 ms).
// request.downloadFile has no configurable timeout, but streamDataFromUrl
// (HTTP Kit requestInStream) defaults to 60s which is too short for large
// files on slow networks.
const STREAM_READ_TIMEOUT = 604800000;

/**
 * Streams data from a URL directly to a file without keeping the entire response in memory.
 * Each received chunk is immediately written to the file via the provided write callback.
 * This avoids OOM issues when downloading large files.
 *
 * @param url - The URL to fetch
 * @param options - Fetch options
 * @param writeChunk - Callback invoked for each received chunk; should write the chunk to the target file
 * @param onProgress - Optional progress callback (0-1)
 * @param abortController - Optional abort controller
 * @returns Promise resolving to headers and response code once the download completes
 */
async function streamDataFromUrl(
  url: string,
  options: FetchOptions,
  writeChunk: (chunk: ArrayBuffer) => Promise<void> | void,
  onProgress?: (progress: number) => void,
  abortController?: FetchAbortController,
): Promise<StreamFetchResult> {
  return new Promise((resolve, reject) => {
    const httpRequest = http.createHttp();
    // If already aborted, clean up and return early to avoid unnecessary work
    if (abortController?.aborted) {
      httpRequest.destroy();
      reject(
        new AbortRequestError(
          `The download task has been removed.`,
          'E_PREFETCH_ABORT',
        ),
      );
      return;
    }

    let headers: Object | undefined;
    let responseCode: number | undefined;
    let writeError: Error | undefined;
    let writeChain: Promise<void> = Promise.resolve();

    let dataReceiveProgressHandler:
      | ((progress: {receiveSize: number; totalSize: number}) => void)
      | undefined;
    let headersReceiveHandler: ((data: Object) => void) | undefined;
    let dataReceiveHandler: ((chunk: ArrayBuffer) => void) | undefined;
    let dataEndHandler: (() => void) | undefined;

    let cleanedUp = false;
    function cleanUp() {
      if (cleanedUp) {
        return;
      }
      cleanedUp = true;
      try {
        if (dataReceiveProgressHandler) {
          httpRequest.off('dataReceiveProgress', dataReceiveProgressHandler);
          dataReceiveProgressHandler = undefined;
        }
        if (headersReceiveHandler) {
          httpRequest.off('headersReceive', headersReceiveHandler);
          headersReceiveHandler = undefined;
        }
        if (dataReceiveHandler) {
          httpRequest.off('dataReceive', dataReceiveHandler);
          dataReceiveHandler = undefined;
        }
        if (dataEndHandler) {
          httpRequest.off('dataEnd', dataEndHandler);
          dataEndHandler = undefined;
        }
      } catch (err) {
        // eslint-disable-next-line no-console
        console.warn('Failed to remove httpRequest listeners: ' + err);
      }
      httpRequest.destroy();
    }

    function maybeResolve() {
      if (headers !== undefined && responseCode !== undefined) {
        if (writeError) {
          reject(writeError);
        } else {
          resolve({headers, responseCode});
        }
        cleanUp();
      }
    }

    if (abortController) {
      abortController.onAbort = () => {
        cleanUp();
        reject(
          new AbortRequestError(
            `The download task has been removed.`,
            'E_PREFETCH_ABORT',
          ),
        );
      };
    }

    dataReceiveProgressHandler = ({receiveSize, totalSize}) => {
      onProgress?.(receiveSize / totalSize);
    };
    httpRequest.on('dataReceiveProgress', dataReceiveProgressHandler);

    headersReceiveHandler = data => {
      headers = data;
      maybeResolve();
    };
    httpRequest.on('headersReceive', headersReceiveHandler);

    dataReceiveHandler = chunk => {
      if (writeError || cleanedUp) {
        return;
      }
      writeChain = writeChain
        .then(() => {
          if (writeError || cleanedUp) {
            return;
          }
          return writeChunk(chunk);
        })
        .catch(err => {
          writeError = err instanceof Error ? err : new Error(String(err));
        });
    };
    httpRequest.on('dataReceive', dataReceiveHandler);

    dataEndHandler = () => {
      writeChain.finally(() => {
        maybeResolve();
      });
    };
    httpRequest.on('dataEnd', dataEndHandler);

    try {
      httpRequest.requestInStream(
        url,
        {
          header: options.headers,
          usingCache: options.usingCache,
          readTimeout: STREAM_READ_TIMEOUT,
          remoteValidation: getRemoteValidation(options.remoteValidation),
        },
        (err, data) => {
          if (err) {
            cleanUp();
            reject(
              new Error(`Couldn't fetch data from ${url}, ${err.message}`),
            );
          } else {
            responseCode = data;
            maybeResolve();
          }
        },
      );
    } catch (err) {
      cleanUp();
      reject(new Error(`Couldn't fetch data from ${url}, ${err.message}`));
    }
  });
}

/**
 * Downloads `url` to `filePath` via streaming, supporting the remoteValidation
 * certificate option that @ohos.request.downloadFile does NOT expose.
 *
 * Temp-file protection: streams to `<filePath>_tmp` first, then atomically moves
 * it to `filePath` via moveFile only after the download fully succeeds. On any
 * failure or abort, the temp file is deleted so `filePath` never holds a partial
 * file. The file descriptor is closed on every exit path.
 *
 * @param url - The URL to download
 * @param filePath - The final destination file path
 * @param options - Fetch options (headers, usingCache, remoteValidation, ...)
 * @param onProgress - Optional progress callback (0-1)
 * @param abortController - Optional abort controller
 * @returns A promise that resolves once the file is fully written and moved.
 * @throws {AbortRequestError} when aborted (code 'E_PREFETCH_ABORT')
 * @throws {Error} on HTTP non-2xx, network, or I/O failure
 */
export async function downloadFileFromUrl(
  url: string,
  filePath: string,
  options: FetchOptions,
  onProgress?: (progress: number) => void,
  abortController?: FetchAbortController,
): Promise<void> {
  const tempPath = filePath + '_tmp';
  let file: fs.File | undefined;
  try {
    file = await fs.open(
      tempPath,
      // eslint-disable-next-line no-bitwise
      fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE | fs.OpenMode.TRUNC,
    );
    const result = await streamDataFromUrl(
      url,
      options,
      async (chunk: ArrayBuffer) => {
        await fs.write(file!.fd, chunk);
      },
      onProgress,
      abortController,
    );
    if (
      result.responseCode === undefined ||
      result.responseCode < 200 ||
      result.responseCode >= 300
    ) {
      throw new Error(`HTTP error ${result.responseCode} for ${url}`);
    }
    await fs.close(file);
    file = undefined;
    await fs.moveFile(tempPath, filePath);
  } catch (e) {
    // Download failed/interrupted: clean up the file handle and temp file.
    try {
      if (file !== undefined) {
        try {
          await fs.close(file);
        } catch (_) {
          // Ignore close errors during cleanup
        }
        file = undefined;
      }
      if (await fs.access(tempPath)) {
        await fs.unlink(tempPath);
      }
    } catch (cleanupErr) {
      // Cleanup failure does not affect the main error propagation.
    }
    throw e;
  }
}
